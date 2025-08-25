/*
 * Copyright (C) 2025 Pedro Henrique / phkaiser13
 *
 * File: crds.rs
 *
 * This file defines the Rust data structures that correspond to our Custom
 * Resource Definitions (CRDs). By using the `kube::CustomResource` derive macro,
 * we create a strongly-typed representation of our custom APIs, enabling safe
 * and idiomatic interaction with the Kubernetes API server.
 *
 * Architecture:
 * - Each top-level struct decorated with `#[derive(CustomResource)]` (e.g.,
 * `phPreview`, `phRelease`, `phPipeline`) represents a single API Kind.
 * - The `#[kube(...)]` attribute provides the necessary metadata to map the Rust
 * struct to its corresponding CRD in the cluster (group, version, kind). This
 * metadata MUST exactly match the definitions in the YAML CRD files.
 * - The standard Kubernetes object structure is followed by separating the user's
 * desired state (`spec`) from the operator's observed state (`status`).
 * - `serde` attributes are used to map between idiomatic Rust `snake_case` and
 * idiomatic Kubernetes `camelCase`.
 * - `schemars` is leveraged to automatically generate an OpenAPI v3 schema from the
 * Rust types, which is embedded into the CRD manifest for server-side validation.
 *
 * This version includes the complete, production-ready definitions for the
 * `phPipeline` resource, which defines a series of stages and steps to be
 * executed by the `pipeline_controller`.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

use kube::CustomResource;
use schemars::JsonSchema;
use serde::{Deserialize, Serialize};

// --- phPreview Custom Resource Definition ---

#[derive(CustomResource, Deserialize, Serialize, Clone, Debug, JsonSchema)]
#[kube(
    group = "ph.io",
    version = "v1alpha1",
    kind = "phPreview",
    namespaced,
    status = "phPreviewStatus",
    printcolumn = r#"{"name":"Status", "type":"string", "jsonPath":".status.conditions[-1:].type"}"#,
    printcolumn = r#"{"name":"Namespace", "type":"string", "jsonPath":".status.namespace"}"#,
    printcolumn = r#"{"name":"Age", "type":"date", "jsonPath":".metadata.creationTimestamp"}"#,
    shortname = "pgprv"
)]
pub struct phPreviewSpec {
    pub repo_url: String,
    pub branch: String,
    pub manifest_path: String,
    pub app_name: String,
}

#[derive(Deserialize, Serialize, Clone, Debug, JsonSchema, Default)]
pub struct phPreviewStatus {
    #[serde(skip_serializing_if = "Option::is_none")]
    pub namespace: Option<String>,
    pub conditions: Vec<StatusCondition>,
}

#[derive(Deserialize, Serialize, Clone, Debug, JsonSchema)]
pub struct StatusCondition {
    #[serde(rename = "type")]
    pub type_: String,
    pub message: String,
}

impl StatusCondition {
    pub fn new(type_: String, message: String) -> Self {
        Self { type_, message }
    }
}


// --- phRelease Custom Resource Definition ---

#[derive(CustomResource, Deserialize, Serialize, Clone, Debug, JsonSchema)]
#[kube(
    group = "ph.io",
    version = "v1alpha1",
    kind = "phRelease",
    namespaced,
    status = "phReleaseStatus",
    shortname = "pgrls"
)]
pub struct phReleaseSpec {
    #[serde(rename = "appName")]
    pub app_name: String,
    pub version: String,
    pub strategy: ReleaseStrategy,
}

#[derive(Deserialize, Serialize, Clone, Debug, JsonSchema)]
pub struct ReleaseStrategy {
    #[serde(rename = "type")]
    pub strategy_type: StrategyType,
    pub canary: Option<CanaryStrategy>,
    #[serde(rename = "blueGreen")]
    pub blue_green: Option<BlueGreenStrategy>,
}

#[derive(Deserialize, Serialize, Clone, Debug, JsonSchema)]
#[serde(rename_all = "PascalCase")]
pub enum StrategyType {
    Canary,
    BlueGreen,
}

#[derive(Deserialize, Serialize, Clone, Debug, JsonSchema)]
pub struct CanaryStrategy {
    #[serde(rename = "trafficPercent")]
    pub traffic_percent: u8,
    #[serde(rename = "autoIncrement", default)]
    pub auto_increment: bool,
}

#[derive(Deserialize, Serialize, Clone, Debug, JsonSchema)]
pub struct BlueGreenStrategy {
    #[serde(rename = "autoPromote", default)]
    pub auto_promote: bool,
}

#[derive(Deserialize, Serialize, Clone, Debug, JsonSchema, Default)]
pub struct phReleaseStatus {
    pub phase: String,
    #[serde(rename = "stableVersion", skip_serializing_if = "Option::is_none")]
    pub stable_version: Option<String>,
    #[serde(rename = "canaryVersion", skip_serializing_if = "Option::is_none")]
    pub canary_version: Option<String>,
    #[serde(rename = "trafficSplit", skip_serializing_if = "Option::is_none")]
    pub traffic_split: Option<String>,
}


// --- phPipeline Custom Resource Definition ---

/// # phPipeline
/// Represents a declarative CI/CD pipeline.
/// Creating a `phPipeline` resource defines a pipeline that the operator
/// can trigger and execute based on Git events or manual requests.
#[derive(CustomResource, Deserialize, Serialize, Clone, Debug, JsonSchema)]
#[kube(
    group = "ph.io",
    version = "v1alpha1",
    kind = "phPipeline",
    namespaced,
    status = "phPipelineStatus",
    printcolumn = r#"{"name":"Status", "type":"string", "jsonPath":".status.phase"}"#,
    printcolumn = r#"{"name":"Age", "type":"date", "jsonPath":".metadata.creationTimestamp"}"#,
    shortname = "pgpipe"
)]
pub struct phPipelineSpec {
    /// A list of stages to be executed sequentially.
    pub stages: Vec<PipelineStage>,
}

/// A single stage in the pipeline, containing one or more steps.
#[derive(Deserialize, Serialize, Clone, Debug, JsonSchema)]
pub struct PipelineStage {
    /// The name of the stage (e.g., "build", "test", "deploy").
    pub name: String,
    /// The steps to be executed within this stage.
    pub steps: Vec<PipelineStep>,
}

/// A single step in a pipeline stage, corresponding to a Kubernetes Job.
#[derive(Deserialize, Serialize, Clone, Debug, JsonSchema)]
pub struct PipelineStep {
    /// The name of the step.
    pub name: String,
    /// The container image to run for this step.
    pub image: String,
    /// The command to execute. If not provided, the image's entrypoint is used.
    #[serde(default, skip_serializing_if = "Vec::is_empty")]
    pub command: Vec<String>,
    /// The arguments to pass to the command.
    #[serde(default, skip_serializing_if = "Vec::is_empty")]
    pub args: Vec<String>,
}

/// The observed state of the phPipeline resource.
#[derive(Deserialize, Serialize, Clone, Debug, JsonSchema, Default)]
pub struct phPipelineStatus {
    /// The current phase of the pipeline.
    #[serde(skip_serializing_if = "Option::is_none")]
    pub phase: Option<PipelinePhase>,
    /// The timestamp when the pipeline started execution.
    #[serde(rename = "startTime", skip_serializing_if = "Option::is_none")]
    pub start_time: Option<String>,
    /// The timestamp when the pipeline completed or failed.
    #[serde(rename = "completionTime", skip_serializing_if = "Option::is_none")]
    pub completion_time: Option<String>,
    /// The index of the current stage being executed.
    #[serde(rename = "currentStageIndex", skip_serializing_if = "Option::is_none")]
    pub current_stage_index: Option<usize>,
    /// The index of the current step being executed within the stage.
    #[serde(rename = "currentStepIndex", skip_serializing_if = "Option::is_none")]
    pub current_step_index: Option<usize>,
}

/// An enum representing the possible phases of a pipeline's lifecycle.
#[derive(Deserialize, Serialize, Clone, Debug, JsonSchema, PartialEq)]
#[serde(rename_all = "PascalCase")]
pub enum PipelinePhase {
    Pending,
    Running,
    Succeeded,
    Failed,
}