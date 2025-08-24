/* Copyright (C) 2025 Pedro Henrique / phkaiser13
* File: src/modules/release_orchestrator/src/config.rs
* This file defines the data structures for the `release_orchestrator` module's
* configuration. It uses `serde` to deserialize the JSON configuration from the
* C core into strongly-typed Rust enums and structs. This approach provides
* compile-time safety, ensuring that only valid strategies and service meshes
* are processed by the business logic.
* SPDX-License-Identifier: Apache-2.0 */

use serde::Deserialize;

/// Represents the supported release strategies.
/// Using an enum ensures that the logic can only operate on a known set of strategies.
#[derive(Deserialize, Debug, PartialEq, Clone, Copy)]
#[serde(rename_all = "kebab-case")] // Handles "blue-green" and "traffic-shift" correctly.
pub enum ReleaseStrategy {
    Canary,
    BlueGreen,
    TrafficShift,
}

/// Represents the supported service mesh providers.
/// This allows the implementation to be extended to other meshes in the future.
#[derive(Deserialize, Debug, PartialEq, Clone, Copy)]
#[serde(rename_all = "lowercase")]
pub enum ServiceMesh {
    Istio,
    Linkerd,
    Traefik,
}

/// Represents the main configuration structure for a release orchestration task.
/// It is deserialized from the JSON string provided by the C core.
#[derive(Deserialize, Debug)]
pub struct ReleaseConfig {
    /// The deployment strategy to execute.
    pub strategy: ReleaseStrategy,

    /// The target service mesh to interact with.
    pub service_mesh: ServiceMesh,

    /// The name of the application being deployed (e.g., the Deployment/Service name).
    pub app_name: String,

    /// The Kubernetes namespace where the application resides.
    pub namespace: String,

    /// The version identifier (e.g., image tag) of the currently stable deployment.
    pub stable_version: String,

    /// The version identifier (e.g., image tag) of the new version to be released.
    pub new_version: String,

    /// The percentage of traffic to shift to the new version.
    /// This is only applicable for `Canary` and `TrafficShift` strategies.
    /// Using `Option` makes its optionality explicit and safe to handle.
    pub traffic_percent: Option<u8>,
}