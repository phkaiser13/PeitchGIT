/*
 * Copyright (C) 2025 Pedro Henrique / phkaiser13
 *
 * File: main.rs
 *
 * This file serves as the main entry point for the phgit-operator binary. Its
 * primary responsibility is to initialize the application environment and launch
 * the reconciliation controllers for each of our Custom Resources (CRDs).
 *
 * Architecture:
 * - It uses the `tokio` runtime's `#[tokio::main]` attribute to set up an
 * asynchronous execution environment.
 * - Initializes a `tracing` subscriber to enable structured logging throughout
 * the operator. Log levels can be controlled via the `RUST_LOG` environment
 * variable (e.g., `RUST_LOG=phgit_operator=info,kube=warn`).
 * - Creates a single, shared Kubernetes `Client` instance. When running inside a
 * pod, `Client::try_default()` will automatically discover and use the in-cluster
 * service account for authentication.
 * - A shared `Context` struct is created and wrapped in an `Arc` (Atomic Reference
 * Counter) to be safely passed to each controller. This is the idiomatic way
 * to provide shared state (like the kube client) to concurrent tasks.
 * - It spawns a separate controller task for each CRD (`PhgitPreview`, `PhgitRelease`,
 * `PhgitPipeline`).
 * - `tokio::join!` is used to run all controller futures concurrently. The operator
 * will run indefinitely until one of the controllers exits with an error.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

// Import necessary modules from our operator crate.
// The `controllers` and `crds` modules will be created in separate files.
mod controllers;
mod crds;

// Import external dependencies.
use futures::StreamExt;
use kube::{
    runtime::{controller::Action, Controller},
    Client, Api, ResourceExt
};
use std::{sync::Arc, time::Duration};
use tracing::info;

// Custom error types for better diagnostics.
#[derive(thiserror::Error, Debug)]
pub enum Error {
    #[error("Kubernetes API error: {0}")]
    KubeError(#[from] kube::Error),
    #[error("Finalizer error: {0}")]
    FinalizerError(#[from] kube::runtime::finalizer::Error<kube::Error>),
}

/// A shared context struct that will be passed to all controllers.
/// This pattern allows sharing state, like the Kubernetes client, database
/// connection pools, or configuration settings, across reconciliation loops.
pub struct Context {
    /// The Kubernetes client.
    client: Client,
}

/// The main entry point of the operator.
#[tokio::main]
async fn main() -> Result<(), Box<dyn std::error::Error>> {
    // Initialize the logging framework. This allows us to see logs from both
    // our operator and the underlying kube-rs library.
    tracing_subscriber::fmt::init();

    // Initialize the Kubernetes client. `try_default` will automatically
    // handle in-cluster and local (kubeconfig) authentication.
    let client = Client::try_default().await?;

    // Create APIs for each of our Custom Resources. The `Api` object is a
    // handle to a specific resource type within a namespace (or cluster-wide).
    let previews = Api::<crds::PhgitPreview>::all(client.clone());
    let releases = Api::<crds::PhgitRelease>::all(client.clone());
    let pipelines = Api::<crds::PhgitPipeline>::all(client.clone());

    // Create the shared context, wrapping it in an Arc for safe concurrent access.
    let context = Arc::new(Context {
        client: client.clone(),
    });

    info!("phgit-operator starting up...");

    // Spawn and run the controllers for each CRD concurrently.
    tokio::join!(
        // Controller for PhgitPreview resources.
        Controller::new(previews, Default::default())
            .run(
                controllers::preview_controller::reconcile,
                controllers::preview_controller::error_policy,
                context.clone()
            )
            .for_each(|res| async move {
                match res {
                    Ok(o) => info!("Reconciled {}/{}", o.0.namespace.unwrap(), o.0.name),
                    Err(e) => tracing::error!("Reconciliation error: {}", e),
                }
            }),

        // Placeholder for the PhgitRelease controller.
        // This demonstrates how multiple controllers are managed.
        Controller::new(releases, Default::default())
            .run(
                controllers::release_controller::reconcile,
                controllers::release_controller::error_policy,
                context.clone()
            )
            .for_each(|res| async move {
                match res {
                    Ok(o) => info!("Reconciled {}/{}", o.0.namespace.unwrap(), o.0.name),
                    Err(e) => tracing::error!("Reconciliation error: {}", e),
                }
            }),

        // Placeholder for the PhgitPipeline controller.
        Controller::new(pipelines, Default::default())
            .run(
                controllers::pipeline_controller::reconcile,
                controllers::pipeline_controller::error_policy,
                context.clone()
            )
            .for_each(|res| async move {
                match res {
                    Ok(o) => info!("Reconciled {}/{}", o.0.namespace.unwrap(), o.0.name),
                    Err(e) => tracing::error!("Reconciliation error: {}", e),
                }
            })
    );

    info!("phgit-operator shutting down.");
    Ok(())
}