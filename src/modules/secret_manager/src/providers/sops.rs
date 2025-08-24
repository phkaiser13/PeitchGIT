/* Copyright (C) 2025 Pedro Henrique / phkaiser13
* File: src/providers/sops.rs
* This file provides the concrete implementation of the `SecretProvider` trait
* for Mozilla's SOPS (Secrets OPerationS). It works by invoking the `sops`
* command-line tool to decrypt a specified file in memory, parsing the
* resulting plaintext YAML/JSON, and extracting the requested value. This
* approach securely encapsulates the interaction with an external process.
* SPDX-License-Identifier: Apache-2.0 */

use super::SecretProvider;
use anyhow::{anyhow, Context, Result};
use async_trait::async_trait;
use serde::Deserialize;
use serde_yaml::Value;
use tokio::process::Command;

// --- Configuration ---

/// Configuration for the SOPS provider. Currently a placeholder as SOPS
/// typically relies on environment configuration (e.g., GPG keys, KMS access).
#[derive(Deserialize, Debug)]
pub struct SopsConfig {}

// --- Provider Implementation ---

/// A `SecretProvider` implementation for fetching secrets from SOPS-encrypted files.
pub struct SopsProvider;

impl SopsProvider {
    /// Creates a new `SopsProvider`.
    ///
    /// It performs a critical pre-flight check to ensure the `sops` executable
    /// is available in the system's PATH, preventing runtime failures.
    pub fn new(_config: SopsConfig) -> Result<Self> {
        which::which("sops").context(
            "The 'sops' executable was not found in your PATH. Please install SOPS.",
        )?;
        Ok(Self)
    }
}

/// Helper function to navigate a nested `serde_yaml::Value` using a dot-separated path.
fn find_value_by_path<'a>(mut value: &'a Value, path: &str) -> Option<&'a Value> {
    for key in path.split('.') {
        value = value.get(key)?;
    }
    Some(value)
}

#[async_trait]
impl SecretProvider for SopsProvider {
    /// Fetches a secret value from a SOPS-encrypted file.
    ///
    /// The `key` is expected to be in the format: `path/to/file.enc.yaml:path.to.value`.
    /// For example: `secrets/prod.enc.yaml:database.password`.
    async fn fetch_secret_value(&self, key: &str) -> Result<String> {
        let (file_path, value_path) = key.split_once(':').ok_or_else(|| {
            anyhow!(
                "Invalid SOPS secret key format. Expected 'path/to/file:key.path', got '{}'",
                key
            )
        })?;

        println!("Decrypting SOPS file '{}' to fetch key '{}'...", file_path, value_path);

        // Execute `sops --decrypt <file_path>` and capture its output.
        let output = Command::new("sops")
            .arg("--decrypt")
            .arg(file_path)
            .output()
            .await
            .with_context(|| format!("Failed to execute 'sops' command for file '{}'", file_path))?;

        if !output.status.success() {
            let stderr = String::from_utf8_lossy(&output.stderr);
            return Err(anyhow!(
                "'sops' command failed with exit code {}: {}",
                output.status,
                stderr
            ));
        }

        let decrypted_content = String::from_utf8(output.stdout)
            .context("SOPS output was not valid UTF-8")?;

        // Parse the decrypted YAML into a generic Value.
        let yaml_data: Value = serde_yaml::from_str(&decrypted_content)
            .with_context(|| format!("Failed to parse decrypted YAML from '{}'", file_path))?;

        // Find the specific value within the YAML structure.
        let secret_value = find_value_by_path(&yaml_data, value_path)
            .ok_or_else(|| anyhow!("Key path '{}' not found in file '{}'", value_path, file_path))?;

        // Convert the found value to a string.
        secret_value
            .as_str()
            .map(String::from)
            .ok_or_else(|| anyhow!("Value at path '{}' is not a string", value_path))
    }
}