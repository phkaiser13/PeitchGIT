/* Copyright (C) 2025 Pedro Henrique / phkaiser13
* File: src/modules/multi_cluster_orchestrator/src/cluster_manager.rs
* This file contains the core logic for managing and interacting with a fleet
* of Kubernetes clusters. It defines the data structures for configuration,
* a `ClusterManager` responsible for initializing clients, and the logic for
* executing actions concurrently across the target clusters. This module is
* completely decoupled from the FFI layer, focusing solely on the business
* logic of multi-cluster operations.
* SPDX-License-Identifier: Apache-2.0 */

use anyhow::{anyhow, Context, Result};
use futures::future::join_all;
use kube::{
    api::{Api, DynamicObject, Patch, PatchParams, ResourceExt},
    discovery, Client, Config,
};
use serde::Deserialize;
use std::collections::BTreeMap;
use std::sync::Arc;

// --- Configuration Structures ---

/// Defines an action to be performed across clusters.
#[derive(Deserialize, Debug, Clone)]
#[serde(tag = "type", rename_all = "snake_case")]
pub enum Action {
    /// Apply a set of Kubernetes manifests.
    Apply {
        /// A string containing one or more YAML documents.
        manifests: String,
    },
    /// Get the status of a specific resource.
    Status {
        kind: String,
        name: String,
        namespace: Option<String>,
    },
}

/// Defines a target cluster for an operation.
#[derive(Deserialize, Debug)]
pub struct ClusterTarget {
    /// The logical name of the cluster, must match a key in `cluster_configs`.
    pub name: String,
}

/// Top-level configuration structure deserialized from the FFI JSON input.
#[derive(Deserialize, Debug)]
pub struct MultiClusterConfig {
    /// A map from logical cluster name to its kubeconfig file path.
    pub cluster_configs: BTreeMap<String, String>,
    /// The list of clusters to target for this specific operation.
    pub targets: Vec<ClusterTarget>,
    /// The action to perform on the target clusters.
    pub action: Action,
}

// --- Result Reporting Structure ---

/// Represents the outcome of an operation on a single cluster.
#[derive(Debug)]
pub struct ClusterResult {
    pub cluster_name: String,
    /// `Ok` contains a success message, `Err` contains the error details.
    pub outcome: std::result::Result<String, String>,
}

// --- Core Logic ---

/// Manages a collection of Kubernetes clients and orchestrates actions upon them.
pub struct ClusterManager {
    /// A map of cluster names to their initialized and ready-to-use Kubernetes clients.
    clients: BTreeMap<String, Client>,
}

impl ClusterManager {
    /// Creates a new `ClusterManager` by initializing clients for all provided cluster configs.
    /// This is an async operation as it involves I/O to read kubeconfig files and
    /// potentially initial communication with the clusters.
    pub async fn new(configs: &BTreeMap<String, String>) -> Result<Self> {
        let client_futures = configs.iter().map(|(name, path)| async move {
            let config = Config::from_kubeconfig(&kube::config::Kubeconfig::read_from(path)?)
                .await
                .with_context(|| {
                    format!(
                        "Failed to load kubeconfig for cluster '{}' from path '{}'",
                        name, path
                    )
                })?;
            let client = Client::try_from(config).with_context(|| {
                format!("Failed to create Kubernetes client for cluster '{}'", name)
            })?;
            Ok((name.clone(), client))
        });

        let results: Vec<Result<(String, Client)>> = join_all(client_futures).await;
        let mut clients = BTreeMap::new();
        for result in results {
            let (name, client) = result?;
            clients.insert(name, client);
        }

        Ok(Self { clients })
    }

    /// Executes a given action concurrently across a list of target clusters.
    ///
    /// It spawns a Tokio task for each target, ensuring maximum parallelism. It then
    /// collects and returns a `ClusterResult` for each, providing a detailed report.
    pub async fn execute_action(
        &self,
        targets: &[ClusterTarget],
        action: &Action,
    ) -> Vec<ClusterResult> {
        let action = Arc::new(action.clone());
        let task_handles = targets.iter().map(|target| {
            let client = match self.clients.get(&target.name) {
                Some(c) => c.clone(),
                None => {
                    // If a target cluster wasn't in the initial config, return an error immediately.
                    return tokio::spawn(async move {
                        ClusterResult {
                            cluster_name: target.name.clone(),
                            outcome: Err(format!(
                                "Configuration for target cluster '{}' not found.",
                                target.name
                            )),
                        }
                    });
                }
            };
            let cluster_name = target.name.clone();
            let action_clone = Arc::clone(&action);

            // Spawn a dedicated task for each cluster operation.
            tokio::spawn(async move {
                let outcome = match action_clone.as_ref() {
                    Action::Apply { manifests } => {
                        Self::execute_apply(client, manifests).await
                    }
                    Action::Status { kind, name, namespace } => {
                        Self::execute_status(client, kind, name, namespace.as_deref()).await
                    }
                };
                ClusterResult { cluster_name, outcome: outcome.map_err(|e| format!("{:?}", e)) }
            })
        }).collect::<Vec<_>>();

        join_all(task_handles)
            .await
            .into_iter()
            .map(|res| res.expect("Tokio task panicked!"))
            .collect()
    }

    /// Private helper to apply manifests to a single cluster.
    async fn execute_apply(client: Client, manifests: &str) -> Result<String> {
        let ssapply = PatchParams::apply("peitchgit.multi_cluster_orchestrator");
        let mut applied_resources = Vec::new();

        for doc in serde_yaml::Deserializer::from_str(manifests) {
            let obj: DynamicObject = serde::Deserialize::deserialize(doc)
                .context("Failed to deserialize YAML manifest into a Kubernetes object")?;

            let gvk = obj.gvk();
            let name = obj.name_any();
            let namespace = obj.namespace();

            let api_resource = discovery::resolve_api_resource(&client, &gvk).await?;
            let api: Api<DynamicObject> = Api::namespaced_with(client.clone(), namespace.as_deref().unwrap_or("default"), &api_resource);

            api.patch(&name, &ssapply, &Patch::Apply(&obj)).await?;
            applied_resources.push(format!("{}/{}", gvk, name));
        }

        if applied_resources.is_empty() {
            return Err(anyhow!("No valid Kubernetes resources found in manifests."));
        }

        Ok(format!("Successfully applied: {}", applied_resources.join(", ")))
    }

    /// Private helper to get a resource's status from a single cluster.
    async fn execute_status(client: Client, kind: &str, name: &str, namespace: Option<&str>) -> Result<String> {
        // This is a simplified status check. A real implementation would be more detailed.
        let api: Api<DynamicObject> = if let Some(ns) = namespace {
            Api::namespaced(client, ns)
        } else {
            Api::all(client)
        };

        match api.get_status(name).await {
            Ok(status_obj) => {
                let status_yaml = serde_yaml::to_string(&status_obj.status)?;
                Ok(format!("Resource '{}/{}' found. Status:\n{}", kind, name, status_yaml))
            },
            Err(e) => Err(e).context(format!("Failed to get status for '{}/{}'", kind, name)),
        }
    }
}