/*
 * Copyright (C) 2025 Pedro Henrique / phkaiser13
 *
 * File: src/modules/git_sync/src/kube_diff.rs
 *
 * This file implements the core logic for detecting configuration drift between a live
 * Kubernetes cluster and a Git repository that serves as the source of truth. The primary
 * function, `detect_drift`, connects to a specified Kubernetes cluster, inventories key
 * resources (Deployments, Services, and ConfigMaps), and compares their live state against
 * the configuration files stored in a local Git repository clone.
 *
 * Architecture:
 * The implementation leverages the powerful `kube-rs` crate for asynchronous interaction
 * with the Kubernetes API, ensuring non-blocking, high-performance operations. For the
 * diffing process, the `similar` crate is employed to generate human-readable, unified
 * diff patches, which are essential for clear reporting and subsequent reconciliation actions.
 *
 * The core architectural principles are:
 * 1. Asynchronicity: All Kubernetes API calls are `async` to handle network latency
 * gracefully without blocking the execution thread. This is crucial for a responsive
 * and scalable tool.
 * 2. Genericity and Reusability: The resource discovery logic is designed to be extensible.
 * While it currently targets Deployments, Services, and ConfigMaps, the pattern used
 * can be easily adapted to support other Kubernetes resource types by adding new
 * generic function calls.
 * 3. Robust Error Handling: The entire process is wrapped in `anyhow::Result` to provide
 * context-rich error messages, simplifying debugging and improving operational stability.
 * Every potential point of failure—from API connection to file I/O—is handled.
 * 4. Clear Data Structures: A dedicated `DriftDetail` struct is defined to encapsulate
 * the findings for each resource, including its name, kind, namespace, and the generated
 * diff patch. This creates a well-defined contract for the data returned by this module.
 * 5. Separation of Concerns: This file is solely responsible for diffing. It does not
 * handle Git operations (like creating a PR) or secret management, adhering to the
 * modular design of the application.
 *
 * The process flow is as follows:
 * - Establish a connection to the Kubernetes cluster.
 * - Iterate through all namespaces.
 * - For each namespace, list all Deployments, Services, and ConfigMaps.
 * - For each discovered resource, serialize its live specification into YAML format.
 * - Construct the expected path to the corresponding configuration file in the Git repository.
 * - Read the file from the Git repository.
 * - Generate a textual diff between the live YAML and the Git YAML.
 * - If a difference is detected, a `DriftDetail` object is created and added to a result vector.
 * - The final result is a collection of all detected drifts, which can then be used by other
 * modules, such as the `git_ops` module, to create a pull request for reconciliation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

use kube::{
    api::{Api, ListParams, ResourceExt},
    Client,
};
use k8s_openapi::api::{
    apps::v1::Deployment,
    core::v1::{ConfigMap, Service},
};
use serde::Serialize;
use similar::{ChangeTag, TextDiff};
use std::path::{Path, PathBuf};
use tokio::fs;
use anyhow::{Context, Result, anyhow};

/// Represents the details of a detected configuration drift for a single Kubernetes resource.
///
/// This struct is the primary output of the drift detection process. It contains all the
/// necessary information to identify the resource that has drifted and the exact changes
/// between its live state and the desired state in the Git repository.
#[derive(Debug, Serialize)]
pub struct DriftDetail {
    /// The kind of the Kubernetes resource (e.g., "Deployment", "Service").
    pub kind: String,
    /// The name of the Kubernetes resource.
    pub name: String,
    /// The namespace in which the resource resides.
    pub namespace: String,
    /// A unified diff patch representing the changes. If this string is empty,
    /// it implies no drift was detected, although in practice, only drifted
    /// resources will generate a `DriftDetail` instance.
    pub patch: String,
}

/// Detects configuration drift between a Kubernetes cluster and a Git repository.
///
/// This is the main entry point of the `kube_diff` module. It orchestrates the process of
/// connecting to the cluster, listing resources, and comparing them against the Git source of truth.
///
/// # Arguments
/// * `git_repo_path` - A reference to the path of the local clone of the Git repository.
///
/// # Returns
/// A `Result` containing a vector of `DriftDetail` structs for each resource where drift
/// was detected. Returns an error if the connection to the cluster fails or if there are
/// issues reading from the repository.
pub async fn detect_drift(git_repo_path: &Path) -> Result<Vec<DriftDetail>> {
    // 1. Establish a connection to the Kubernetes cluster.
    // `Client::try_default()` attempts to create a client from the standard kubeconfig
    // locations (~/.kube/config) or from in-cluster service account information.
    let client = Client::try_default().await.context("Failed to create Kubernetes client. Ensure kubeconfig is set up correctly.")?;
    let mut drifts = Vec::new();

    // 2. Discover all namespaces in the cluster.
    let namespaces_api: Api<k8s_openapi::api::core::v1::Namespace> = Api::all(client.clone());
    let namespaces = namespaces_api.list(&ListParams::default()).await.context("Failed to list namespaces.")?;

    // 3. Iterate over each namespace to find resources.
    for ns in namespaces {
        let namespace_name = ns.name_any();
        println!("Checking namespace: {}", namespace_name);

        // --- Check for different resource types ---
        // The process is repeated for each resource type we want to track.
        // This can be extended by adding more calls to `list_and_compare_resources`.

        // Check Deployments
        list_and_compare_resources::<Deployment>(client.clone(), &namespace_name, git_repo_path, &mut drifts).await?;

        // Check Services
        list_and_compare_resources::<Service>(client.clone(), &namespace_name, git_repo_path, &mut drifts).await?;

        // Check ConfigMaps
        list_and_compare_resources::<ConfigMap>(client.clone(), &namespace_name, git_repo_path, &mut drifts).await?;
    }

    Ok(drifts)
}

/// A generic function to list all resources of a specific kind within a namespace
/// and compare each one against its corresponding file in the Git repository.
///
/// # Type Parameters
/// * `K`: The type of the Kubernetes resource. It must satisfy a set of traits:
///   - `Resource`: Provides metadata about the resource kind, version, etc.
///   - `Debug`: To allow printing for debugging purposes.
///   - `Serialize`: To convert the resource object into YAML format.
///   - `for<'de> serde::Deserialize<'de>`: To allow deserialization from the K8s API.
///
/// # Arguments
/// * `client` - The Kubernetes client instance.
/// * `namespace` - The name of the namespace to scan.
/// * `git_repo_path` - The path to the Git repository.
/// * `drifts` - A mutable reference to the vector where drift details will be stored.
async fn list_and_compare_resources<K>(
    client: Client,
    namespace: &str,
    git_repo_path: &Path,
    drifts: &mut Vec<DriftDetail>,
) -> Result<()>
where
    K: kube::Resource<Scope = kube::core::NamespaceResourceScope> + serde::Serialize + for<'de> serde::Deserialize<'de> + std::fmt::Debug,
    <K as kube::Resource>::DynamicType: Default,
{
    // Obtain an API handle for the specific resource kind `K` in the given namespace.
    let api: Api<K> = Api::namespaced(client, namespace);
    let list = api.list(&ListParams::default()).await.with_context(|| format!("Failed to list {} in namespace {}", std::any::type_name::<K>(), namespace))?;

    for resource in list {
        let resource_name = resource.name_any();
        let kind = K::kind(&K::make_dynamic(&(), Default::default())).to_string();

        // The live state of the resource from the cluster. We serialize its 'spec' field.
        // We focus on the 'spec' because metadata often contains cluster-specific, runtime-generated
        // values that would create noise in the diff.
        let live_spec = resource.spec().ok_or_else(|| anyhow!("Resource {}/{} has no spec", kind, resource_name))?;
        let live_yaml = serde_yaml::to_string(&live_spec)
            .with_context(|| format!("Failed to serialize live spec for {}/{}", kind, resource_name))?;

        // Construct the expected file path in the Git repository.
        // The convention is: <repo_root>/<namespace>/<kind_lowercase>/<resource_name>.yaml
        let mut git_file_path = PathBuf::from(git_repo_path);
        git_file_path.push(namespace);
        git_file_path.push(kind.to_lowercase());
        git_file_path.push(format!("{}.yaml", resource_name));

        // Read the file content from the source of truth (Git).
        let git_yaml = match fs::read_to_string(&git_file_path).await {
            Ok(content) => content,
            Err(_) => {
                // If the file doesn't exist in Git, it's considered a drift (a new, untracked resource).
                println!("Resource {}/{} found in cluster but not in Git.", kind, resource_name);
                drifts.push(DriftDetail {
                    kind: kind.clone(),
                    name: resource_name.clone(),
                    namespace: namespace.to_string(),
                    patch: format!("--- /dev/null\n+++ {}\n{}", git_file_path.display(), live_yaml),
                });
                continue;
            }
        };

        // Perform the diff.
        let diff = TextDiff::from_lines(&git_yaml, &live_yaml);

        // Format the diff into a unified patch string. We only capture changes (insertions/deletions).
        let patch: String = diff
            .iter_all_changes()
            .filter(|change| change.tag() != ChangeTag::Equal)
            .map(|change| {
                let sign = match change.tag() {
                    ChangeTag::Delete => "-",
                    ChangeTag::Insert => "+",
                    ChangeTag::Equal => " ",
                };
                format!("{}{}", sign, change)
            })
            .collect();

        // If the patch string is not empty, it means a drift was detected.
        if !patch.is_empty() {
            println!("Drift detected for {}/{}", kind, resource_name);
            let full_patch = format!(
                "--- a/{0}\n+++ b/{0}\n{1}",
                git_file_path.strip_prefix(git_repo_path).unwrap_or(&git_file_path).display(),
                patch
            );

            drifts.push(DriftDetail {
                kind: kind.clone(),
                name: resource_name.clone(),
                namespace: namespace.to_string(),
                patch: full_patch,
            });
        }
    }
    Ok(())
}