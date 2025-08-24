/* Copyright (C) 2025 Pedro Henrique / phkaiser13
* File: src/modules/k8s_preview/src/config.rs
* This file defines the data structures used for configuration within the
* k8s_preview module. It uses the `serde` crate to deserialize a JSON string,
* received from the C core via FFI, into strongly-typed Rust structs. This
* ensures that the input is validated at the boundary, preventing invalid
* data from propagating into the business logic.
* SPDX-License-Identifier: Apache-2.0 */

use serde::Deserialize;

/// Represents the possible actions that can be performed by this module.
/// Using an enum provides compile-time safety, ensuring that only valid
/// actions are processed.
///
/// The `#[serde(rename_all = "snake_case")]` attribute instructs serde to
/// match the JSON string values "create" and "destroy" to the enum variants
/// `Create` and `Destroy` respectively.
#[derive(Deserialize, Debug, PartialEq)]
#[serde(rename_all = "snake_case")]
pub enum Action {
    Create,
    Destroy,
}

/// Represents the main configuration structure for the preview environment task.
/// An instance of this struct is created by deserializing the JSON configuration
/// string passed from the C core.
///
/// The `#[derive(Deserialize)]` macro automatically implements the logic
/// to parse a JSON object into this struct.
#[derive(Deserialize, Debug)]
pub struct PreviewConfig {
    /// The action to be performed, either creating or destroying a preview environment.
    pub action: Action,

    /// The unique identifier for the pull request, used to name and tag resources.
    pub pr_number: u32,

    /// The URL of the git repository to be deployed.
    pub git_repo_url: String,

    /// The specific commit SHA that triggered the action, ensuring reproducibility.
    pub commit_sha: String,

    /// An optional path to a kubeconfig file. If `None`, the client will attempt
    /// to use in-cluster configuration or the default user configuration.
    pub kubeconfig_path: Option<String>,
}