/*
 * Copyright (C) 2025 Pedro Henrique / phkaiser13
 *
 * File: pipeline_controller.rs
 *
 * This file contains the reconciliation logic for the `PhgitPipeline` custom
 * resource. It acts as a Kubernetes-native CI/CD pipeline runner.
 *
 * Architecture:
 * - The reconciler implements a state machine for pipeline execution. When a
 * `PhgitPipeline` is triggered, the controller will create Kubernetes Jobs
 * for each step in the first stage.
 * - It will then watch the status of these Jobs.
 * - Once all Jobs in a stage are complete, it will proceed to the next stage,
 * creating a new set of Jobs.
 * - If any Job fails, the entire pipeline run is marked as 'Failed'.
 * - If all stages complete successfully, the run is marked as 'Succeeded'.
 * - This controller is the most complex as it manages the full lifecycle of
 * other asynchronous resources (Jobs). It requires careful state management
 * in the `.status` field of the `PhgitPipeline` resource to track progress
 * and avoid re-running completed stages.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

use crate::{crds::PhgitPipeline, Context, Error};
use kube::runtime::controller::Action;
use std::sync::Arc;
use std::time::Duration;
use tracing::{info, warn};

/// The main reconciliation function for the PhgitPipeline controller.
pub async fn reconcile(
    pipeline: Arc<PhgitPipeline>,
    _ctx: Arc<Context>,
) -> Result<Action, Error> {
    info!(
        "Reconciling PhgitPipeline: {}/{}",
        pipeline.namespace().unwrap(),
        pipeline.name_any()
    );

    // TODO: Implement the pipeline execution state machine.
    // This is a high-level overview of the required logic:

    // 1. Check the `pipeline.status.phase`.
    //    - If "Succeeded" or "Failed", do nothing until a new trigger condition is met.
    //    - If "Running", check the status of the currently executing Jobs.
    //    - If "Pending" or empty, start a new pipeline run.

    // 2. To start a new run:
    //    - Update status to "Running" and set a unique `pipelineRunID`.
    //    - Get the first stage from `pipeline.spec.stages`.
    //    - For each `step` in the stage, create a Kubernetes Job resource.
    //      - The Job's Pod spec will use the `image` and `command` from the step definition.
    //      - Add an ownerReference to the Job pointing to the PhgitPipeline, so
    //        the Job is garbage-collected if the pipeline is deleted.

    // 3. To check on a running pipeline:
    //    - List all Jobs that have an ownerReference pointing to this PhgitPipeline
    //      and match the current `pipelineRunID`.
    //    - Check the status of each Job.
    //    - If all Jobs for the current stage have succeeded, move to the next stage
    //      and create its Jobs.
    //    - If any Job has failed, update the pipeline status to "Failed" and stop.
    //    - If all stages are complete, update the status to "Succeeded".

    // Requeue to re-evaluate the pipeline state.
    Ok(Action::requeue(Duration::from_secs(30)))
}

/// Defines the error handling policy for the reconciliation loop.
pub fn error_policy(_pipeline: Arc<PhgitPipeline>, error: &Error, _ctx: Arc<Context>) -> Action {
    warn!("Reconciliation failed: {:?}", error);
    Action::requeue(Duration::from_secs(30))
}