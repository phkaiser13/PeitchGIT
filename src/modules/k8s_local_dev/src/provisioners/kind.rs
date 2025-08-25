/*
 * Copyright (C) 2025 Pedro Henrique / phkaiser13
 *
 * File: src/modules/k8s_local_dev/src/provisioners/kind.rs
 *
 * This file implements the provisioning logic for 'kind' (Kubernetes in Docker).
 * It acts as a high-level wrapper around the `kind` command-line tool, translating
 * the user's requests from our unified CLI into specific `kind` commands.
 *
 * Architecture:
 * This module exposes two primary public functions: `create_cluster` and `delete_cluster`.
 * Both functions follow a similar pattern:
 * 1. Command Construction: A `tokio::process::Command` is built to execute the `kind` binary.
 * The appropriate subcommand (`create cluster` or `delete cluster`) and arguments
 * (`--name`, `--config`) are appended based on the function's parameters.
 * 2. Execution Delegation: The constructed command is then passed to the robust
 * `common::execute_command` utility function. This delegates the complex tasks of
 * asynchronous execution, I/O streaming, and error handling to the shared module.
 * 3. Prerequisite Checks: The code implicitly relies on the `kind` executable being
 * present in the system's PATH. The `execute_command` function is designed to
 * produce a user-friendly error message if the binary is not found.
 *
 * By abstracting the `kind`-specific commands into this module, we keep the main
 * CLI logic clean and focused on dispatching, while also separating the implementation
 * details of `kind` from other provisioners like `k3s`. This modularity makes it
 * easy to update `kind`-related commands in the future without affecting other parts
 * of the application.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

use super::common::execute_command;
use anyhow::{Context, Result};
use std::path::Path;
use tokio::process::Command;

/// Creates a new Kubernetes cluster using `kind`.
///
/// This function constructs and executes the `kind create cluster` command.
///
/// # Arguments
/// * `name` - The name to assign to the new `kind` cluster.
/// * `config_path` - An optional path to a `kind` configuration YAML file.
///
/// # Returns
/// An `anyhow::Result<()>` indicating the success or failure of the operation.
pub async fn create_cluster(name: &str, config_path: Option<&Path>) -> Result<()> {
    let mut command = Command::new("kind");
    command.arg("create").arg("cluster").arg("--name").arg(name);

    // If a configuration file is provided, add it to the command arguments.
    // The `--config` flag allows for advanced cluster setup, such as multi-node clusters
    // or custom container images.
    if let Some(path) = config_path {
        command.arg("--config").arg(path);
        println!("Using configuration file: {}", path.display());
    }

    execute_command(&mut command)
        .await
        .context("Failed to execute 'kind create cluster' command.")
}

/// Deletes an existing Kubernetes cluster managed by `kind`.
///
/// This function constructs and executes the `kind delete cluster` command.
///
/// # Arguments
/// * `name` - The name of the `kind` cluster to delete.
///
/// # Returns
/// An `anyhow::Result<()>` indicating the success or failure of the operation.
pub async fn delete_cluster(name: &str) -> Result<()> {
    let mut command = Command::new("kind");
    command.arg("delete").arg("cluster").arg("--name").arg(name);

    execute_command(&mut command)
        .await
        .context("Failed to execute 'kind delete cluster' command.")
}