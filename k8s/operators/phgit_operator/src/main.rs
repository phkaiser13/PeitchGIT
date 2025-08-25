/*
 * Copyright (C) 2025 Pedro Henrique / phkaiser13
 *
 * File: main.rs
 *
 * This file is the main entry point for the ph Kubernetes Operator. It is
 * responsible for setting up and running the controller manager, which in turn
 * hosts and executes the reconciliation loops for all custom resources managed
 * by this operator.
 *
 * Architecture:
 * The program follows the standard `kube-rs` operator structure.
 * 1.  **Initialization**: It begins by initializing a Kubernetes client, which
 * provides the connection to the cluster's API server. Tracing (logging) is
 * also set up to provide structured, observable output.
 * 2.  **CRD Registration**: The `main` function discovers all three Custom
 * Resource Definitions (CRDs) that this operator manages: `phPreview`,
 * `phRelease`, and `phPipeline`. This ensures the operator is aware of the
 * custom APIs it needs to handle.
 * 3.  **Controller Manager**: A `Controller` from `kube-rs` is instantiated for
 * each CRD. The `Controller` is the core component that manages the "watch"
 * and "reconcile" loop.
 * - It watches for any changes (creations, updates, deletions) to its
 * corresponding custom resource.
 * - For each change event, it triggers the associated reconciliation function
 * (e.g., `preview_controller::reconcile`).
 * - It is configured with an error handling function (`on_error`) that is
 * invoked whenever the reconciliation logic returns an error.
 * 4.  **Shared Context**: A shared `Context` object, containing the Kubernetes
 * client, is created. This context is passed down to every reconciliation loop,
 * providing them with the necessary tools to interact with the cluster without
 * needing to re-initialize a client on every event.
 * 5.  **Concurrent Execution**: All three controllers are run concurrently using
 * `tokio::join!`. This allows the operator to handle events for Previews,
 * Releases, and Pipelines simultaneously and independently, making the system
 * highly responsive and scalable.
 *
 * This top-level orchestration ensures that each piece of the operator's logic
 * is properly initialized and executed within the asynchronous `tokio` runtime,
 * forming a complete, production-grade Kubernetes operator.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

use futures::stream::StreamExt;
use kube::runtime::controller::{Action, Controller};
use kube::Client;
use std::sync::Arc;
use tokio;

// Import the CRDs and controller modules.
mod crds;
mod controllers {
    pub mod pipeline_controller;
    pub mod preview_controller;
    pub mod release_controller;
}

// Re-exporting the CRDs for easier access.
use crds::{phPipeline, phPreview, phRelease};

// The shared context struct passed to all controllers.
// It holds a Kubernetes client that can be cloned for each controller.
pub struct Context {
    pub client: Client,
}

/// The main entry point of the operator.
#[tokio::main]
async fn main() -> Result<(), Box<dyn std::error::Error>> {
    // 1. Initialize Kubernetes Client
    // This client is the main interface to the Kubernetes API server.
    let client = Client::try_default().await?;

    // 2. Create APIs for our Custom Resources
    // These APIs are strongly-typed clients for our CRDs, scoped to all namespaces.
    let previews = kube::Api::<phPreview>::all(client.clone());
    let releases = kube::Api::<phRelease>::all(client.clone());
    let pipelines = kube::Api::<phPipeline>::all(client.clone());

    // 3. Create the shared context
    // This context will be shared across all controller loops. Arc is used for
    // safe, concurrent access.
    let context = Arc::new(Context {
        client: client.clone(),
    });

    println!("ph Operator starting...");

    // 4. Set up and run the controllers concurrently
    // We use tokio::join! to run all three controllers in parallel. If any of
    // the controllers exit with an error, the entire operator will shut down.
    tokio::join!(
        // --- Preview Controller ---
        Controller::new(previews, Default::default())
            .run(
                controllers::preview_controller::reconcile,
                controllers::preview_controller::on_error,
                context.clone(),
            )
            .for_each(|res| async move {
                match res {
                    Ok(o) => println!("Reconciled phPreview: {:?}", o),
                    Err(e) => eprintln!("phPreview reconcile error: {}", e),
                }
            }),

        // --- Release Controller ---
        Controller::new(releases, Default::default())
            .run(
                controllers::release_controller::reconcile,
                controllers::release_controller::on_error,
                context.clone(),
            )
            .for_each(|res| async move {
                match res {
                    Ok(o) => println!("Reconciled phRelease: {:?}", o),
                    Err(e) => eprintln!("phRelease reconcile error: {}", e),
                }
            }),

        // --- Pipeline Controller ---
        Controller::new(pipelines, Default::default())
            .run(
                controllers::pipeline_controller::reconcile,
                controllers::pipeline_controller::on_error,
                context.clone(),
            )
            .for_each(|res| async move {
                match res {
                    Ok(o) => println!("Reconciled phPipeline: {:?}", o),
                    Err(e) => eprintln!("phPipeline reconcile error: {}", e),
                }
            })
    );

    println!("ph Operator shutting down.");
    Ok(())
}