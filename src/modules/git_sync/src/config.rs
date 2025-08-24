/* Copyright (C) 2025 Pedro Henrique / phkaiser13
* File: src/modules/git_sync/src/config.rs
* This file defines the data structures for the `git_sync` module's
* configuration. It uses `serde` to deserialize the JSON configuration string
* passed from the C core into a strongly-typed Rust struct. This ensures
* type safety and input validation at the module's boundary.
* SPDX-License-Identifier: Apache-2.0 */

use serde::Deserialize;

/// Represents the main configuration structure for the Git-aware sync task.
/// An instance of this struct is created by deserializing the JSON configuration
/// string passed from the C core.
#[derive(Deserialize, Debug)]
pub struct SyncConfig {
    /// The clone URL of the Git repository containing the desired state (manifests).
    pub repo_url: String,

    /// The branch of the repository to sync against.
    pub branch: String,

    /// The relative path within the repository where the Kubernetes manifests are located.
    pub manifest_path: String,

    /// An optional path to a kubeconfig file for cluster authentication.
    /// If `None`, the client will infer configuration from the environment.
    pub kubeconfig_path: Option<String>,

    /// If true, the module will only detect and report drift without taking any
    /// modifying actions (like creating a pull request).
    pub dry_run: bool,

    /// If true and drift is detected (and `dry_run` is false), the module will
    /// attempt to create a pull request with the necessary changes to reconcile the state.
    pub create_pr: bool,
}