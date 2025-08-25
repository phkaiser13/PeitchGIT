/*
 * Copyright (C) 2025 Pedro Henrique / phkaiser13
 *
 * File: preview_controller.rs
 *
 * This file contains the core reconciliation logic for the `PhgitPreview`
 * custom resource. The reconciler's primary goal is to ensure that the actual
 * state of the Kubernetes cluster matches the desired state specified in a
 * `PhgitPreview` object.
 *
 * Architecture:
 * - It implements the main `reconcile` function, which is the heart of the
 * controller loop provided by `kube-runtime`.
 * - A key feature is the use of a "finalizer". When a `PhgitPreview` is created,
 * we add our finalizer string to its metadata. This acts as a lock, preventing
 * Kubernetes from deleting the CR immediately upon a deletion request. Instead,
 * it updates the object with a `deletionTimestamp` and re-triggers our
 * reconciler. Our code can then detect this state and perform the necessary
 * cleanup (e.g., deleting the preview namespace). Only after our cleanup
 * succeeds and we remove the finalizer will Kubernetes complete the deletion.
 * This ensures no orphaned resources are left in the cluster.
 * - The controller is stateful and idempotent. It continuously checks the state
 * of required resources (like the namespace) and only creates them if they
 * don't exist.
 * - It updates the `.status` subresource of the `PhgitPreview` object to provide
 * real-time feedback to users about the state of their environment.
 * - An `error_policy` function is defined to handle errors gracefully,
 * implementing an exponential backoff strategy for requeuing failed actions.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

use crate::{crds::PhgitPreview, Context, Error};
use k8s_openapi::api::core::v1::Namespace;
use kube::{
    api::{Api, ObjectMeta, Patch, PatchParams},
    runtime::{
        controller::Action,
        finalizer::{finalizer, Event as FinalizerEvent},
    },
    ResourceExt,
};
use std::sync::Arc;
use std::time::Duration;
use tracing::{info, warn};

// A unique identifier for our finalizer.
static PHGIT_PREVIEW_FINALIZER: &str = "preview.phgit.io/finalizer";

/// The main reconciliation function for the PhgitPreview controller.
/// This function is called by the `kube-runtime` controller whenever a change
/// to a `PhgitPreview` resource is detected.
pub async fn reconcile(
    preview: Arc<PhgitPreview>,
    ctx: Arc<Context>,
) -> Result<Action, Error> {
    let client = ctx.client.clone();
    // Create an API handle for `PhgitPreview` resources in the correct namespace.
    let api: Api<PhgitPreview> =
        Api::namespaced(client.clone(), &preview.namespace().unwrap());

    // The `finalizer` function from `kube-runtime` provides a robust pattern
    // for managing resource lifecycles.
    finalizer(&api, PHGIT_PREVIEW_FINALIZER, preview, |event| async {
        match event {
            // This arm is called when a new resource is created or an existing one is updated.
            FinalizerEvent::Apply(preview) => apply_preview(preview, ctx.clone()).await,
            // This arm is called when a resource has a `deletionTimestamp`,
            // indicating it has been marked for deletion.
            FinalizerEvent::Cleanup(preview) => cleanup_preview(preview, ctx.clone()).await,
        }
    })
        .await
        .map_err(|e| e.into())
}

/// This function implements the main "apply" logic. It creates the necessary
/// resources for a preview environment.
async fn apply_preview(preview: Arc<PhgitPreview>, ctx: Arc<Context>) -> Result<Action, Error> {
    info!(
        "Applying PhgitPreview: {}/{}",
        preview.namespace().unwrap(),
        preview.name_any()
    );

    let client = ctx.client.clone();
    let ns_name = format!("pr-{}", preview.spec.pr_number);
    let ns_api: Api<Namespace> = Api::all(client.clone());

    // 1. Create the preview namespace idempotently using Server-Side Apply.
    let ns = Namespace {
        metadata: ObjectMeta {
            name: Some(ns_name.clone()),
            labels: Some([("created-by".to_string(), "phgit-operator".to_string())].into()),
            ..ObjectMeta::default()
        },
        ..Namespace::default()
    };
    // The `apply_manager` field is required for Server-Side Apply.
    let patch_params = PatchParams::apply("phgit-operator-preview-controller");
    ns_api.patch(&ns_name, &patch_params, &Patch::Apply(&ns)).await?;

    // 2. TODO: Implement Git repository cloning.
    // In a real implementation, this would involve using a Git library (like git2-rs)
    // or shelling out to the `git` command to clone `preview.spec.repo_url`
    // at `preview.spec.commit_sha` into a temporary directory.
    info!("Placeholder: Cloning repository {} at {}", preview.spec.repo_url, preview.spec.commit_sha);

    // 3. TODO: Apply manifests from the repository.
    // This logic would parse the YAML/JSON files from the cloned repository
    // (at `preview.spec.manifest_path`) and apply them to the created namespace
    // using the Kubernetes client.
    info!("Placeholder: Applying manifests to namespace '{}'", ns_name);

    // 4. Update the status of the PhgitPreview resource.
    let preview_api: Api<PhgitPreview> = Api::namespaced(client.clone(), &preview.namespace().unwrap());
    let status = serde_json::json!({
        "status": {
            "phase": "Ready",
            "url": format!("http://pr-{}.example.com", preview.spec.pr_number),
            "message": "Preview environment successfully provisioned."
        }
    });

    preview_api
        .patch_status(&preview.name_any(), &PatchParams::default(), &Patch::Merge(&status))
        .await?;

    // If all steps succeed, requeue the resource for a check in 10 minutes.
    Ok(Action::requeue(Duration::from_secs(600)))
}

/// This function implements the cleanup logic, which is triggered when a
/// PhgitPreview resource is deleted.
async fn cleanup_preview(preview: Arc<PhgitPreview>, ctx: Arc<Context>) -> Result<Action, Error> {
    info!(
        "Cleaning up PhgitPreview: {}/{}",
        preview.namespace().unwrap(),
        preview.name_any()
    );

    let client = ctx.client.clone();
    let ns_name = format!("pr-{}", preview.spec.pr_number);
    let ns_api: Api<Namespace> = Api::all(client.clone());

    // Delete the namespace. Kubernetes will handle garbage collection of all
    // resources within it.
    match ns_api.delete(&ns_name, &Default::default()).await {
        Ok(_) => info!("Deleted namespace '{}'", ns_name),
        Err(kube::Error::Api(e)) if e.code == 404 => {
            info!("Namespace '{}' already deleted, skipping.", ns_name);
        }
        Err(e) => return Err(e.into()),
    }

    // Returning `Ok` here will cause the `finalizer` helper to remove the
    // finalizer string from the resource, allowing Kubernetes to complete the deletion.
    Ok(Action::await_change())
}

/// Defines the error handling policy for the reconciliation loop.
/// If an error occurs, this function decides what action to take.
pub fn error_policy(_preview: Arc<PhgitPreview>, error: &Error, _ctx: Arc<Context>) -> Action {
    warn!("Reconciliation failed: {:?}", error);
    // Requeue the request after a delay. The controller uses exponential backoff.
    Action::requeue(Duration::from_secs(5))
}