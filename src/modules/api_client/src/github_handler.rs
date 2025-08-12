/* Copyright (C) 2025 Pedro Henrique / phkaiser13
* src/github_handler.rs - Business logic for GitHub API interactions.
*
* This file defines a generic ApiProvider trait and its concrete
* implementation for the GitHub API. This architecture decouples the FFI
* bridge from the specifics of any single provider, making the module
* extensible to support others like GitLab or Bitbucket in the future.
*
* It leverages `reqwest` for async HTTP requests and `serde` for efficient
* JSON parsing, which are the idiomatic choices in the Rust ecosystem.
* The error handling uses `Box<dyn std::error::Error>` to be flexible.
*
* SPDX-License-Identifier: Apache-2.0 */

use serde::Deserialize;

// Use our internal logging wrapper from the parent module (lib.rs).
use crate::{log_to_core, LogLevel};

// --- Generic API Provider Contracts ---

/// A generic, provider-agnostic representation of a repository.
/// This struct is public so it can be used by the command logic in `lib.rs`.
#[derive(Debug)]
pub struct RepoInfo {
    pub full_name: String,
    pub description: String,
    pub stars: i64,
    pub forks: i64,
    pub url: String,
    pub default_branch: String,
}

/// Defines the contract for any Git API service.
/// Using an `async_trait` would be an option if we had more complex trait needs,
/// but for now, returning a Future manually is perfectly fine and dependency-free.
/// The trait is public to be implemented and used across modules.
pub trait ApiProvider {
    /// Fetches repository information asynchronously.
    /// Returns a Result containing either the RepoInfo or a dynamic error.
    fn fetch_repo_info(&self, user: &str, repo: &str) -> impl std::future::Future<Output = Result<RepoInfo, Box<dyn std::error::Error>>> + Send;
}

// --- GitHub API Implementation ---

/// Matches the structure of the JSON from the GitHub API.
/// The `#[derive(Deserialize)]` macro automatically generates the code
/// to parse JSON into this struct. `#[serde(rename = "...")]` maps JSON
/// fields to our struct fields.
#[derive(Deserialize, Debug)]
struct GitHubRepoResponse {
    full_name: String,
    // Using `Option<String>` is the idiomatic and safe Rust way to handle
    // JSON fields that can be `null`.
    description: Option<String>,
    #[serde(rename = "stargazers_count")]
    stars: i64,
    #[serde(rename = "forks_count")]
    forks: i64,
    #[serde(rename = "html_url")]
    url: String,
    default_branch: String,
}

const GITHUB_API_ENDPOINT: &str = "https://api.github.com/repos";

/// The concrete implementation for the ApiProvider interface for GitHub.
pub struct GitHubHandler {
    // We use a shared reqwest::Client for connection pooling and performance.
    client: reqwest::Client,
}

impl GitHubHandler {
    /// Creates a new instance of the GitHubHandler.
    /// It's good practice to create the client once and reuse it.
    pub fn new() -> Self {
        Self {
            client: reqwest::Client::new(),
        }
    }
}

impl ApiProvider for GitHubHandler {
    /// Performs the API call to GitHub and maps the response to our generic RepoInfo struct.
    /// This is an `async` function, allowing for non-blocking I/O.
    async fn fetch_repo_info(&self, user: &str, repo: &str) -> Result<RepoInfo, Box<dyn std::error::Error>> {
        let url = format!("{}/{}/{}", GITHUB_API_ENDPOINT, user, repo);
        log_to_core(LogLevel::Debug, &format!("Querying GitHub API: {}", url));

        let response = self.client
            .get(&url)
            // It's crucial to set a User-Agent, as GitHub's API requires it.
            .header("User-Agent", "gitph-rust-client")
            .send()
            .await?;

        if !response.status().is_success() {
            return Err(format!("Repository not found or API error (status: {})", response.status()).into());
        }

        // `json()` is a convenient method from `reqwest` that reads the body
        // and deserializes it into our `GitHubRepoResponse` struct.
        let api_response = response.json::<GitHubRepoResponse>().await?;

        // Map the provider-specific response to our generic struct.
        // `unwrap_or_else` provides a clean way to handle the optional description.
        let repo_info = RepoInfo {
            full_name: api_response.full_name,
            description: api_response.description.unwrap_or_else(|| "(No description provided)".to_string()),
            stars: api_response.stars,
            forks: api_response.forks,
            url: api_response.url,
            default_branch: api_response.default_branch,
        };

        Ok(repo_info)
    }
}

// --- Command Logic ---

/// The main logic function for the "srp" command. It is now provider-agnostic.
/// This function is `async` because it awaits the `fetch_repo_info` call.
pub async fn set_repository(provider: Box<dyn ApiProvider>, args: &[String]) -> Result<(), Box<dyn std::error::Error>> {
    // 1. Validate input
    if args.len() != 1 {
        return Err("Expected exactly one argument: <user/repo>".into());
    }
    let repo_path = &args[0];

    // 2. Parse the input string
    let parts: Vec<&str> = repo_path.splitn(2, '/').collect();
    if parts.len() != 2 || parts[0].is_empty() || parts[1].is_empty() {
        return Err("Invalid format. Please use <user/repo>".into());
    }
    let (user, repo) = (parts[0], parts[1]);

    // 3. Call the API logic via the trait object
    let repo_info = provider.fetch_repo_info(user, repo).await?;

    // 4. Display the results to the user in a clean format.
    println!("--- Repository Found ---");
    println!("Name:          {}", repo_info.full_name);
    println!("Description:   {}", repo_info.description);
    println!("Default Branch: {}", repo_info.default_branch);
    println!("Stars:         {}", repo_info.stars);
    println!("Forks:         {}", repo_info.forks);
    println!("URL:           {}", repo_info.url);
    println!("------------------------");

    log_to_core(LogLevel::Info, &format!("Successfully fetched info for {}", repo_info.full_name));
    Ok(())
}