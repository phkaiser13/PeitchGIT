/*
 * Copyright (C) 2025 Pedro Henrique / phkaiser13
 *
 * File: pipeline_controller.rs
 *
 * This file implements the reconciliation logic for the PhgitPipeline custom resource.
 * It functions as a state machine that orchestrates a series of jobs within a
 * Kubernetes cluster, effectively creating a simple CI/CD pipeline. The controller
 * processes a PhgitPipeline specification, which defines stages and steps, and
 * executes them sequentially.
 *
 * Architecture:
 * The core of this controller is the `reconcile` function, which is repeatedly
 * called by the kube-rs runtime. This loop checks the current state of the pipeline
 * against the desired state defined in the `PhgitPipeline` spec and its `status` subresource.
 *
 * Core Logic:
 * 1.  **State Management**: The controller is stateless itself. All state is stored
 * in the `status` field of the `PhgitPipeline` resource on the Kubernetes API server.
 * This makes the operator resilient; if it restarts, it can pick up right
 * where it left off by reading the resource's status.
 * 2.  **Initialization**: When a new `PhgitPipeline` is created, the reconciler
 * initializes its status, setting the phase to "Running" and the current
 * stage/step indices to 0.
 * 3.  **Job Orchestration**: For the current step, the controller checks if a
 * corresponding Kubernetes Job has been created.
 * - If not, it creates a new Job based on the step's specification (image,
 * command, args). A crucial part of this is setting an `ownerReference` on
 * the Job, pointing to the `PhgitPipeline`. This ensures that when the
 * `PhgitPipeline` is deleted, Kubernetes automatically garbage-collects
 * all associated Jobs.
 * 4.  **Job Monitoring**: If a Job for the current step already exists, the
 * controller inspects its status.
 * - If the Job is still running (`active` > 0), the reconciler requeues
 * itself to check back later.
 * - If the Job has succeeded (`succeeded` > 0), the controller updates the
 * `PhgitPipeline`'s status to advance to the next step or stage.
 * - If the Job has failed (`failed` > 0), the controller updates the
 * `PhgitPipeline`'s status to "Failed" and stops processing.
 * 5.  **Completion**: Once all steps in all stages have succeeded, the
 * `PhgitPipeline` status is updated to "Succeeded", and a completion timestamp
 * is recorded.
 *
 * This implementation uses `kube-rs` for all interactions with the Kubernetes API,
 * including getting, creating, and patching `PhgitPipeline` and `Job` resources.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

use chrono::Utc;
use k8s_openapi::api::batch::v1::Job;
use kube::{
    api::{Api, ListParams, Patch, PatchParams, PostParams, ResourceExt},
    client::Client,
    runtime::controller::Action,
    Error as KubeError,
};
use serde_json::json;
use std::sync::Arc;
use std::time::Duration;
use thiserror::Error;

use crate::crds::{PhgitPipeline, PhgitPipelineStatus, PipelinePhase};

// Custom error types for the pipeline controller.
#[derive(Debug, Error)]
pub enum PipelineError {
    #[error("Kubernetes API error: {0}")]
    KubeError(#[from] KubeError),

    #[error("Missing PhgitPipeline spec")]
    MissingSpec,
}

/// The context required by the reconciler.
pub struct Context {
    pub client: Client,
}

/// Main reconciliation function for the PhgitPipeline resource.
///
/// This is the heart of the controller, acting as a state machine. It is called
/// whenever there's a change to a PhgitPipeline resource.
pub async fn reconcile(pipeline: Arc<PhgitPipeline>, ctx: Arc<Context>) -> Result<Action, PipelineError> {
    let client = &ctx.client;
    let ns = pipeline.namespace().unwrap();
    let pipelines: Api<PhgitPipeline> = Api::namespaced(client.clone(), &ns);

    // Initialize or retrieve the status.
    let mut status = pipeline.status.clone().unwrap_or_default();
    let spec = pipeline.spec.as_ref().ok_or(PipelineError::MissingSpec)?;

    // Check if the pipeline has already completed or failed.
    if matches!(status.phase, Some(PipelinePhase::Succeeded) | Some(PipelinePhase::Failed)) {
        // The pipeline has finished its lifecycle. No further action is needed.
        return Ok(Action::await_change());
    }

    // Initialize status for a new pipeline.
    if status.phase.is_none() {
        status.phase = Some(PipelinePhase::Running);
        status.start_time = Some(Utc::now().to_rfc3339());
        status.current_stage_index = Some(0);
        status.current_step_index = Some(0);
        update_status(&pipelines, &pipeline.name_any(), status.clone()).await?;
        return Ok(Action::requeue(Duration::from_secs(1))); // Requeue immediately to start processing.
    }

    // Get current stage and step from status.
    let stage_index = status.current_stage_index.unwrap_or(0);
    let step_index = status.current_step_index.unwrap_or(0);

    // Check if all stages are complete.
    if stage_index >= spec.stages.len() {
        status.phase = Some(PipelinePhase::Succeeded);
        status.completion_time = Some(Utc::now().to_rfc3339());
        update_status(&pipelines, &pipeline.name_any(), status).await?;
        println!("Pipeline '{}' completed successfully.", pipeline.name_any());
        return Ok(Action::await_change());
    }

    let current_stage = &spec.stages[stage_index];
    let current_step = &current_stage.steps[step_index];

    // Find or create the Job for the current step.
    let jobs: Api<Job> = Api::namespaced(client.clone(), &ns);
    let job_name = format!(
        "{}-s{}-{}",
        pipeline.name_any(),
        stage_index,
        current_step.name.replace('_', "-")
    );

    match jobs.get(&job_name).await {
        Ok(job) => {
            // Job exists, check its status.
            if let Some(job_status) = job.status {
                if job_status.succeeded.unwrap_or(0) > 0 {
                    // Job succeeded, advance to the next step/stage.
                    println!("Job '{}' succeeded.", job_name);

                    if step_index + 1 >= current_stage.steps.len() {
                        // Move to the next stage.
                        status.current_stage_index = Some(stage_index + 1);
                        status.current_step_index = Some(0);
                    } else {
                        // Move to the next step in the same stage.
                        status.current_step_index = Some(step_index + 1);
                    }
                    update_status(&pipelines, &pipeline.name_any(), status).await?;
                    return Ok(Action::requeue(Duration::from_secs(1))); // Requeue to process next step.

                } else if job_status.failed.unwrap_or(0) > 0 {
                    // Job failed, mark the pipeline as failed.
                    eprintln!("Job '{}' failed.", job_name);
                    status.phase = Some(PipelinePhase::Failed);
                    status.completion_time = Some(Utc::now().to_rfc3339());
                    update_status(&pipelines, &pipeline.name_any(), status).await?;
                    return Ok(Action::await_change()); // Stop reconciliation.

                }
                // Job is still running, requeue to check later.
                println!("Job '{}' is still running.", job_name);
                Ok(Action::requeue(Duration::from_secs(15)))
            } else {
                // Job status is not yet available, check again shortly.
                Ok(Action::requeue(Duration::from_secs(5)))
            }
        }
        Err(KubeError::Api(e)) if e.code == 404 => {
            // Job does not exist, so we create it.
            println!("Creating Job '{}' for pipeline '{}'", job_name, pipeline.name_any());
            let job = create_job_for_step(&pipeline, &job_name, stage_index)?;
            jobs.create(&PostParams::default(), &job).await?;
            Ok(Action::requeue(Duration::from_secs(10))) // Requeue to check job status.
        }
        Err(e) => {
            // Another Kubernetes API error occurred.
            Err(e.into())
        }
    }
}

/// Creates a Kubernetes Job object for a specific pipeline step.
///
/// This function constructs a `k8s_openapi::api::batch::v1::Job` definition from a
/// `PhgitPipeline` resource and a given step. It includes an `ownerReference` so
/// that the Job is garbage-collected when the parent `PhgitPipeline` is deleted.
fn create_job_for_step(pipeline: &PhgitPipeline, job_name: &str, stage_index: usize) -> Result<Job, PipelineError> {
    let spec = pipeline.spec.as_ref().ok_or(PipelineError::MissingSpec)?;
    let status = pipeline.status.as_ref().unwrap(); // Status is guaranteed to be initialized here.
    let step_index = status.current_step_index.unwrap();
    let step = &spec.stages[stage_index].steps[step_index];

    // Define the Job using serde_json for easy construction.
    let job_json = json!({
        "apiVersion": "batch/v1",
        "kind": "Job",
        "metadata": {
            "name": job_name,
            "ownerReferences": [{
                "apiVersion": "phgit.io/v1alpha1",
                "kind": "PhgitPipeline",
                "name": pipeline.name_any(),
                "uid": pipeline.uid().unwrap(),
                "controller": true,
            }]
        },
        "spec": {
            "template": {
                "spec": {
                    "containers": [{
                        "name": step.name,
                        "image": step.image,
                        "command": step.command,
                        "args": step.args,
                    }],
                    "restartPolicy": "Never"
                }
            },
            "backoffLimit": 1 // Do not retry failed jobs.
        }
    });

    // Deserialize the JSON value into a Job object.
    serde_json::from_value(job_json).map_err(|e| PipelineError::KubeError(KubeError::SerdeError(e)))
}

/// Patches the status subresource of the PhgitPipeline.
async fn update_status(pipelines: &Api<PhgitPipeline>, name: &str, status: PhgitPipelineStatus) -> Result<(), PipelineError> {
    let patch = Patch::Merge(json!({
        "status": status
    }));
    pipelines.patch_status(name, &PatchParams::default(), &patch).await?;
    Ok(())
}

/// Error handling function for the reconciliation loop.
pub fn on_error(pipeline: Arc<PhgitPipeline>, error: &PipelineError, ctx: Arc<Context>) -> Action {
    eprintln!(
        "Reconciliation error for PhgitPipeline '{}': {:?}",
        pipeline.name_any(),
        error
    );

    // On error, attempt to update the status to Failed before requeueing.
    let client = ctx.client.clone();
    let ns = pipeline.namespace().unwrap();
    let name = pipeline.name_any();
    let pipelines: Api<PhgitPipeline> = Api::namespaced(client, &ns);

    let mut status = pipeline.status.clone().unwrap_or_default();
    status.phase = Some(PipelinePhase::Failed);
    status.completion_time = Some(Utc::now().to_rfc3339());

    // This is a best-effort update. We launch a separate task to avoid blocking
    // the error handler.
    tokio::spawn(async move {
        if let Err(e) = update_status(&pipelines, &name, status).await {
            eprintln!("Failed to update status on error for '{}': {}", name, e);
        }
    });

    Action::requeue(Duration::from_secs(30))
}