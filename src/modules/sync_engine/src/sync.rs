/* Copyright (C) 2025 Pedro Henrique / phkaiser13
 *
 * sync.rs - Core state machine and logic for the synchronization engine.
 *
 * This file contains the high-stakes logic for performing a stateful,
 * bi-directional repository synchronization. It uses a `SyncEngine` struct
 * to manage the entire lifecycle of a sync operation, from loading persistent
 * state to analyzing commit graphs and applying changes.
 *
 * The engine leverages the `git2-rs` library for deep, programmatic access
 * to the Git object database, allowing for sophisticated analysis that would
 * be impossible by just wrapping the `git` CLI.
 *
 * SPDX-License-Identifier: Apache-2.0 */

use git2::{Oid, Repository};
use serde::{Deserialize, Serialize};
use std::path::{Path, PathBuf};

// A custom result type for our sync logic to ensure clean error handling.
type SyncResult<T> = Result<T, Box<dyn std::error::Error + Send + Sync>>;

// --- 1. The Persistent State Model ---
// This struct is serialized to/from JSON to keep track of the last known
// synchronized commit hashes. This is critical for finding the correct
// common ancestor in subsequent runs, especially if branches are rebased.
#[derive(Serialize, Deserialize, Debug, Default)]
struct SyncState {
    last_source_synced_oid: Option<String>,
    last_target_synced_oid: Option<String>,
}

// --- 2. The Sync Engine ---
// Encapsulates all resources and state required for a sync operation.
struct SyncEngine {
    // The source repository for the synchronization.
    source_repo: Repository,
    // The target repository for the synchronization.
    target_repo: Repository,
    // The fully-qualified path to the state file. Stored to avoid recalculation.
    state_path: PathBuf,
    // The in-memory representation of the last synchronized state.
    state: SyncState,
}

impl SyncEngine {
    /// Creates a new SyncEngine instance.
    ///
    /// It opens the git repositories and loads the persistent synchronization
    /// state from a file within the source repository's `.git` directory.
    /// If no state file is found, it initializes with a default, empty state.
    fn new(source_path: &str, target_path: &str) -> SyncResult<Self> {
        let source_repo = Repository::open(source_path)?;
        let target_repo = Repository::open(target_path)?;

        // State is stored in a hidden file to avoid cluttering the user's working directory.
        let state_path = Path::new(source_path)
            .join(".git")
            .join("gitph_sync_state.json");

        let state = if state_path.exists() {
            let state_json = std::fs::read_to_string(&state_path)?;
            serde_json::from_str(&state_json)?
        } else {
            SyncState::default()
        };

        Ok(SyncEngine {
            source_repo,
            target_repo,
            state_path,
            state,
        })
    }

    /// The main entry point for the synchronization state machine.
    ///
    /// This async function orchestrates the entire process:
    /// 1. Fetches the latest changes from remotes.
    /// 2. Analyzes the commit graph to determine if repositories have diverged
    ///    or if one is ahead of the other.
    /// 3. Applies changes by pushing commits (logic to be implemented).
    /// 4. Persists the new state upon successful synchronization.
    async fn run(&mut self) -> SyncResult<String> {
        println!("Starting synchronization...");

        // Phase 1: Fetch updates from all remotes to ensure we have the latest data.
        self.fetch_repo("source", &self.source_repo).await?;
        self.fetch_repo("target", &self.target_repo).await?;

        // Phase 2: Analyze divergence by finding heads and the sync base.
        let source_head = self
            .source_repo
            .find_branch("main", git2::BranchType::Local)?
            .get()
            .peel_to_commit()?
            .id();
        let target_head = self
            .target_repo
            .find_branch("main", git2::BranchType::Local)?
            .get()
            .peel_to_commit()?
            .id();

        let base_oid = self.find_sync_base(source_head, target_head)?;

        let (source_ahead, _) = self
            .source_repo
            .graph_ahead_behind(source_head, base_oid)?;
        let (target_ahead, _) = self
            .target_repo
            .graph_ahead_behind(target_head, base_oid)?;

        println!("Analysis complete:");
        println!("- Source is {} commits ahead.", source_ahead);
        println!("- Target is {} commits ahead.", target_ahead);

        // Phase 3: Apply changes and update state.
        let result_message = if source_ahead > 0 && target_ahead > 0 {
            // DIVERGENCE: This is a critical failure case. The safest action is
            // to stop and inform the user to avoid data loss.
            return Err("Repositories have diverged! Manual intervention required.".into());
        } else if source_ahead > 0 {
            println!("Applying {} commits from source to target...", source_ahead);
            // TODO: Implement the actual logic to apply source commits to target.
            // This would typically involve creating a temporary remote and pushing.
            // For now, we simulate success and update the state.

            // Upon successful application, update state to the new head.
            self.state.last_source_synced_oid = Some(source_head.to_string());
            self.state.last_target_synced_oid = Some(source_head.to_string()); // Target is now at source_head
            self.save_state()?; // Persist the new state.
            "Sync from source to target not yet implemented.".to_string()
        } else if target_ahead > 0 {
            println!("Applying {} commits from target to source...", target_ahead);
            // TODO: Implement the actual logic to apply target commits to source.

            // Upon successful application, update state to the new head.
            self.state.last_source_synced_oid = Some(target_head.to_string()); // Source is now at target_head
            self.state.last_target_synced_oid = Some(target_head.to_string());
            self.save_state()?; // Persist the new state.
            "Sync from target to source not yet implemented.".to_string()
        } else {
            "Repositories are already in sync.".to_string()
        };

        Ok(result_message)
    }

    /// Helper to fetch updates for a given repository from its "origin" remote.
    async fn fetch_repo(&self, name: &str, repo: &Repository) -> SyncResult<()> {
        println!("Fetching updates for {} repository...", name);
        let mut remote = repo.find_remote("origin")?;
        // Fetch the 'main' branch. A more robust implementation might fetch all branches
        // or use a configurable refspec.
        remote.fetch(&["main"], None, None)?;
        Ok(())
    }

    /// Finds the common base for synchronization.
    ///
    /// The strategy is to first trust our persistent state, as it represents the
    /// last known good common point. If the state is missing or the commits
    /// can't be found (e.g., due to a rebase), it falls back to calculating a
    /// new merge base from the current heads.
    fn find_sync_base(&self, source_oid: Oid, target_oid: Oid) -> SyncResult<Oid> {
        if let (Some(s_oid_str), Some(t_oid_str)) =
            (&self.state.last_source_synced_oid, &self.state.last_target_synced_oid)
        {
            println!("Found previous sync state. Calculating merge base from saved OIDs.");
            let last_source_oid = Oid::from_str(s_oid_str)?;
            let last_target_oid = Oid::from_str(t_oid_str)?;

            // We can use either repository to find the merge base of two commits.
            // This is the most reliable way to find the true common ancestor from the last sync.
            return self.source_repo.merge_base(last_source_oid, last_target_oid)
                .map_err(|e| format!("Could not find merge base for saved OIDs {s_oid_str} and {t_oid_str}. Have the branches been rebased? Error: {e}").into());
        }

        // Fallback for the very first run or if state was lost.
        println!("No previous sync state found. Calculating merge base from current heads.");
        self.source_repo
            .merge_base(source_oid, target_oid)
            .map_err(|e| e.into())
    }

    /// Persists the current in-memory state to a JSON file on disk.
    /// This is a critical step to ensure subsequent runs are efficient and correct.
    fn save_state(&self) -> SyncResult<()> {
        println!("Saving sync state to disk at {:?}...", self.state_path);
        let state_json = serde_json::to_string_pretty(&self.state)?;
        std::fs::write(&self.state_path, state_json)?;
        println!("State saved successfully.");
        Ok(())
    }
}

// --- 4. The Command Orchestrator ---
/// Handles the `sync-run` command dispatched from an external caller (e.g., FFI).
///
/// This function is the public-facing entry point. It parses arguments,
/// instantiates the `SyncEngine`, and executes the main `run` loop, translating
/// all internal `SyncResult` errors into simple strings for the caller.
pub async fn handle_run_sync(args: &[String]) -> Result<String, String> {
    if args.len() != 2 {
        return Err(
            "Usage: sync-run <path_to_source_repo> <path_to_target_repo>".to_string(),
        );
    }

    let mut engine = SyncEngine::new(&args[0], &args[1])
        .map_err(|e| format!("Failed to initialize sync engine: {}", e))?;

    engine.run().await.map_err(|e| e.to_string())
}
