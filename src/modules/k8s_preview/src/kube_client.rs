/* Copyright (C) 2025 Pedro Henrique / phkaiser13
* File: src/modules/k8s_preview/src/kube_client.rs
* This file provides a high-level, abstracted interface for interacting with the
* Kubernetes cluster. It encapsulates the logic of using the `kube-rs` crate,
* exposing simple, task-oriented functions. This separation of concerns keeps
* the main business logic clean and independent of the specific Kubernetes
* client implementation details.
* SPDX-License-Identifier: Apache-2.0 */

use anyhow::{Context, Result};
use k8s_openapi::api::core::v1::Namespace;
use kube::{
    api::{Api, DeleteParams, PostParams, ObjectMeta},
    Client, Config,
};
use std::collections::BTreeMap;
use std::path::Path;

/// Initializes a Kubernetes client from a configuration.
///
/// It attempts to load configuration from an optional file path first.
/// If the path is `None`, it falls back to `Config::infer`, which automatically
/// tries in-cluster configuration, then the default kubeconfig file (`~/.kube/config`),
/// and finally environment variables.
///
/// # Arguments
/// * `kubeconfig_path` - An optional slice that holds the path to the kubeconfig file.
pub async fn initialize_client(kubeconfig_path: Option<&str>) -> Result<Client> {
    let config = match kubeconfig_path {
        Some(path) => {
            // Create a config from a specific file path.
            Config::from_kubeconfig(&kube::config::Kubeconfig::read_from(path)?)
                .await
                .context("Failed to create Kubernetes config from custom path")?
        }
        None => {
            // Infer the config from the environment.
            Config::infer()
                .await
                .context("Failed to infer Kubernetes config from environment")?
        }
    };
    Client::try_from(config).context("Failed to create Kubernetes client from config")
}

/// Creates a new namespace in the Kubernetes cluster with specific labels and annotations.
///
/// # Arguments
/// * `client` - The Kubernetes client instance.
/// * `name` - The name of the namespace to create (e.g., "pr-123").
/// * `ttl_hours` - The Time-To-Live for the namespace, stored as an annotation.
pub async fn create_namespace(client: &Client, name: &str, ttl_hours: u32) -> Result<()> {
    let ns_api: Api<Namespace> = Api::all(client.clone());

    let mut labels = BTreeMap::new();
    labels.insert("managed-by".to_string(), "Peitch".to_string());

    let mut annotations = BTreeMap::new();
    annotations.insert("Peitch.io/ttl-hours".to_string(), ttl_hours.to_string());

    let namespace = Namespace {
        metadata: ObjectMeta {
            name: Some(name.to_string()),
            labels: Some(labels),
            annotations: Some(annotations),
            ..ObjectMeta::default()
        },
        ..Namespace::default()
    };

    ns_api
        .create(&PostParams::default(), &namespace)
        .await
        .with_context(|| format!("Failed to create Kubernetes namespace '{}'", name))?;

    println!("Successfully created namespace: {}", name);
    Ok(())
}

/// Deletes a namespace from the Kubernetes cluster.
/// This function is idempotent: if the namespace does not exist, it returns success.
///
/// # Arguments
/// * `client` - The Kubernetes client instance.
/// * `name` - The name of the namespace to delete.
pub async fn delete_namespace(client: &Client, name: &str) -> Result<()> {
    let ns_api: Api<Namespace> = Api::all(client.clone());

    match ns_api.delete(name, &DeleteParams::default()).await {
        Ok(_) => {
            println!("Successfully deleted namespace: {}", name);
            Ok(())
        }
        Err(kube::Error::Api(e)) if e.code == 404 => {
            // If the resource doesn't exist, it's a success for a delete operation.
            println!("Namespace '{}' not found, assuming already deleted.", name);
            Ok(())
        }
        Err(e) => {
            // For any other error, propagate it.
            Err(e).with_context(|| format!("Failed to delete Kubernetes namespace '{}'", name))
        }
    }
}

/// Applies Kubernetes manifests from a given repository path to a specific namespace.
///
/// NOTE: This is a placeholder implementation. A real-world implementation would
/// require walking the directory, parsing YAML files (potentially multi-document),
/// and applying them using a server-side apply strategy. This could be done by
/// shelling out to `kubectl apply` or by using `serde_yaml` and the dynamic `Api`
/// from `kube-rs`.
///
/// # Arguments
/// * `_client` - The Kubernetes client instance (unused in this placeholder).
/// * `namespace` - The target namespace for the manifests.
/// * `_repo_path` - The local path to the checked-out git repository.
pub async fn apply_manifests(
    _client: &Client,
    namespace: &str,
    _repo_path: &Path,
) -> Result<()> {
    // In a real implementation, we would clone the repo specified in the config
    // to a temporary directory (`_repo_path`) and then apply manifests from it.
    println!(
        "Simulating application of manifests from git repo to namespace '{}'...",
        namespace
    );
    // Simulate a successful operation.
    Ok(())
}