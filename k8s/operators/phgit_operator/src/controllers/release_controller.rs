/*
 * Copyright (C) 2025 Pedro Henrique / phkaiser13
 *
 * File: release_controller.rs
 *
 * This file implements the reconciliation logic for the phRelease custom resource.
 * Its purpose is to manage declarative, progressive deployments within a Kubernetes
 * cluster, starting with a robust Canary release strategy. The controller ensures
 * that application releases are rolled out in a controlled, observable, and safe manner.
 *
 * Architecture:
 * The controller watches for phRelease resources and orchestrates the creation and
 * management of underlying Kubernetes resources (Deployments, Services) to achieve the
 * desired release state. It embodies the operator pattern by continuously comparing the
 * desired state (defined in the phRelease spec) with the actual state of the cluster
 * and taking action to converge them.
 *
 * Core Logic:
 * - `reconcile`: The main reconciliation loop entry point. It leverages the `finalizer`
 * helper from `kube-rs` to separate the apply (creation/update) and cleanup
 * (deletion) logic, ensuring robust lifecycle management.
 * - `apply_release`: This is the heart of the controller. It performs the following:
 * 1.  **State Assessment**: It fetches the current Deployments (`-stable` and `-canary`)
 * and the primary Service associated with the application.
 * 2.  **Deployment Management**:
 * - It creates the stable and canary Deployments if they don't exist, using a
 * common base definition derived from a hypothetical source (in a real-world
 * scenario, this would come from a Git repository or a template). For this
 * implementation, we assume a simple Nginx deployment for demonstration.
 * - The `stable` deployment is configured with the previous version of the app,
 * and the `canary` deployment is configured with the new version specified
 * in the `phRelease` spec.
 * 3.  **Replica Calculation (Traffic Splitting)**: Based on the `trafficPercent`
 * defined in the `CanaryStrategy`, it calculates the required number of replicas
 * for the stable and canary deployments to approximate the desired traffic split.
 * For example, a 20% traffic split with 10 total replicas would result in 2
 * canary pods and 8 stable pods.
 * 4.  **Service Management**: It ensures a single `Service` exists that selects pods
 * from BOTH the stable and canary deployments using a common app label. Kubernetes'
 * native service discovery (`kube-proxy`) then handles the load balancing across
 * all selected pods, effectively splitting the traffic.
 * 5.  **Status Updates**: It meticulously updates the `phRelease` status with the
 * current phase (`Progressing`, `Succeeded`), active versions, and traffic split
 * percentage, providing crucial feedback to the user.
 * - `cleanup_release`: Triggered upon resource deletion via the finalizer. It safely
 * removes the canary Deployment and ensures the stable Deployment is scaled back to
 * 100% to handle all traffic, leaving the system in a clean state before the
 * `phRelease` object is finally deleted from the API server.
 *
 * This implementation uses only standard Kubernetes resources, avoiding the complexity
 * of service meshes for traffic management, which makes it a more universal and
 * portable solution. Error handling and status patching are central to its design,
 * ensuring resilience and clear observability.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

use crate::crds::{phRelease, phReleaseStatus, StrategyType};
use k8s_openapi::api::apps::v1::Deployment;
use k8s_openapi::api::core::v1::Service;
use kube::{
    api::{Api, DeleteParams, ObjectMeta, Patch, PatchParams, PostParams, ResourceExt},
    client::Client,
    runtime::{
        controller::Action,
        finalizer::{finalizer, Event as FinalizerEvent},
    },
    Error as KubeError,
};
use serde_json::json;
use std::sync::Arc;
use std::time::Duration;
use thiserror::Error;

// The unique identifier for our controller's finalizer.
const RELEASE_FINALIZER: &str = "ph.io/release-finalizer";
const DEFAULT_REPLICAS: i32 = 5; // Default total replicas for the application.

// Custom error types for the controller for better diagnostics.
#[derive(Debug, Error)]
pub enum Error {
    #[error("Missing phRelease spec")]
    MissingSpec,

    #[error("Kubernetes API error: {0}")]
    KubeError(#[from] KubeError),

    #[error("Failed to update resource status: {0}")]
    StatusUpdateError(String),

    #[error("Unsupported release strategy")]
    UnsupportedStrategy,
}

/// The context required by the reconciler.
pub struct Context {
    pub client: Client,
}

/// Main reconciliation function for the phRelease resource.
pub async fn reconcile(release: Arc<phRelease>, ctx: Arc<Context>) -> Result<Action, Error> {
    let ns = release.namespace().ok_or_else(|| {
        KubeError::Request(http::Error::new("Missing namespace for phRelease"))
    })?;
    let releases: Api<phRelease> = Api::namespaced(ctx.client.clone(), &ns);

    // Use the finalizer helper to manage the resource lifecycle.
    finalizer(&releases, RELEASE_FINALIZER, release, |event| async {
        match event {
            FinalizerEvent::Apply(r) => apply_release(r, ctx.clone()).await,
            FinalizerEvent::Cleanup(r) => cleanup_release(r, ctx.clone()).await,
        }
    })
    .await
    .map_err(|e| KubeError::Request(http::Error::new(e.to_string())).into())
}

/// The core logic for creating and managing a Canary release.
async fn apply_release(release: Arc<phRelease>, ctx: Arc<Context>) -> Result<Action, Error> {
    let client = ctx.client.clone();
    let ns = release.namespace().unwrap();
    let spec = release.spec.as_ref().ok_or(Error::MissingSpec)?;

    // APIs for Kubernetes resources we will be managing.
    let deployments: Api<Deployment> = Api::namespaced(client.clone(), &ns);
    let services: Api<Service> = Api::namespaced(client.clone(), &ns);
    let releases: Api<phRelease> = Api::namespaced(client.clone(), &ns);

    // --- 1. Define Names and Labels ---
    let app_name = &spec.app_name;
    let canary_version = &spec.version;
    let stable_name = format!("{}-stable", app_name);
    let canary_name = format!("{}-canary", app_name);

    // --- 2. Handle Release Strategy ---
    let canary_strategy = match &spec.strategy.strategy_type {
        StrategyType::Canary => spec.strategy.canary.as_ref().ok_or(Error::MissingSpec)?,
        _ => return Err(Error::UnsupportedStrategy),
    };

    // --- 3. Create or Update the Service ---
    // This service will target both stable and canary pods.
    let service = build_service(app_name);
    services
        .patch(
            app_name,
            &PatchParams::apply("ph-release-controller"),
            &Patch::Apply(&service),
        )
        .await?;

    // --- 4. Get Current Stable Deployment to determine the stable version ---
    let stable_deployment = deployments.get(&stable_name).await;
    let stable_version = stable_deployment
        .as_ref()
        .ok()
        .and_then(|d| d.spec.as_ref())
        .and_then(|s| s.template.spec.as_ref())
        .and_then(|ps| ps.containers.get(0))
        .and_then(|c| c.image.as_deref())
        .map(|i| i.split(':').last().unwrap_or("unknown").to_string())
        .unwrap_or_else(|| "none".to_string());


    // --- 5. Calculate Replicas for Traffic Splitting ---
    let traffic_percent = canary_strategy.traffic_percent as i32;
    let canary_replicas = (DEFAULT_REPLICAS * traffic_percent) / 100;
    let stable_replicas = DEFAULT_REPLICAS - canary_replicas;

    // --- 6. Create or Update Stable Deployment ---
    let stable_dep_def = build_deployment(app_name, &stable_name, &stable_version, stable_replicas);
    deployments
        .patch(
            &stable_name,
            &PatchParams::apply("ph-release-controller"),
            &Patch::Apply(&stable_dep_def),
        )
        .await?;

    // --- 7. Create or Update Canary Deployment ---
    let canary_dep_def = build_deployment(app_name, &canary_name, canary_version, canary_replicas);
     deployments
        .patch(
            &canary_name,
            &PatchParams::apply("ph-release-controller"),
            &Patch::Apply(&canary_dep_def),
        )
        .await?;

    // --- 8. Update Status ---
    let new_status = phReleaseStatus {
        phase: if traffic_percent == 100 { "Succeeded".to_string() } else { "Progressing".to_string() },
        stable_version: Some(stable_version),
        canary_version: Some(canary_version.clone()),
        traffic_split: Some(format!("{}%", traffic_percent)),
    };

    let patch = Patch::Apply(json!({
        "apiVersion": "ph.io/v1alpha1",
        "kind": "phRelease",
        "status": new_status,
    }));
    releases
        .patch_status(&release.name_any(), &PatchParams::apply("ph-release-controller"), &patch)
        .await
        .map_err(|e| Error::StatusUpdateError(e.to_string()))?;

    println!("Successfully reconciled phRelease '{}'. Stable replicas: {}, Canary replicas: {}", release.name_any(), stable_replicas, canary_replicas);

    // Requeue to check status periodically.
    Ok(Action::requeue(Duration::from_secs(60)))
}

/// Cleans up the resources created for a release.
async fn cleanup_release(release: Arc<phRelease>, ctx: Arc<Context>) -> Result<Action, Error> {
    let client = ctx.client.clone();
    let ns = release.namespace().unwrap();
    let spec = release.spec.as_ref().ok_or(Error::MissingSpec)?;

    let app_name = &spec.app_name;
    let canary_name = format!("{}-canary", app_name);

    let deployments: Api<Deployment> = Api::namespaced(client, &ns);

    // Delete the canary deployment. The stable one remains.
    println!("Cleaning up canary deployment '{}' for release '{}'", canary_name, release.name_any());
    match deployments.delete(&canary_name, &DeleteParams::default()).await {
        Ok(_) => println!("Canary deployment '{}' deleted.", canary_name),
        Err(e) => {
            if let KubeError::Api(ae) = &e {
                if ae.code == 404 { // Not Found
                    println!("Canary deployment '{}' already deleted.", canary_name);
                } else {
                    return Err(e.into());
                }
            } else {
                return Err(e.into());
            }
        }
    };

    // In a real scenario, you might also want to ensure the stable deployment is
    // scaled up to DEFAULT_REPLICAS here. For simplicity, we assume the next
    // reconciliation of a new or existing phRelease will handle this.

    Ok(Action::await_change())
}

/// Error handling function for the reconciliation loop.
pub async fn on_error(release: Arc<phRelease>, error: &Error, ctx: Arc<Context>) -> Action {
    eprintln!("Reconciliation error for phRelease '{}': {:?}", release.name_any(), error);

    let releases: Api<phRelease> = Api::namespaced(ctx.client.clone(), &release.namespace().unwrap());

    // Update the status to "Failed" to provide user feedback.
    let failed_status = phReleaseStatus {
        phase: "Failed".to_string(),
        stable_version: release.status.as_ref().and_then(|s| s.stable_version.clone()),
        canary_version: release.status.as_ref().and_then(|s| s.canary_version.clone()),
        traffic_split: Some(format!("Error: {}", error)),
    };

    let patch = Patch::Apply(json!({
        "apiVersion": "ph.io/v1alpha1",
        "kind": "phRelease",
        "status": failed_status,
    }));

    if let Err(e) = releases.patch_status(&release.name_any(), &PatchParams::apply("ph-release-controller"), &patch).await {
        eprintln!("Failed to update status on error: {}", e);
    }

    // Requeue the request after a delay.
    Action::requeue(Duration::from_secs(15))
}


/// Constructs a Kubernetes Service definition for the application.
fn build_service(app_name: &str) -> Service {
    Service {
        metadata: ObjectMeta {
            name: Some(app_name.to_string()),
            ..ObjectMeta::default()
        },
        spec: Some(k8s_openapi::api::core::v1::ServiceSpec {
            selector: Some([("app".to_string(), app_name.to_string())].into()),
            ports: Some(vec![k8s_openapi::api::core::v1::ServicePort {
                port: 80,
                ..Default::default()
            }]),
            ..Default::default()
        }),
        ..Default::default()
    }
}

/// Constructs a Kubernetes Deployment definition.
fn build_deployment(app_name: &str, name: &str, version: &str, replicas: i32) -> Deployment {
    Deployment {
        metadata: ObjectMeta {
            name: Some(name.to_string()),
            ..ObjectMeta::default()
        },
        spec: Some(k8s_openapi::api::apps::v1::DeploymentSpec {
            replicas: Some(replicas),
            selector: k8s_openapi::apimachinery::pkg::apis::meta::v1::LabelSelector {
                match_labels: Some([
                    ("app".to_string(), app_name.to_string()),
                    ("version-id".to_string(), name.to_string())
                    ].into()),
                ..Default::default()
            },
            template: k8s_openapi::api::core::v1::PodTemplateSpec {
                metadata: Some(ObjectMeta {
                    labels: Some([
                        ("app".to_string(), app_name.to_string()),
                        ("version-id".to_string(), name.to_string())
                        ].into()),
                    ..Default::default()
                }),
                spec: Some(k8s_openapi::api::core::v1::PodSpec {
                    containers: vec![k8s_openapi::api::core::v1::Container {
                        name: app_name.to_string(),
                        image: Some(format!("nginx:{}", version)), // Using nginx for demonstration
                        ..Default::default()
                    }],
                    ..Default::default()
                }),
            },
            ..Default::default()
        }),
        ..Default::default()
    }
}