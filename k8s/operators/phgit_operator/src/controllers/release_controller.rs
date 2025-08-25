/*
 * Copyright (C) 2025 Pedro Henrique / phkaiser13
 *
 * File: release_controller.rs
 *
 * This file contains the reconciliation logic for the PhgitRelease custom resource.
 * Its main role is to orchestrate advanced deployment strategies within Kubernetes,
 * specifically Canary and Blue/Green releases. The controller watches for PhgitRelease
 * objects and acts upon them to manage application lifecycle, ensuring safe and
 * controlled rollouts.
 *
 * Architecture:
 * The controller distinguishes between release strategies based on a `strategy` field
 * in the PhgitRelease spec. It delegates the reconciliation logic to specialized
 * functions for each strategy.
 *
 * Core Logic:
 * - `reconcile`: The main reconciliation loop. It determines the desired strategy
 * (e.g., "canary", "blue-green") and calls the appropriate handler. It also manages
 * the finalizer logic for cleanup.
 * - `handle_canary_release`: Implements the canary release strategy. This involves:
 * 1. Ensuring a stable "primary" Deployment exists.
 * 2. Creating a new "canary" Deployment with the updated container image. This
 * deployment runs alongside the primary but does not initially receive traffic.
 * 3. Creating or updating an Istio `VirtualService` to split traffic between the
 * primary and canary services. A specified percentage of traffic (e.g., 10%)
 * is routed to the canary for a testing period.
 * 4. (Future extension) Monitoring canary metrics and either promoting it by
 * shifting 100% of traffic or rolling back on failure.
 * - `handle_blue_green_release`: Implements the blue-green release strategy. This involves:
 * 1. Identifying the "active" (blue) deployment currently serving traffic.
 * 2. Deploying a new, full-scale "inactive" (green) version of the application
 * with a different version label.
 * 3. The "green" deployment can be tested internally via its own ClusterIP service.
 * 4. Upon promotion, the main Kubernetes Service's selector is atomically updated
 * to point from the "blue" version to the "green" version, instantly shifting
 * all user traffic.
 * 5. The old "blue" deployment is kept for a potential quick rollback.
 * - `cleanup`: Handles the removal of associated resources (like the VirtualService)
 * when a PhgitRelease object is deleted.
 *
 * This implementation relies heavily on `kube-rs` for native Kubernetes API
 * interactions, including creating and patching Deployments, Services, and custom
 * resources like Istio's VirtualService.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

use kube::{
    api::{Api, DeleteParams, Patch, PatchParams, PostParams, ResourceExt},
    client::Client,
    runtime::controller::Action,
    Error as KubeError,
};
use serde_json::json;
use std::sync::Arc;
use std::time::Duration;
use thiserror::Error;

use crate::crds::{PhgitRelease, ReleaseStrategy};

// Custom error types for the release controller.
#[derive(Debug, Error)]
pub enum ReleaseError {
    #[error("Kubernetes API error: {0}")]
    KubeError(#[from] KubeError),

    #[error("Missing PhgitRelease spec")]
    MissingSpec,

    #[error("Failed to apply resource: {0}")]
    ApplyError(String),
}

/// The context required by the reconciler.
pub struct Context {
    pub client: Client,
}

// Name for the finalizer to ensure cleanup logic is run before deletion.
const PHGIT_RELEASE_FINALIZER: &str = "phgitreleases.phgit.io/finalizer";

/// Main reconciliation function for the PhgitRelease resource.
///
/// # Arguments
/// * `release` - The PhgitRelease resource that triggered the reconciliation.
/// * `ctx` - The controller context containing the Kubernetes client.
///
/// # Returns
/// A `Result` with either an `Action` or a `ReleaseError`.
pub async fn reconcile(release: Arc<PhgitRelease>, ctx: Arc<Context>) -> Result<Action, ReleaseError> {
    let ns = release
        .namespace()
        .ok_or_else(|| KubeError::Request(http::Error::new("Missing namespace")))?;
    let releases: Api<PhgitRelease> = Api::namespaced(ctx.client.clone(), &ns);

    // Finalizer management: Add finalizer on creation, handle cleanup on deletion.
    if release.metadata.deletion_timestamp.is_some() {
        if release.metadata.finalizers.as_ref().map_or(false, |f| f.contains(&PHGIT_RELEASE_FINALIZER.to_string())) {
            // Resource is being deleted, run cleanup logic.
            cleanup(&release, &ctx).await?;

            // Remove the finalizer to allow Kubernetes to delete the resource.
            releases.patch_finalizers(
                &release.name_any(),
                &PatchParams::default(),
                Patch::Json(json!([
                    { "op": "remove", "path": "/metadata/finalizers", "value": [PHGIT_RELEASE_FINALIZER] }
                ]))
            ).await?;
        }
        // No need to requeue after successful cleanup.
        Ok(Action::await_change())
    } else {
        // Resource is being created or updated.
        // Ensure finalizer is present.
        if !release.metadata.finalizers.as_ref().map_or(false, |f| f.contains(&PHGIT_RELEASE_FINALIZER.to_string())) {
            releases.patch_finalizers(
                &release.name_any(),
                &PatchParams::default(),
                Patch::Json(json!([
                    { "op": "add", "path": "/metadata/finalizers/-", "value": PHGIT_RELEASE_FINALIZER }
                ]))
            ).await?;
        }

        // Delegate to the appropriate strategy handler.
        let spec = release.spec.as_ref().ok_or(ReleaseError::MissingSpec)?;
        match spec.strategy {
            ReleaseStrategy::Canary => handle_canary_release(&release, &ctx).await?,
            ReleaseStrategy::BlueGreen => handle_blue_green_release(&release, &ctx).await?,
        }

        Ok(Action::requeue(Duration::from_secs(300)))
    }
}

/// Implements the Canary release strategy.
///
/// Deploys a new "canary" version and uses an Istio VirtualService to route a
/// small percentage of traffic to it for evaluation.
async fn handle_canary_release(release: &PhgitRelease, ctx: &Context) -> Result<(), ReleaseError> {
    let client = &ctx.client;
    let ns = release.namespace().unwrap();
    let spec = release.spec.as_ref().ok_or(ReleaseError::MissingSpec)?;

    // Define APIs for Deployments and VirtualServices.
    let deployments: Api<k8s_openapi::api::apps::v1::Deployment> = Api::namespaced(client.clone(), &ns);
    let virtual_services: Api<serde_json::Value> = Api::namespaced(client.clone(), &ns);

    let app_name = &spec.app_name;
    let primary_name = format!("{}-primary", app_name);
    let canary_name = format!("{}-canary", app_name);
    let image = &spec.image;
    let canary_weight = spec.canary_weight.unwrap_or(10); // Default to 10% traffic.

    // 1. Define and apply the canary Deployment.
    // This deployment is a copy of the primary but with the new image and canary labels.
    let canary_deployment = json!({
        "apiVersion": "apps/v1",
        "kind": "Deployment",
        "metadata": {
            "name": canary_name,
            "labels": { "app": app_name, "version": "canary" }
        },
        "spec": {
            "replicas": 1, // Start with a small number of replicas.
            "selector": { "matchLabels": { "app": app_name, "version": "canary" } },
            "template": {
                "metadata": { "labels": { "app": app_name, "version": "canary" } },
                "spec": {
                    "containers": [{
                        "name": app_name,
                        "image": image,
                    }]
                }
            }
        }
    });

    println!("Applying canary deployment: {}", canary_name);
    deployments.patch(&canary_name, &PatchParams::apply("phgit-operator"), &Patch::Apply(&canary_deployment)).await?;

    // 2. Define and apply the Istio VirtualService for traffic splitting.
    // This resource directs traffic to the primary and canary services based on weight.
    let virtual_service = json!({
        "apiVersion": "networking.istio.io/v1alpha3",
        "kind": "VirtualService",
        "metadata": { "name": app_name },
        "spec": {
            "hosts": [app_name],
            "http": [{
                "route": [
                    {
                        "destination": { "host": primary_name, "subset": "v1" },
                        "weight": 100 - canary_weight
                    },
                    {
                        "destination": { "host": canary_name, "subset": "v2" },
                        "weight": canary_weight
                    }
                ]
            }]
        }
    });

    println!("Applying VirtualService for '{}' with {}% traffic to canary.", app_name, canary_weight);
    virtual_services.patch(app_name, &PatchParams::apply("phgit-operator"), &Patch::Apply(&virtual_service)).await
        .map_err(|e| ReleaseError::ApplyError(format!("Failed to apply VirtualService: {}. Is Istio installed?", e)))?;


    // TODO: Add status updates to PhgitRelease CR.
    println!("Canary release for '{}' successfully orchestrated.", app_name);
    Ok(())
}


/// Implements the Blue/Green release strategy.
///
/// Deploys a new "green" version alongside the "blue" version. Promotion is
/// handled by switching the selector on the main service.
async fn handle_blue_green_release(release: &PhgitRelease, ctx: &Context) -> Result<(), ReleaseError> {
    let client = &ctx.client;
    let ns = release.namespace().unwrap();
    let spec = release.spec.as_ref().ok_or(ReleaseError::MissingSpec)?;

    let app_name = &spec.app_name;
    let image = &spec.image;

    let deployments: Api<k8s_openapi::api::apps::v1::Deployment> = Api::namespaced(client.clone(), &ns);
    let services: Api<k8s_openapi::api::core::v1::Service> = Api::namespaced(client.clone(), &ns);

    // 1. Determine which version is currently active ("blue").
    // We inspect the main service's selector to find this.
    let main_service = services.get(app_name).await?;
    let current_version = main_service.spec.as_ref()
        .and_then(|s| s.selector.as_ref())
        .and_then(|sel| sel.get("version"))
        .cloned()
        .unwrap_or_else(|| "blue".to_string()); // Default to blue if not set.

    // 2. The new version will be the other color.
    let new_version = if current_version == "blue" { "green" } else { "blue" };
    let new_deployment_name = format!("{}-{}", app_name, new_version);

    // 3. Define and apply the "green" (new) Deployment.
    let green_deployment = json!({
        "apiVersion": "apps/v1",
        "kind": "Deployment",
        "metadata": {
            "name": new_deployment_name,
            "labels": { "app": app_name, "version": new_version }
        },
        "spec": {
            "replicas": spec.replicas.unwrap_or(2), // Deploy at full scale.
            "selector": { "matchLabels": { "app": app_name, "version": new_version } },
            "template": {
                "metadata": { "labels": { "app": app_name, "version": new_version } },
                "spec": {
                    "containers": [{
                        "name": app_name,
                        "image": image,
                    }]
                }
            }
        }
    });

    println!("Applying '{}' deployment: {}", new_version, new_deployment_name);
    deployments.patch(&new_deployment_name, &PatchParams::apply("phgit-operator"), &Patch::Apply(&green_deployment)).await?;

    // 4. Promotion step: If auto-promotion is enabled, patch the main service.
    if spec.promote.unwrap_or(false) {
        println!("Promoting '{}' version by updating the main service selector.", new_version);
        let service_patch = json!({
            "spec": {
                "selector": {
                    "version": new_version
                }
            }
        });

        services.patch(app_name, &PatchParams::apply("phgit-operator"), &Patch::Merge(&service_patch)).await?;
        println!("Service '{}' now routes traffic to '{}' version.", app_name, new_version);
    } else {
        println!("'{}' deployment is ready. To promote, set 'spec.promote' to true.", new_version);
    }

    // TODO: Add status updates to the PhgitRelease CR.
    Ok(())
}

/// Cleanup function to remove associated resources upon deletion.
async fn cleanup(release: &PhgitRelease, ctx: &Context) -> Result<(), ReleaseError> {
    let client = &ctx.client;
    let ns = release.namespace().unwrap();
    let spec = release.spec.as_ref().ok_or(ReleaseError::MissingSpec)?;

    // For canary, we need to delete the VirtualService.
    if let ReleaseStrategy::Canary = spec.strategy {
        let virtual_services: Api<serde_json::Value> = Api::namespaced(client.clone(), &ns);
        println!("Cleaning up VirtualService for '{}'", spec.app_name);
        if let Err(e) = virtual_services.delete(&spec.app_name, &DeleteParams::default()).await {
            if let KubeError::Api(ae) = &e {
                if ae.code != 404 { // Ignore "Not Found" errors.
                    return Err(e.into());
                }
            }
        }
    }

    // For both strategies, we might want to clean up old deployments.
    // For now, we leave them for manual rollback/inspection.

    println!("Cleanup for PhgitRelease '{}' complete.", release.name_any());
    Ok(())
}


/// Error handling function for the reconciliation loop.
pub fn on_error(release: Arc<PhgitRelease>, error: &ReleaseError, ctx: Arc<Context>) -> Action {
    eprintln!(
        "Reconciliation error for PhgitRelease '{}': {:?}",
        release.name_any(),
        error
    );
    Action::requeue(Duration::from_secs(15))
}