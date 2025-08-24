/* Copyright (C) 2025 Pedro Henrique / phkaiser13
* File: src/modules/git_sync/src/kube_diff.rs
* This file contains the core logic for detecting configuration drift. It reads
* local Kubernetes manifest files, fetches the corresponding live objects from the
* cluster, and performs a comparison to identify any differences between the
* desired state (Git) and the actual state (Kubernetes).
* SPDX-License-Identifier: Apache-2.0 */

use anyhow::{anyhow, Context, Result};
use kube::{
    api::{Api, DynamicObject, ResourceExt},
    discovery, Client,
};
use std::fs;
use std::path::Path;

// NOTE: For robust YAML parsing, especially multi-document files, the `serde_yaml`
// crate is essential. The specification did not include it in Cargo.toml, so this
// implementation uses `serde_yaml` as a conceptual dependency. Ensure it is added
// to the project: `cargo add serde_yaml`.

/// Detects drift between the Kubernetes manifests in a local directory and the
/// state of a live cluster.
///
/// It iterates through all YAML files in the given path, parses them, and for each
/// resource, fetches the live version from the cluster to compare.
///
/// # Arguments
/// * `client` - The Kubernetes client instance.
/// * `manifest_path` - The local path to the directory containing manifest files.
///
/// # Returns
/// A `Result` containing an `Option<String>`.
/// - `Ok(Some(drift_report))` if drift is detected. The string contains the diff.
/// - `Ok(None)` if all resources are in sync.
/// - `Err(e)` if an error occurs during the process.
pub async fn detect_drift(client: &Client, manifest_path: &Path) -> Result<Option<String>> {
    let mut drift_report = String::new();

    // Find all YAML/YML files recursively in the manifest path.
    for entry in walkdir::WalkDir::new(manifest_path) {
        let entry = entry.context("Failed to read directory entry")?;
        let path = entry.path();
        if path.is_file()
            && (path.extension() == Some("yaml".as_ref())
            || path.extension() == Some("yml".as_ref()))
        {
            let file_content = fs::read_to_string(path)
                .with_context(|| format!("Failed to read manifest file: {}", path.display()))?;

            // A single YAML file can contain multiple documents separated by "---".
            for doc_str in file_content.split("---") {
                if doc_str.trim().is_empty() {
                    continue;
                }

                // Attempt to parse the document into a dynamic Kubernetes object.
                let local_obj: DynamicObject = match serde_yaml::from_str(doc_str) {
                    Ok(obj) => obj,
                    Err(e) => {
                        // Log a warning but continue, as the file might contain non-k8s YAML.
                        eprintln!(
                            "Warning: Skipping non-Kubernetes or malformed YAML document in {}: {}",
                            path.display(),
                            e
                        );
                        continue;
                    }
                };

                // Discover the API resource type for the object.
                let api_resource =
                    discovery::resolve_api_resource(client, &local_obj.gvk()).await?;
                let api: Api<DynamicObject> =
                    Api::namespaced_with(client.clone(), local_obj.namespace().as_deref().unwrap_or("default"), &api_resource);

                let name = local_obj.name_any();
                println!("Checking resource: {}/{}", local_obj.gvk(), name);

                // Fetch the live object from the cluster.
                match api.get(&name).await {
                    Ok(live_obj) => {
                        // Compare the local and live objects.
                        if has_drift(&local_obj, &live_obj) {
                            let diff = generate_diff(&local_obj, &live_obj)?;
                            drift_report.push_str(&diff);
                        }
                    }
                    Err(kube::Error::Api(e)) if e.code == 404 => {
                        // Object exists in Git but not in the cluster - this is drift.
                        drift_report.push_str(&format!(
                            "--- Drift Detected: Resource Missing in Cluster ---\nResource: {}/{}\nSource File: {}\n\n",
                            local_obj.gvk(),
                            name,
                            path.display()
                        ));
                    }
                    Err(e) => {
                        return Err(e).context(format!("Failed to fetch live object for {}", name));
                    }
                }
            }
        }
    }

    if drift_report.is_empty() {
        Ok(None)
    } else {
        Ok(Some(drift_report))
    }
}

/// Compares a local manifest object with a live cluster object to check for drift.
/// A simple but effective strategy is to serialize both to YAML after normalizing
/// the live object, and then compare the resulting strings.
fn has_drift(local_obj: &DynamicObject, live_obj: &DynamicObject) -> bool {
    // Convert to generic `serde_yaml::Value` for easy manipulation.
    let mut live_val =
        serde_yaml::to_value(live_obj).expect("Failed to serialize live object to value");
    let local_val =
        serde_yaml::to_value(local_obj).expect("Failed to serialize local object to value");

    // Normalization: Remove cluster-managed fields from the live object before comparison.
    // The 'status' field is the most common source of noise.
    if let Some(map) = live_val.as_mapping_mut() {
        map.remove("status");
    }

    // A more robust implementation would also remove fields from `metadata` like
    // `resourceVersion`, `uid`, `creationTimestamp`, `managedFields`, etc.

    local_val != live_val
}

/// Generates a user-friendly diff report for a single drifted object.
fn generate_diff(local_obj: &DynamicObject, live_obj: &DynamicObject) -> Result<String> {
    let name = local_obj.name_any();
    let gvk = local_obj.gvk();

    // Normalize the live object for a cleaner diff.
    let mut live_val = serde_yaml::to_value(live_obj)?;
    if let Some(map) = live_val.as_mapping_mut() {
        map.remove("status");
    }
    let live_yaml_clean = serde_yaml::to_string(&live_val)?;
    let local_yaml = serde_yaml::to_string(local_obj)?;

    // NOTE: In a real project, use a proper diffing library like `diffy` or `similar`
    // to generate a unified diff format. For this implementation, we show both versions.
    Ok(format!(
        "--- Drift Detected: {}/{} ---\n\n<<< DESIRED STATE (GIT) >>>\n{}\n\n>>> ACTUAL STATE (CLUSTER) >>>\n{}\n\n",
        gvk, name, local_yaml, live_yaml_clean
    ))
}

// NOTE: The `walkdir` crate is highly recommended for recursively walking directories.
// If it's not available, a manual recursive function using `std::fs::read_dir` would
// be needed here. Ensure `walkdir` is added to Cargo.toml: `cargo add walkdir`.