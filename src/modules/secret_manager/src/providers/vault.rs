/* Copyright (C) 2025 Pedro Henrique / phkaiser13
* File: src/providers/vault.rs
* This file provides the concrete implementation of the `SecretProvider` trait
* for HashiCorp Vault. It encapsulates all logic for communicating with the
* Vault HTTP API, including client initialization, authentication, and parsing
* the specific structure of Vault's API responses. It is completely self-contained
* and exposes its functionality only through the common trait interface.
* SPDX-License-Identifier: Apache-2.0 */

use super::{ProviderConfig, SecretProvider};
use anyhow::{anyhow, Context, Result};
use async_trait::async_trait;
use reqwest::{Client, Url};
use serde::Deserialize;
use std::collections::BTreeMap;

// --- Configuration ---

/// Configuration specific to the HashiCorp Vault provider.
#[derive(Deserialize, Debug)]
pub struct VaultConfig {
    /// The network address of the Vault server (e.g., "http://127.0.0.1:8200").
    pub address: String,
    /// The Vault token used for authentication.
    pub token: String,
}

// --- API Response Structures ---

/// Represents the top-level structure of a Vault KVv2 secret response.
#[derive(Deserialize, Debug)]
struct VaultResponse {
    data: VaultResponseData,
}

/// Represents the nested `data` field in a Vault KVv2 secret response.
#[derive(Deserialize, Debug)]
struct VaultResponseData {
    data: BTreeMap<String, String>,
}

// --- Provider Implementation ---

/// A `SecretProvider` implementation for fetching secrets from HashiCorp Vault.
pub struct VaultProvider {
    client: Client,
    address: Url,
}

impl VaultProvider {
    /// Creates a new `VaultProvider`.
    ///
    * It initializes a persistent `reqwest` client with the necessary
    /// authentication headers and validates the Vault address URL.
    pub fn new(config: VaultConfig) -> Result<Self> {
        let address = Url::parse(&config.address)
            .with_context(|| format!("Invalid Vault address URL: '{}'", config.address))?;

        let mut headers = reqwest::header::HeaderMap::new();
        headers.insert(
            "X-Vault-Token",
            reqwest::header::HeaderValue::from_str(&config.token)
                .context("Invalid Vault token provided")?,
        );

        let client = Client::builder()
            .default_headers(headers)
            .build()
            .context("Failed to build Vault HTTP client")?;

        Ok(Self { client, address })
    }
}

#[async_trait]
impl SecretProvider for VaultProvider {
    /// Fetches a secret value from a Vault KVv2 engine.
    ///
    /// The `key` is expected to be in the format: `secret/path:key_in_secret`.
    /// For example: `kv/data/my-app/prod:database_password`.
    async fn fetch_secret_value(&self, key: &str) -> Result<String> {
        // Split the input key into the Vault path and the key within the secret.
        let (secret_path, secret_key) = key.split_once(':').ok_or_else(|| {
            anyhow!(
                "Invalid Vault secret key format. Expected 'path/to/secret:key', got '{}'",
                key
            )
        })?;

        // Construct the full API URL. Vault's KVv2 API path is typically mounted
        // under a path that includes `/data/`.
        let api_url = self.address.join(&format!("v1/{}", secret_path))
            .with_context(|| format!("Failed to construct API URL for path '{}'", secret_path))?;

        println!("Fetching secret '{}' from Vault path '{}'...", secret_key, secret_path);

        // Perform the GET request.
        let response = self
            .client
            .get(api_url)
            .send()
            .await
            .context("Failed to send request to Vault API")?;

        // Check for a successful HTTP status code.
        if !response.status().is_success() {
            let status = response.status();
            let body = response.text().await.unwrap_or_else(|_| "Could not read error body".to_string());
            return Err(anyhow!(
                "Vault API returned a non-success status: {}. Body: {}",
                status,
                body
            ));
        }

        // Parse the JSON response and extract the secret value.
        let vault_response = response
            .json::<VaultResponse>()
            .await
            .context("Failed to deserialize Vault API response")?;

        let secret_value = vault_response
            .data
            .data
            .get(secret_key)
            .ok_or_else(|| {
                anyhow!(
                    "Key '{}' not found in secret at path '{}'",
                    secret_key,
                    secret_path
                )
            })?;

        Ok(secret_value.clone())
    }
}