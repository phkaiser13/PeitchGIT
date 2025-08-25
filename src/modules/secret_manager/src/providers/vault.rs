/*
 * Copyright (C) 2025 Pedro Henrique / phkaiser13
 *
 * File: src/modules/secret_manager/src/providers/vault.rs
 *
 * This file provides a high-level, asynchronous client for interacting with a HashiCorp
 * Vault instance, specifically tailored for reading secrets from Vault's Key/Value (KV)
 * Version 2 secrets engine. The primary goal of this module is to abstract away the
 * complexities of the Vault API, offering a simple and robust interface for other parts
 * of the application to securely fetch secrets.
 *
 * Architecture:
 * The implementation leverages the `rust_vault` crate, which provides a solid foundation
 * for Vault API communication over HTTP. This client is fully asynchronous, built upon
 * `tokio` and `reqwest`, ensuring non-blocking network operations which are critical for
 * performance in I/O-bound tasks.
 *
 * Key components of the architecture:
 * 1. `VaultClient`: A struct that encapsulates the configuration and the underlying
 * `rust_vault` client. It serves as the main entry point for all operations. Its
 * construction is separated from its usage, promoting good configuration management
 * (e.g., loading credentials from environment variables at startup).
 * 2. `VaultConfig`: A simple struct to hold the necessary credentials for connecting
 * to Vault—namely, the server address and the authentication token. This avoids passing
 * loose strings and creates a clear configuration contract.
 * 3. Asynchronous Operations: All functions that perform network I/O are `async`,
 * allowing them to be integrated seamlessly into a Tokio-based runtime without blocking.
 * 4. Focused Responsibility: This module's sole responsibility is to fetch raw secret
 * data from Vault. It does not concern itself with how these secrets are used, such
 * as injecting them into Kubernetes. This adheres to the single-responsibility principle.
 * 5. Robust Error Handling: All public functions return `anyhow::Result`, which provides
 * rich, contextual error messages. This simplifies error handling for the caller and
 * aids in debugging issues related to network connectivity, authentication, or secret
 * path resolution.
 *
 * Process Flow for fetching a secret:
 * - A `VaultClient` is instantiated once with the required `VaultConfig`.
 * - The `get_secret` method is called with the path to the desired secret in Vault
 * (e.g., "secret/data/my-app/config").
 * - The client communicates with the Vault server via its HTTP API.
 * - It specifically targets the KVv2 engine's data structure, parsing the nested JSON
 * response to extract the `data` map which contains the actual secret key-value pairs.
 * - Upon success, it returns a `HashMap<String, String>` of the secret data.
 * - In case of any failure (e.g., 404 Not Found, 403 Forbidden), a descriptive error
 * is returned.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

use anyhow::{Context, Result};
use rust_vault::client::{VaultClient as VaultApiClient, VaultClientSettings};
use rust_vault::kv2;
use std::collections::HashMap;

/// Configuration for the Vault client.
///
/// This struct holds all the necessary information to establish a connection with a
/// Vault server. It is typically constructed from environment variables or a
/// configuration file at application startup.
#[derive(Debug, Clone)]
pub struct VaultConfig {
    /// The network address of the Vault server (e.g., "http://127.0.0.1:8200").
    pub address: String,
    /// The authentication token to be used for all API requests.
    pub token: String,
}

/// An asynchronous client for interacting with HashiCorp Vault.
///
/// This client provides a simplified interface for reading secrets from Vault's
/// KVv2 secrets engine.
#[derive(Debug)]
pub struct VaultClient {
    /// The underlying API client from the `rust_vault` crate.
    api_client: VaultApiClient,
}

impl VaultClient {
    /// Creates a new `VaultClient`.
    ///
    /// # Arguments
    /// * `config` - A `VaultConfig` struct containing the address and token.
    ///
    /// # Returns
    /// A new instance of `VaultClient`.
    pub fn new(config: &VaultConfig) -> Result<Self> {
        // Configure the settings for the underlying Vault API client.
        // This includes setting the server address and the authentication token.
        let settings = VaultClientSettings::builder()
            .address(&config.address)
            .token(&config.token)
            .build()
            .with_context(|| format!("Failed to build Vault client settings for address: {}", config.address))?;

        // Instantiate the underlying client.
        let api_client = VaultApiClient::new(settings)
            .context("Failed to create the underlying Vault API client.")?;

        Ok(VaultClient { api_client })
    }

    /// Fetches a secret from the Vault KVv2 secrets engine.
    ///
    /// This function reads the secret at the specified path and returns the data as a
    /// key-value map. It is designed to work specifically with the KVv2 engine, which
    /// stores the secret data within a nested `data` field in the API response.
    ///
    /// # Arguments
    /// * `path` - The full path to the secret in Vault, including the mount point.
    ///   For example: "secret/data/my-application/database".
    ///
    /// # Returns
    /// A `Result` containing a `HashMap<String, String>` of the secret data if successful.
    /// Returns an error if the secret is not found, if authentication fails, or if there
    /// is a network issue.
    ///
    /// # Example
    /// ```rust,no_run
    /// # use std::collections::HashMap;
    /// # use anyhow::Result;
    /// # use crate::providers::vault::{VaultClient, VaultConfig};
    /// #
    /// #[tokio::main]
    /// async fn main() -> Result<()> {
    ///     let config = VaultConfig {
    ///         address: std::env::var("VAULT_ADDR").unwrap_or_else(|_| "[http://127.0.0.1:8200](http://127.0.0.1:8200)".to_string()),
    ///         token: std::env::var("VAULT_TOKEN").expect("VAULT_TOKEN must be set"),
    ///     };
    ///
    ///     let vault_client = VaultClient::new(&config)?;
    ///     let secret_path = "secret/data/my-app/db";
    ///
    ///     match vault_client.get_secret(secret_path).await {
    ///         Ok(secrets) => {
    ///             if let Some(password) = secrets.get("password") {
    ///                 println!("Successfully fetched password.");
    ///             }
    ///         }
    ///         Err(e) => {
    ///             eprintln!("Error fetching secret '{}': {:?}", secret_path, e);
    ///         }
    ///     }
    ///     Ok(())
    /// }
    /// ```
    pub async fn get_secret(&self, path: &str) -> Result<HashMap<String, String>> {
        println!("Attempting to fetch secret from Vault at path: {}", path);

        // Call the `read` function from the `rust_vault` kv2 module.
        // This function handles the GET request to the Vault API for a KVv2 secret.
        let secret_response = kv2::read(&self.api_client, path)
            .await
            .with_context(|| format!("Failed to read secret from Vault path '{}'. Check connectivity and permissions.", path))?;

        // The KVv2 engine returns a JSON object where the actual secrets are nested
        // under `data.data`. We extract this inner map.
        // If `secret_response.data.data` is None or the structure is unexpected,
        // this will gracefully fail with a helpful error message.
        let secrets = secret_response
            .data
            .data
            .context("The secret data map was not found in the Vault response. This might not be a KVv2 path.")?;

        println!("Successfully fetched {} keys from Vault path: {}", secrets.len(), path);
        Ok(secrets)
    }
}