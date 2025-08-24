/* Copyright (C) 2025 Pedro Henrique / phkaiser13
* File: src/modules/k8s_local_dev/src/provisioners/kind.rs
* This file provides the concrete implementation of the `Provisioner` trait for
* 'kind' (Kubernetes in Docker). It translates the abstract methods of the trait
* into specific command-line invocations of the `kind` executable. It handles
* process execution, argument construction, and error reporting for all
* interactions with `kind`.
* SPDX-License-Identifier: Apache-2.0 */

use super::Provisioner;
use anyhow::{anyhow, Context, Result};
use async_trait::async_trait;
use std::process::Stdio;
use tokio::process::Command;

/// Represents the 'kind' provisioner.
/// This is a stateless struct; its logic is contained in the `Provisioner` trait implementation.
pub struct KindProvisioner;

/// A helper function to execute an external command and handle its output.
///
/// # Arguments
/// * `program` - The command to execute (e.g., "kind").
/// * `args` - A slice of string arguments for the command.
async fn run_command(program: &str, args: &[&str]) -> Result<()> {
    println!("> {} {}", program, args.join(" "));

    // Check if the command exists first.
    which::which(program).with_context(|| {
        format!(
            "The required command '{}' was not found in your PATH. Please install it.",
            program
        )
    })?;

    let mut cmd = Command::new(program);
    cmd.args(args)
        .stdout(Stdio::inherit()) // Stream stdout directly to the user's terminal.
        .stderr(Stdio::inherit()); // Stream stderr directly to the user's terminal.

    let status = cmd
        .status()
        .await
        .with_context(|| format!("Failed to execute command: '{}'", program))?;

    if !status.success() {
        return Err(anyhow!(
            "Command '{} {}' failed with exit code: {}",
            program,
            args.join(" "),
            status.code().unwrap_or(1)
        ));
    }

    Ok(())
}

#[async_trait]
impl Provisioner for KindProvisioner {
    /// Creates a 'kind' cluster by running `kind create cluster`.
    async fn create(&self, name: &str, k8s_version: &str) -> Result<()> {
        println!("Creating 'kind' cluster '{}' with Kubernetes v{}...", name, k8s_version);
        let image = format!("kindest/node:v{}", k8s_version);
        run_command(
            "kind",
            &["create", "cluster", "--name", name, "--image", &image],
        )
            .await
            .context("Failed to create 'kind' cluster")
    }

    /// Deletes a 'kind' cluster by running `kind delete cluster`.
    async fn delete(&self, name: &str) -> Result<()> {
        println!("Deleting 'kind' cluster '{}'...", name);
        run_command("kind", &["delete", "cluster", "--name", name])
            .await
            .context("Failed to delete 'kind' cluster")
    }

    /// Lists 'kind' clusters by running `kind get clusters`.
    async fn list(&self) -> Result<()> {
        println!("Listing 'kind' clusters...");
        run_command("kind", &["get", "clusters"])
            .await
            .context("Failed to list 'kind' clusters")
    }
}