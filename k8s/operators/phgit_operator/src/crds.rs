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
 * `PhgitPreview`) represents a single API Kind in Kubernetes.
 * - The `#[kube(...)]` attribute provides the necessary metadata to map the Rust
 * struct to its corresponding CRD in the cluster (group, version, kind). This
 * metadata MUST exactly match the definitions in the YAML CRD files to ensure
 * correct serialization and deserialization.
 * - The standard Kubernetes object structure is followed by separating the user's
 * desired state (`spec`) from the operator's observed state (`status`).
 * - The `Spec` struct (e.g., `PhgitPreviewSpec`) holds the configuration provided
 * by the user.
 * - The `Status` struct (e.g., `PhgitPreviewStatus`) is managed exclusively by the
 * controller to report the current state of the resource. It is crucial for
 * user feedback and observability.
 * - `serde` attributes are used to map between idiomatic Rust `snake_case` field
 * names and the idiomatic Kubernetes `camelCase` YAML field names.
 * - `schemars` is leveraged to automatically generate an OpenAPI v3 schema from the
 * Rust types. This schema is embedded into the CRD manifest, enabling powerful
 * server-side validation, typed `kubectl` commands, and better client library
 * integration.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

use kube::CustomResource;
use schemars::JsonSchema;
use serde::{Deserialize, Serialize};

// --- PhgitPreview Custom Resource Definition ---

/// # PhgitPreview
/// Represents the desired state for an ephemeral preview environment.
/// Creating a `PhgitPreview` resource will trigger the `preview_controller`
/// to provision and deploy a temporary environment from a Git repository branch.
#[derive(CustomResource, Deserialize, Serialize, Clone, Debug, JsonSchema)]
#[kube(
    group = "phgit.io",
    version = "v1alpha1",
    kind = "PhgitPreview",
    namespaced,
    status = "PhgitPreviewStatus",
    printcolumn = r#"{"name":"Status", "type":"string", "jsonPath":".status.conditions[-1:].type"}"#,
    printcolumn = r#"{"name":"Namespace", "type":"string", "jsonPath":".status.namespace"}"#,
    printcolumn = r#"{"name":"Age", "type":"date", "jsonPath":".metadata.creationTimestamp"}"#,
    shortname = "pgprv"
)]
pub struct PhgitPreviewSpec {
    /// The URL of the Git repository containing the application manifests.
    #[serde(rename = "repoUrl")]
    pub repo_url: String,

    /// The branch, tag, or commit hash to deploy.
    pub branch: String,

    /// The path within the repository where Kubernetes manifests (YAML/YML) are located.
    #[serde(rename = "manifestPath")]
    pub manifest_path: String,

    /// A descriptive name for the application being previewed.
    #[serde(rename = "appName")]
    pub app_name: String,
}

/// The observed state of the PhgitPreview resource, managed by the controller.
/// This status provides crucial feedback to users about the state of their
/// preview environment.
#[derive(Deserialize, Serialize, Clone, Debug, JsonSchema, Default)]
pub struct PhgitPreviewStatus {
    /// The unique namespace created for this preview environment.
    #[serde(skip_serializing_if = "Option::is_none")]
    pub namespace: Option<String>,

    /// A list of conditions providing detailed, time-stamped status updates
    /// throughout the resource's lifecycle (e.g., Creating, Deployed, Failed).
    pub conditions: Vec<StatusCondition>,
}

/// Represents a single condition in the status of a resource.
/// This structure is a common pattern in Kubernetes for detailed status reporting.
#[derive(Deserialize, Serialize, Clone, Debug, JsonSchema)]
pub struct StatusCondition {
    /// The type of the condition (e.g., "Creating", "Deployed", "Terminating", "Failed").
    #[serde(rename = "type")]
    pub type_: String,
    
    /// A human-readable message providing details about the condition.
    pub message: String,
    // Note: In a production-grade operator, you would also include fields like
    // `lastTransitionTime` and `status` ("True", "False", "Unknown").
}

impl StatusCondition {
    /// A helper function to create a new `StatusCondition` instance.
    /// This improves code readability and consistency in the controller logic.
    pub fn new(type_: String, message: String) -> Self {
        Self { type_, message }
    }
}


// --- PhgitRelease Custom Resource Definition ---

/// # PhgitRelease
/// Represents a declarative release process for an application.
/// Creating a `PhgitRelease` resource will trigger the `release_controller`
/// to perform a progressive deployment strategy (e.g., Canary or Blue-Green).
#[derive(CustomResource, Deserialize, Serialize, Clone, Debug, JsonSchema)]
#[kube(
    group = "phgit.io",
    version = "v1alpha1",
    kind = "PhgitRelease",
    namespaced,
    status = "PhgitReleaseStatus",
    shortname = "pgrls"
)]
pub struct PhgitReleaseSpec {
    #[serde(rename = "appName")]
    #[schemars(length(min = 1))]
    pub app_name: String,

    #[schemars(length(min = 1))]
    pub version: String,

    pub strategy: ReleaseStrategy,
}

/// Defines the strategy for the release.
#[derive(Deserialize, Serialize, Clone, Debug, JsonSchema)]
pub struct ReleaseStrategy {
    #[serde(rename = "type")]
    pub strategy_type: StrategyType,
    pub canary: Option<CanaryStrategy>,
    #[serde(rename = "blueGreen")]
    pub blue_green: Option<BlueGreenStrategy>,
}

/// Enum for the different types of release strategies.
#[derive(Deserialize, Serialize, Clone, Debug, JsonSchema)]
#[serde(rename_all = "PascalCase")]
pub enum StrategyType {
    Canary,
    BlueGreen,
}

/// Specific configuration for a Canary release.
#[derive(Deserialize, Serialize, Clone, Debug, JsonSchema)]
pub struct CanaryStrategy {
    #[serde(rename = "trafficPercent")]
    #[schemars(range(minimum = 0, maximum = 100))]
    pub traffic_percent: u8,
    #[serde(rename = "autoIncrement", default)]
    pub auto_increment: bool,
}

/// Specific configuration for a Blue-Green release.
#[derive(Deserialize, Serialize, Clone, Debug, JsonSchema)]
pub struct BlueGreenStrategy {
    #[serde(rename = "autoPromote", default)]
    pub auto_promote: bool,
}

/// The observed state of the PhgitRelease resource.
#[derive(Deserialize, Serialize, Clone, Debug, JsonSchema)]
pub struct PhgitReleaseStatus {
    pub phase: String,
    #[serde(rename = "stableVersion")]
    pub stable_version: Option<String>,
    #[serde(rename = "canaryVersion")]
    pub canary_version: Option<String>,
    #[serde(rename = "trafficSplit")]
    pub traffic_split: Option<String>,
}


// --- PhgitPipeline Custom Resource Definition ---

/// # PhgitPipeline
/// Represents a declarative CI/CD pipeline.
/// Creating a `PhgitPipeline` resource defines a pipeline that the operator
/// can trigger and execute based on Git events or manual requests.
#[derive(CustomResource, Deserialize, Serialize, Clone, Debug, JsonSchema)]
#[kube(
    group = "phgit.io",
    version = "v1alpha1",
    kind = "PhgitPipeline",
    namespaced,
    status = "PhgitPipelineStatus",
    shortname = "pgpipe"
)]
pub struct PhgitPipelineSpec {
    // The definition for PhgitPipelineSpec will go here, mirroring the CRD YAML.
    // This will include triggers, stages, and steps.
}

/// The observed state of the PhgitPipeline resource.
#[derive(Deserialize, Serialize, Clone, Debug, JsonSchema)]
pub struct PhgitPipelineStatus {
    // The definition for PhgitPipelineStatus will go here.
    // This will include the phase, last run details, etc.
}