/*
 * Copyright (C) 2025 Pedro Henrique / phkaiser13
 *
 * File: preview_controller.rs
 *
 * This file implements the reconciliation logic for the PhgitPreview custom resource.
 * Its primary purpose is to manage ephemeral preview environments within a Kubernetes
 * cluster. The controller watches for PhgitPreview objects and, upon creation or update,
 * triggers the deployment of application manifests from a specified Git repository into a
 * temporary, isolated namespace.
 *
 * Architecture:
 * The controller follows the standard Kubernetes operator pattern, driven by a reconcile
 * loop that seeks to bring the cluster's actual state in line with the desired state
 * defined by the PhgitPreview resource.
 *
 * Core Logic:
 * - `reconcile`: The main entry point for the reconciliation loop. It handles the creation
 * and deletion of preview environments.
 * - `apply_preview`: This function contains the logic to create a preview environment. It
 * performs the following steps:
 * 1. Creates a unique, temporary namespace for the preview.
 * 2. Clones the specified Git repository at a given revision (branch, tag, or commit hash).
 * 3. Locates Kubernetes manifest files (YAML) within the cloned repository at the specified path.
 * 4. Parses and applies each manifest to the newly created namespace using the Kubernetes API.
 * 5. Updates the status of the PhgitPreview resource to reflect the successful deployment
 * and the name of the created namespace.
 * - `cleanup_preview`: This function handles the teardown of the preview environment. It is
 * responsible for:
 * 1. Deleting the entire namespace created for the preview, which garbage-collects all
 * resources within it (Deployments, Services, etc.).
 * 2. (Future extension) Archiving logs or other important artifacts before deletion.
 *
 * The implementation leverages the `kube-rs` library for Kubernetes API interactions,
 * `tokio` for asynchronous operations, and the `git2` crate for Git repository manipulation.
 * Error handling is managed throughout to ensure the operator is resilient and provides
 * clear status updates.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

use kube::{
    api::{Api, DeleteParams, PostParams, ResourceExt},
    client::Client,
    runtime::controller::Action,
    Error as KubeError,
};
use std::sync::Arc;
use std::time::Duration;
use thiserror::Error;
use tokio::process::Command;

use crate::crds::PhgitPreview;

// Custom error types for the controller for better diagnostics.
#[derive(Debug, Error)]
pub enum PreviewError {
    #[error("Failed to apply Kubernetes manifests: {0}")]
    ManifestApplyError(String),

    #[error("Failed to clone Git repository: {0}")]
    GitCloneError(String),

    #[error("Failed to create namespace: {0}")]
    NamespaceCreationError(String),

    #[error("Kubernetes API error: {0}")]
    KubeError(#[from] KubeError),

    #[error("Missing PhgitPreview spec")]
    MissingSpec,
}

/// The context required by the reconciler. It holds the Kubernetes client.
pub struct Context {
    pub client: Client,
}

/// Main reconciliation function for the PhgitPreview resource.
/// This function is the entry point of the controller's reconciliation loop.
///
/// # Arguments
/// * `preview` - An Arc-wrapped PhgitPreview resource that triggered the reconciliation.
/// * `ctx` - An Arc-wrapped Context containing the Kubernetes client.
///
/// # Returns
/// A `Result` with either an `Action` to be taken by the controller runtime or a `PreviewError`.
pub async fn reconcile(preview: Arc<PhgitPreview>, ctx: Arc<Context>) -> Result<Action, PreviewError> {
    // The resource's namespace is a required piece of information.
    let ns = preview
        .namespace()
        .ok_or_else(|| PreviewError::KubeError(KubeError::Request(http::Error::new("Missing namespace"))))?;
    let previews: Api<PhgitPreview> = Api::namespaced(ctx.client.clone(), &ns);

    // Check for the deletion timestamp to determine if the resource is being deleted.
    if preview.metadata.deletion_timestamp.is_some() {
        // The resource is being deleted.
        cleanup_preview(preview, ctx).await?;
        // No further action is needed after cleanup.
        Ok(Action::await_change())
    } else {
        // The resource is being created or updated.
        apply_preview(preview, ctx).await?;
        // Requeue the resource for a check after 10 minutes, for example.
        Ok(Action::requeue(Duration::from_secs(600)))
    }
}

/// Creates and manages the preview environment.
///
/// This function implements the core logic for setting up a new preview environment.
/// It creates a namespace, clones a Git repository, and applies the Kubernetes
/// manifests found within it.
///
/// # Arguments
/// * `preview` - The PhgitPreview resource instance.
/// * `ctx` - The controller context with the Kubernetes client.
async fn apply_preview(preview: Arc<PhgitPreview>, ctx: Arc<Context>) -> Result<(), PreviewError> {
    let client = &ctx.client;
    let spec = preview.spec.as_ref().ok_or(PreviewError::MissingSpec)?;

    // Generate a unique namespace name for the preview environment.
    // Example: "preview-pr-123-app-b5a3c1"
    let ns_name = format!(
        "preview-{}-{}-{}",
        spec.branch.replace('/', "-"),
        spec.app_name,
        &preview.metadata.uid.as_ref().unwrap()[..6]
    );

    // 1. Create the Kubernetes Namespace.
    // The namespace provides isolation for the preview deployment.
    let ns_api: Api<k8s_openapi::api::core::v1::Namespace> = Api::all(client.clone());
    let new_ns = serde_json::from_value(serde_json::json!({
        "apiVersion": "v1",
        "kind": "Namespace",
        "metadata": { "name": ns_name }
    }))
        .expect("Failed to create namespace definition"); // This should not fail.

    match ns_api.create(&PostParams::default(), &new_ns).await {
        Ok(_) => println!("Namespace '{}' created successfully.", ns_name),
        Err(e) => {
            // It's possible the namespace already exists from a previous reconciliation attempt.
            if let KubeError::Api(ae) = &e {
                if ae.code == 409 {
                    // Conflict: Namespace already exists. We can proceed.
                    println!("Namespace '{}' already exists. Continuing.", ns_name);
                } else {
                    return Err(PreviewError::NamespaceCreationError(e.to_string()));
                }
            } else {
                return Err(PreviewError::NamespaceCreationError(e.to_string()));
            }
        }
    }

    // 2. Clone the Git repository.
    // We use a temporary directory to clone the repository into.
    let temp_dir = tempfile::Builder::new()
        .prefix("phgit-preview-")
        .tempdir()
        .map_err(|e| PreviewError::GitCloneError(e.to_string()))?;
    let repo_path = temp_dir.path().to_str().unwrap();

    println!("Cloning repository '{}' into '{}'", spec.repo_url, repo_path);

    let git_clone_status = Command::new("git")
        .arg("clone")
        .arg("--branch")
        .arg(&spec.branch)
        .arg("--single-branch")
        .arg("--depth")
        .arg("1")
        .arg(&spec.repo_url)
        .arg(repo_path)
        .status()
        .await
        .map_err(|e| PreviewError::GitCloneError(e.to_string()))?;

    if !git_clone_status.success() {
        return Err(PreviewError::GitCloneError(
            "Git clone command failed".to_string(),
        ));
    }

    // 3. Find and apply Kubernetes manifests.
    // The `manifest_path` specifies a directory inside the cloned repo.
    let manifest_dir = temp_dir.path().join(&spec.manifest_path);
    if !manifest_dir.is_dir() {
        return Err(PreviewError::ManifestApplyError(format!(
            "Manifest path '{}' not found or is not a directory.",
            manifest_dir.display()
        )));
    }

    println!("Applying manifests from '{}' to namespace '{}'", manifest_dir.display(), ns_name);

    // Iterate over YAML/YML files in the manifest directory.
    for entry in std::fs::read_dir(manifest_dir).map_err(|e| PreviewError::ManifestApplyError(e.to_string()))? {
        let entry = entry.map_err(|e| PreviewError::ManifestApplyError(e.to_string()))?;
        let path = entry.path();
        if path.is_file() {
            if let Some(ext) = path.extension() {
                if ext == "yaml" || ext == "yml" {
                    let manifest_content = std::fs::read_to_string(&path)
                        .map_err(|e| PreviewError::ManifestApplyError(format!("Failed to read manifest {}: {}", path.display(), e)))?;

                    let kubectl_apply_status = Command::new("kubectl")
                        .arg("apply")
                        .arg("-n")
                        .arg(&ns_name)
                        .arg("-f")
                        .arg("-") // Read from stdin
                        .stdin(std::process::Stdio::piped())
                        .stdout(std::process::Stdio::piped())
                        .stderr(std::process::Stdio::piped())
                        .spawn()
                        .and_then(|mut child| {
                            let mut stdin = child.stdin.take().expect("Failed to open stdin");
                            std::thread::spawn(move || {
                                use std::io::Write;
                                stdin.write_all(manifest_content.as_bytes()).expect("Failed to write to stdin");
                            });
                            child.wait_with_output()
                        })
                        .map_err(|e| PreviewError::ManifestApplyError(e.to_string()))?;

                    if !kubectl_apply_status.status.success() {
                        let stderr = String::from_utf8_lossy(&kubectl_apply_status.stderr);
                        return Err(PreviewError::ManifestApplyError(format!(
                            "Failed to apply manifest {}: {}",
                            path.display(),
                            stderr
                        )));
                    }
                    println!("Successfully applied manifest: {}", path.display());
                }
            }
        }
    }

    // TODO: Update the status of the PhgitPreview resource to reflect the deployed state.
    // This would involve setting a "status.url" or "status.namespace" field.

    println!("Preview environment for '{}' created successfully in namespace '{}'.", preview.name_any(), ns_name);

    Ok(())
}

/// Cleans up the resources created for a preview environment.
///
/// This function is triggered when a PhgitPreview resource is deleted. It ensures
/// that the corresponding namespace and all its contents are removed from the cluster.
///
/// # Arguments
/// * `preview` - The PhgitPreview resource being deleted.
/// * `ctx` - The controller context with the Kubernetes client.
async fn cleanup_preview(preview: Arc<PhgitPreview>, ctx: Arc<Context>) -> Result<(), PreviewError> {
    // The UID is used to generate the unique namespace name.
    let uid = preview.uid().ok_or_else(|| {
        PreviewError::KubeError(KubeError::Request(http::Error::new("Missing UID")))
    })?;
    let spec = preview.spec.as_ref().ok_or(PreviewError::MissingSpec)?;

    let ns_name = format!(
        "preview-{}-{}-{}",
        spec.branch.replace('/', "-"),
        spec.app_name,
        &uid[..6]
    );

    // Delete the namespace. Kubernetes will handle garbage collection of all
    // resources within that namespace.
    let ns_api: Api<k8s_openapi::api::core::v1::Namespace> = Api::all(ctx.client.clone());
    println!("Deleting namespace '{}' for preview '{}'", ns_name, preview.name_any());

    match ns_api.delete(&ns_name, &DeleteParams::default()).await {
        Ok(_) => {
            println!("Namespace '{}' deleted successfully.", ns_name);
        }
        Err(e) => {
            // If the namespace is already gone, that's fine.
            if let KubeError::Api(ae) = &e {
                if ae.code == 404 { // Not Found
                    println!("Namespace '{}' already deleted.", ns_name);
                } else {
                    eprintln!("Error deleting namespace '{}': {}", ns_name, e);
                }
            } else {
                eprintln!("Error deleting namespace '{}': {}", ns_name, e);
            }
        }
    }

    // Here you could add logic to archive logs from pods in the namespace
    // before it's deleted, or perform other cleanup tasks.

    Ok(())
}

/// Error handling function for the reconciliation loop.
///
/// # Arguments
/// * `preview` - The PhgitPreview resource that caused the error.
/// * `error` - The error that occurred.
/// * `ctx` - The controller context.
///
/// # Returns
/// An `Action` to instruct the controller runtime on how to proceed.
pub fn on_error(preview: Arc<PhgitPreview>, error: &PreviewError, ctx: Arc<Context>) -> Action {
    eprintln!(
        "Reconciliation error for PhgitPreview '{}': {:?}",
        preview.name_any(),
        error
    );
    // When an error occurs, we requeue the reconciliation request after a short delay.
    Action::requeue(Duration::from_secs(5))
}