/*
 * Copyright (C) 2025 Pedro Henrique / phkaiser13
 *
 * File: crds.rs
 *
 * This file defines the Rust data structures that correspond to our Custom
 * Resource Definitions (CRDs). By using the `kube::CustomResource` derive macro,
 * we create a strongly-typed representation of our custom APIs.
 *
 * Architecture:
 * - Each struct (e.g., `PhgitPreview`) represents a single Kind in Kubernetes.
 * - The `#[kube(...)]` attribute provides the necessary metadata to map the Rust
 * struct to the corresponding CRD in the cluster (group, version, kind). This
 * metadata MUST exactly match the definitions in the YAML CRD files.
 * - The `Spec` and `Status` inner structs mirror the `spec` and `status` fields
 * of the Kubernetes object. This is a standard pattern that separates the
 * user's desired state (`spec`) from the operator's observed state (`status`).
 * - `serde` attributes (like `#[serde(rename = "prNumber")]`) are used to map
 * between the idiomatic Rust `snake_case` field names and the idiomatic
 * Kubernetes `camelCase` YAML field names.
 * - `schemars` is used to generate an OpenAPI v3 schema, which enables server-side
 * validation by the Kubernetes API server.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

use kube::CustomResource;
use schemars::JsonSchema;
use serde::{Deserialize, Serialize};

/// # PhgitPreview
/// Represents the desired state for an ephemeral preview environment.
/// Creating a `PhgitPreview` resource in a namespace will trigger the operator
/// to provision and deploy a temporary environment based on a pull request.
#[derive(CustomResource, Deserialize, Serialize, Clone, Debug, JsonSchema)]
#[kube(
    group = "phgit.io",
    version = "v1alpha1",
    kind = "PhgitPreview",
    namespaced
)]
#[kube(status = "PhgitPreviewStatus")]
#[kube(shortname = "pgprv")]
pub struct PhgitPreviewSpec {
    #[serde(rename = "prNumber")]
    #[schemars(range(minimum = 1))]
    pub pr_number: u32,
    #[serde(rename = "repoUrl")]
    #[schemars(url)]
    pub repo_url: String,
    #[serde(rename = "commitSha")]
    #[schemars(length(min = 40, max = 40), pattern = "^[0-9a-fA-F]{40}$")]
    pub commit_sha: String,
    #[serde(rename = "ttlHours", default = "default_ttl")]
    #[schemars(range(minimum = 1))]
    pub ttl_hours: u32,
    #[serde(rename = "manifestPath", default = "default_manifest_path")]
    #[schemars(length(min = 1))]
    pub manifest_path: String,
}

// Default value for ttl_hours if not specified in the manifest.
fn default_ttl() -> u32 { 24 }
// Default value for manifest_path if not specified in the manifest.
fn default_manifest_path() -> String { "./k8s".to_string() }

/// The observed state of the PhgitPreview resource, managed by the operator.
#[derive(Deserialize, Serialize, Clone, Debug, JsonSchema)]
pub struct PhgitPreviewStatus {
    pub phase: String,
    pub url: Option<String>,
    #[serde(rename = "expiresAt")]
    pub expires_at: Option<String>,
    pub message: Option<String>,
}

/// # PhgitRelease
/// Represents a declarative release process for an application.
/// Creating a `PhgitRelease` resource will trigger the operator to perform a
/// progressive deployment strategy (e.g., Canary or Blue-Green).
#[derive(CustomResource, Deserialize, Serialize, Clone, Debug, JsonSchema)]
#[kube(
    group = "phgit.io",
    version = "v1alpha1",
    kind = "PhgitRelease",
    namespaced
)]
#[kube(status = "PhgitReleaseStatus")]
#[kube(shortname = "pgrls")]
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

/// # PhgitPipeline
/// Represents a declarative CI/CD pipeline.
/// Creating a `PhgitPipeline` resource defines a pipeline that the operator
/// can trigger and execute based on Git events or manual requests.
#[derive(CustomResource, Deserialize, Serialize, Clone, Debug, JsonSchema)]
#[kube(
    group = "phgit.io",
    version = "v1alpha1",
    kind = "PhgitPipeline",
    namespaced
)]
#[kube(status = "PhgitPipelineStatus")]
#[kube(shortname = "pgpipe")]
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