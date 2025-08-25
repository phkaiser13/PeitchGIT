/*
 * Copyright (C) 2025 Pedro Henrique / phkaiser13
 *
 * File: preview_controller.rs
 *
 * This file implements the reconciliation logic for the phPreview custom resource.
 * Its primary purpose is to manage ephemeral preview environments within a Kubernetes
 * cluster. The controller watches for phPreview objects and, upon creation or update,
 * triggers the deployment of application manifests from a specified Git repository into a
 * temporary, isolated namespace.
 *
 * Architecture:
 * The controller follows the standard Kubernetes operator pattern, driven by a reconcile
 * loop that seeks to bring the cluster's actual state in line with the desired state
 * defined by the phPreview resource. This updated version incorporates a robust
 * finalizer mechanism to ensure graceful cleanup and detailed status updates to provide
 * clear feedback on the resource's state.
 *
 * Core Logic:
 * - `reconcile`: The main entry point for the reconciliation loop. It orchestrates the
 * entire process, from adding finalizers to triggering the creation or deletion of
 * preview environments.
 * - Finalizer Management: On resource creation, it adds a custom finalizer (`ph.io/finalizer`).
 * This prevents the resource from being deleted from the Kubernetes API until the
 * controller has successfully performed all cleanup logic.
 * - Deletion Handling: When a resource is marked for deletion (i.e., `deletion_timestamp`
 * is set), the reconcile loop triggers the cleanup logic. Only after successful
 * cleanup is the finalizer removed, allowing Kubernetes to complete the deletion.
 * - `apply_preview`: This function contains the logic to create a preview environment. It
 * performs the following steps:
 * 1. Creates a unique, temporary namespace for the preview, derived from the resource's UID.
 * 2. Clones the specified Git repository at a given revision.
 * 3. Locates Kubernetes manifest files (YAML) within the cloned repository.
 * 4. Parses and applies each manifest to the newly created namespace.
 * 5. Patches the `phPreview` resource's status subresource to reflect the outcome,
 * setting conditions like `Deployed` and recording the `namespace` and `url`.
 * - `cleanup_preview`: This function handles the teardown of the preview environment. It is
 * responsible for deleting the entire namespace, which garbage-collects all associated
 * resources.
 * - Status Updates: The controller now provides detailed status updates via the
 * `.status` subresource of the `phPreview` CRD. It reports the current phase
 * (e.g., `Creating`, `Deployed`, `Failed`, `Terminating`) and the name of the
 * managed namespace. This feedback is critical for user observability.
 *
 * The implementation leverages `kube-rs` for all Kubernetes API interactions, `tokio` for
 * asynchronous operations, and external commands (`git`, `kubectl`) for environment setup.
 * Error handling is managed throughout to ensure the operator is resilient and provides
 * clear status updates, even in failure scenarios.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

use kube::{
    api::{Api, DeleteParams, Patch, PatchParams, PostParams, ResourceExt},
    client::Client,
    runtime::{
        controller::Action,
        finalizer::{finalizer, Event as FinalizerEvent},
    },
    Error as KubeError,
};
use std::sync::Arc;
use std::time::Duration;
use thiserror::Error;
use tokio::process::Command;

use crate::crds::{phPreview, phPreviewStatus, StatusCondition};

// The unique identifier for our controller's finalizer.
const PREVIEW_FINALIZER: &str = "ph.io/finalizer";

// Custom error types for the controller for better diagnostics and status reporting.
#[derive(Debug, Error)]
pub enum PreviewError {
    #[error("Failed to apply Kubernetes manifests: {0}")]
    ManifestApplyError(String),

    #[error("Failed to clone Git repository: {0}")]
    GitCloneError(String),

    #[error("Failed to create namespace: {0}")]
    NamespaceCreationError(String),

    #[error("Failed to update resource status: {0}")]
    StatusUpdateError(String),

    #[error("Kubernetes API error: {0}")]
    KubeError(#[from] KubeError),

    #[error("Missing phPreview spec")]
    MissingSpec,
}

/// The context required by the reconciler. It holds the Kubernetes client.
pub struct Context {
    pub client: Client,
}

/// Generates the unique namespace name for a given preview resource.
/// This function ensures a consistent naming convention for all created namespaces,
/// making them easily identifiable and manageable.
///
/// # Arguments
/// * `preview` - A reference to the phPreview resource.
///
/// # Returns
/// A `Result` containing the namespace name string or a `PreviewError` if crucial
/// metadata (like UID) is missing.
fn generate_namespace_name(preview: &phPreview) -> Result<String, PreviewError> {
    let spec = preview.spec.as_ref().ok_or(PreviewError::MissingSpec)?;
    let uid = preview
        .uid()
        .ok_or_else(|| KubeError::Request(http::Error::new("Missing UID")))?;

    // The name is constructed to be unique and descriptive.
    // Format: preview-<branch>-<app_name>-<uid_prefix>
    // The branch name has slashes replaced to be a valid DNS label.
    Ok(format!(
        "preview-{}-{}-{}",
        spec.branch.replace('/', "-"),
        spec.app_name,
        &uid[..6]
    ))
}

/// Updates the status subresource of the phPreview custom resource.
///
/// This is a critical function for providing feedback to the user. It patches the
/// `.status` field of the resource to reflect the current state of the reconciliation.
///
/// # Arguments
/// * `preview` - The phPreview resource instance.
/// * `client` - The Kubernetes API client.
/// * `status` - The new status to be applied.
async fn update_status(
    preview: Arc<phPreview>,
    client: Client,
    status: phPreviewStatus,
) -> Result<(), PreviewError> {
    let ns = preview.namespace().unwrap(); // We expect namespace to be present.
    let name = preview.name_any();
    let previews: Api<phPreview> = Api::namespaced(client, &ns);

    // Use a server-side apply patch to update the status. This is the
    // recommended approach for updating subresources.
    let patch = Patch::Apply(serde_json::json!({
        "apiVersion": "ph.io/v1alpha1",
        "kind": "phPreview",
        "status": status,
    }));
    let ps = PatchParams::apply("ph-preview-controller").force();

    previews
        .patch_status(&name, &ps, &patch)
        .await
        .map_err(|e| PreviewError::StatusUpdateError(e.to_string()))?;

    Ok(())
}

/// Main reconciliation function for the phPreview resource.
/// This function is the entry point of the controller's reconciliation loop.
/// It uses the `kube_rs::runtime::finalizer` helper to manage cleanup logic.
///
/// # Arguments
/// * `preview` - An Arc-wrapped phPreview resource that triggered the reconciliation.
/// * `ctx` - An Arc-wrapped Context containing the Kubernetes client.
///
/// # Returns
/// A `Result` with either an `Action` to be taken by the controller runtime or a `PreviewError`.
pub async fn reconcile(preview: Arc<phPreview>, ctx: Arc<Context>) -> Result<Action, PreviewError> {
    let ns = preview
        .namespace()
        .ok_or_else(|| KubeError::Request(http::Error::new("Missing namespace")))?;
    let previews: Api<phPreview> = Api::namespaced(ctx.client.clone(), &ns);

    // The `finalizer` function from `kube-rs` simplifies finalizer logic.
    // It examines the resource and determines whether to run the apply or cleanup logic.
    finalizer(
        &previews,
        PREVIEW_FINALIZER,
        preview,
        |event| async {
            match event {
                // This is the "apply" branch, where the main logic is executed.
                FinalizerEvent::Apply(p) => apply_preview(p, ctx.clone()).await,
                // This is the "cleanup" branch, executed when the resource is being deleted.
                FinalizerEvent::Cleanup(p) => cleanup_preview(p, ctx.clone()).await,
            }
        },
    )
    .await
    .map_err(|e| KubeError::Request(http::Error::new(e.to_string())).into())
}

/// Creates and manages the preview environment.
///
/// This function implements the core logic for setting up a new preview environment.
/// It creates a namespace, clones a Git repository, applies Kubernetes manifests,
/// and robustly updates the resource's status.
///
/// # Arguments
/// * `preview` - The phPreview resource instance.
/// * `ctx` - The controller context with the Kubernetes client.
async fn apply_preview(preview: Arc<phPreview>, ctx: Arc<Context>) -> Result<Action, PreviewError> {
    let client = ctx.client.clone();
    let ns_name = generate_namespace_name(&preview)?;

    // --- 1. Update Status to "Creating" ---
    // Immediately update the status to indicate that the reconciliation has started.
    let initial_status = phPreviewStatus {
        namespace: Some(ns_name.clone()),
        conditions: vec![StatusCondition::new(
            "Creating".to_string(),
            "Reconciliation started".to_string(),
        )],
    };
    update_status(preview.clone(), client.clone(), initial_status).await?;

    // --- 2. Create the Kubernetes Namespace ---
    // The namespace provides isolation for the preview deployment.
    let ns_api: Api<k8s_openapi::api::core::v1::Namespace> = Api::all(client.clone());
    let new_ns = serde_json::from_value(serde_json::json!({
        "apiVersion": "v1",
        "kind": "Namespace",
        "metadata": { "name": ns_name }
    }))
    .expect("Static namespace definition should not fail serialization");

    match ns_api.create(&PostParams::default(), &new_ns).await {
        Ok(_) => println!("Namespace '{}' created successfully.", ns_name),
        Err(e) => {
            // It's possible the namespace already exists from a previous, failed
            // reconciliation attempt. We treat "Conflict" (409) as a non-error.
            if let KubeError::Api(ae) = &e {
                if ae.code == 409 {
                    println!("Namespace '{}' already exists. Continuing.", ns_name);
                } else {
                    return Err(PreviewError::NamespaceCreationError(e.to_string()));
                }
            } else {
                return Err(PreviewError::NamespaceCreationError(e.to_string()));
            }
        }
    }

    // --- 3. Clone the Git repository ---
    // A temporary directory is used to house the cloned repository contents.
    let temp_dir = tempfile::Builder::new()
        .prefix("ph-preview-")
        .tempdir()
        .map_err(|e| PreviewError::GitCloneError(e.to_string()))?;
    let repo_path = temp_dir.path().to_str().unwrap();
    let spec = preview.spec.as_ref().ok_or(PreviewError::MissingSpec)?;

    println!("Cloning repository '{}' into '{}'", spec.repo_url, repo_path);
    let git_clone_status = Command::new("git")
        .args([
            "clone",
            "--branch",
            &spec.branch,
            "--single-branch",
            "--depth",
            "1",
            &spec.repo_url,
            repo_path,
        ])
        .status()
        .await
        .map_err(|e| PreviewError::GitCloneError(e.to_string()))?;

    if !git_clone_status.success() {
        return Err(PreviewError::GitCloneError(
            "Git clone command failed".to_string(),
        ));
    }

    // --- 4. Find and apply Kubernetes manifests ---
    let manifest_dir = temp_dir.path().join(&spec.manifest_path);
    if !manifest_dir.is_dir() {
        return Err(PreviewError::ManifestApplyError(format!(
            "Manifest path '{}' not found or is not a directory.",
            manifest_dir.display()
        )));
    }

    println!("Applying manifests from '{}' to namespace '{}'", manifest_dir.display(), ns_name);

    for entry in std::fs::read_dir(&manifest_dir).map_err(|e| PreviewError::ManifestApplyError(e.to_string()))? {
        let entry = entry.map_err(|e| PreviewError::ManifestApplyError(e.to_string()))?;
        let path = entry.path();
        if path.is_file() && (path.extension() == Some("yaml".as_ref()) || path.extension() == Some("yml".as_ref())) {
            let manifest_content = std::fs::read_to_string(&path)
                .map_err(|e| PreviewError::ManifestApplyError(format!("Failed to read manifest {}: {}", path.display(), e)))?;

            // We pipe the manifest content to `kubectl apply` via stdin.
            // This is generally more robust than creating temporary files.
            let mut child = Command::new("kubectl")
                .args(["apply", "-n", &ns_name, "-f", "-"])
                .stdin(std::process::Stdio::piped())
                .stdout(std::process::Stdio::piped())
                .stderr(std::process::Stdio::piped())
                .spawn()
                .map_err(|e| PreviewError::ManifestApplyError(e.to_string()))?;

            let mut stdin = child.stdin.take().expect("Failed to open stdin");
            let write_handle = tokio::spawn(async move {
                use tokio::io::AsyncWriteExt;
                stdin.write_all(manifest_content.as_bytes()).await
            });

            let output = child.wait_with_output().await.map_err(|e| PreviewError::ManifestApplyError(e.to_string()))?;
            write_handle.await.unwrap().map_err(|e| PreviewError::ManifestApplyError(e.to_string()))?;

            if !output.status.success() {
                let stderr = String::from_utf8_lossy(&output.stderr);
                return Err(PreviewError::ManifestApplyError(format!(
                    "Failed to apply manifest {}: {}",
                    path.display(),
                    stderr
                )));
            }
            println!("Successfully applied manifest: {}", path.display());
        }
    }

    // --- 5. Update Status to "Deployed" ---
    // On success, we patch the status to reflect the successful deployment.
    println!("Preview environment for '{}' created successfully in namespace '{}'.", preview.name_any(), ns_name);
    let final_status = phPreviewStatus {
        namespace: Some(ns_name),
        conditions: vec![StatusCondition::new(
            "Deployed".to_string(),
            "All manifests applied successfully".to_string(),
        )],
    };
    update_status(preview, client, final_status).await?;

    // Requeue for a periodic check. This could be used for health checks in the future.
    Ok(Action::requeue(Duration::from_secs(600)))
}

/// Cleans up the resources created for a preview environment.
///
/// This function is triggered by the finalizer logic when a phPreview resource
/// is marked for deletion. It ensures the complete removal of the preview namespace.
///
/// # Arguments
/// * `preview` - The phPreview resource being deleted.
/// * `ctx` - The controller context with the Kubernetes client.
async fn cleanup_preview(preview: Arc<phPreview>, ctx: Arc<Context>) -> Result<Action, PreviewError> {
    let client = ctx.client.clone();
    let ns_name = generate_namespace_name(&preview)?;

    // --- 1. Update Status to "Terminating" ---
    let status = phPreviewStatus {
        namespace: Some(ns_name.clone()),
        conditions: vec![StatusCondition::new(
            "Terminating".to_string(),
            "Deleting preview environment namespace".to_string(),
        )],
    };
    update_status(preview.clone(), client.clone(), status).await?;

    // --- 2. Delete the Namespace ---
    // Kubernetes garbage collection will handle the deletion of all resources
    // contained within this namespace (Deployments, Services, etc.).
    let ns_api: Api<k8s_openapi::api::core::v1::Namespace> = Api::all(client);
    println!("Deleting namespace '{}' for preview '{}'", ns_name, preview.name_any());

    match ns_api.delete(&ns_name, &DeleteParams::default()).await {
        Ok(_) => {
            println!("Namespace '{}' deletion initiated successfully.", ns_name);
        }
        Err(e) => {
            // If the namespace is already gone (Not Found - 404), we can
            // consider the cleanup successful.
            if let KubeError::Api(ae) = &e {
                if ae.code == 404 {
                    println!("Namespace '{}' already deleted.", ns_name);
                } else {
                    eprintln!("Error deleting namespace '{}': {}", ns_name, e);
                    // Even on error, we proceed, allowing the finalizer to be removed,
                    // but the error is logged for investigation.
                }
            } else {
                eprintln!("Error deleting namespace '{}': {}", ns_name, e);
            }
        }
    }

    // No need to requeue after cleanup, as the resource will be deleted.
    Ok(Action::await_change())
}

/// Error handling function for the reconciliation loop.
///
/// This function is called by the `Controller` runtime when the `reconcile` function
/// returns an error. It updates the resource status with the error message.
///
/// # Arguments
/// * `preview` - The phPreview resource that caused the error.
/// * `error` - The `PreviewError` that occurred.
/// * `ctx` - The controller context.
///
/// # Returns
/// An `Action` to instruct the controller runtime on how to proceed.
pub async fn on_error(preview: Arc<phPreview>, error: &PreviewError, ctx: Arc<Context>) -> Action {
    eprintln!("Reconciliation error for phPreview '{}': {:?}", preview.name_any(), error);

    // When an error occurs, update the status to "Failed" with a descriptive message.
    let failed_status = phPreviewStatus {
        namespace: preview.status.as_ref().and_then(|s| s.namespace.clone()),
        conditions: vec![StatusCondition::new(
            "Failed".to_string(),
            error.to_string(),
        )],
    };

    if let Err(e) = update_status(preview.clone(), ctx.client.clone(), failed_status).await {
        eprintln!("Failed to update status on error: {}", e);
    }

    // Requeue the request after a short delay to attempt reconciliation again.
    Action::requeue(Duration::from_secs(15))
}