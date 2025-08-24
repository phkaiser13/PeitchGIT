/* Copyright (C) 2025 Pedro Henrique / phkaiser13
* File: src/modules/k8s_preview/src/actions.rs
* This file implements the core business logic for the preview environment
* feature. It orchestrates the calls to the `kube_client` module based on the
* parsed configuration. This separation ensures that the business logic is
* decoupled from the low-level details of Kubernetes API interaction.
* SPDX-License-Identifier: Apache-2.0 */

use crate::config::PreviewConfig;
use crate::kube_client;
use anyhow::{Context, Result};
use std::path::Path;

// A default Time-To-Live (TTL) for preview environments, in hours.
// This can be used by a separate cleanup controller to garbage collect old environments.
const DEFAULT_NAMESPACE_TTL_HOURS: u32 = 24;

/// Handles the logic for creating a new preview environment.
///
/// This involves creating a dedicated namespace and applying the application's
/// Kubernetes manifests to it.
///
/// # Arguments
/// * `config` - A reference to the parsed `PreviewConfig`.
pub async fn handle_create_action(config: &PreviewConfig) -> Result<()> {
    // Standardize namespace naming for easy identification.
    let namespace = format!("pr-{}", config.pr_number);
    println!("Starting 'create' action for namespace: {}", namespace);

    let client = kube_client::initialize_client(config.kubeconfig_path.as_deref())
        .await
        .context("Failed to initialize Kubernetes client")?;

    // 1. Create the namespace.
    kube_client::create_namespace(&client, &namespace, DEFAULT_NAMESPACE_TTL_HOURS)
        .await
        .with_context(|| format!("Failed during namespace creation for '{}'", namespace))?;

    // 2. Apply manifests.
    // In a real scenario, we would first clone `config.git_repo_url` to a temporary
    // directory and pass that path here. For now, we use a dummy path.
    let dummy_repo_path = Path::new("/tmp/dummy_repo");
    kube_client::apply_manifests(&client, &namespace, dummy_repo_path)
        .await
        .with_context(|| format!("Failed during manifest application for '{}'", namespace))?;

    println!(
        "Successfully completed 'create' action for namespace: {}",
        namespace
    );
    Ok(())
}

/// Handles the logic for destroying an existing preview environment.
///
/// This is primarily achieved by deleting the entire namespace associated with the PR.
///
/// # Arguments
/// * `config` - A reference to the parsed `PreviewConfig`.
pub async fn handle_destroy_action(config: &PreviewConfig) -> Result<()> {
    let namespace = format!("pr-{}", config.pr_number);
    println!("Starting 'destroy' action for namespace: {}", namespace);

    let client = kube_client::initialize_client(config.kubeconfig_path.as_deref())
        .await
        .context("Failed to initialize Kubernetes client")?;

    // Delete the entire namespace.
    kube_client::delete_namespace(&client, &namespace)
        .await
        .with_context(|| format!("Failed during namespace deletion for '{}'", namespace))?;

    println!(
        "Successfully completed 'destroy' action for namespace: {}",
        namespace
    );
    Ok(())
}