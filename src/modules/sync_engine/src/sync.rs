/* Copyright (C) 2025 Pedro Henrique
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
 * SPDX-License-Identifier: GPL-3.0-or-later */

use serde::{Deserialize, Serialize};
use std::path::{Path, PathBuf};
use git2::{Repository, Oid};

// A custom error type for our sync logic for clean error handling.
type SyncResult<T> = Result<T, Box<dyn std::error::Error + Send + Sync>>;

// --- 1. The Persistent State Model ---
// This struct is serialized to/from JSON to keep track of progress.
#[derive(Serialize, Deserialize, Debug, Default)]
struct SyncState {
    last_source_synced_oid: Option<String>,
    last_target_synced_oid: Option<String>,
}

// --- 2. The Sync Engine ---
struct SyncEngine {
    // For simplicity, we assume local paths. A real implementation would
    // clone from remote URLs into a temporary directory.
    source_repo: Repository,
    target_repo: Repository,
    state_path: PathBuf,
    state: SyncState,
}

impl SyncEngine {
    /// Creates a new SyncEngine, loading the state from disk.
    fn new(source_path: &str, target_path: &str) -> SyncResult<Self> {
        let source_repo = Repository::open(source_path)?;
        let target_repo = Repository::open(target_path)?;

        // State is stored in a hidden file in the source repo's .git dir.
        let state_path = Path::new(source_path).join(".git").join("gitph_sync_state.json");

        let state = if state_path.exists() {
            let state_json = std::fs::read_to_string(&state_path)?;
            serde_json::from_str(&state_json)?
        } else {
            SyncState::default()
        };

        Ok(SyncEngine { source_repo, target_repo, state_path, state })
    }

    /// The main entry point for the state machine.
    async fn run(&mut self) -> SyncResult<String> {
        println!("Starting synchronization...");

        // Phase 1: Fetch updates from all remotes.
        // NOTE: This is a simplified fetch. A real one needs auth callbacks.
        self.fetch_repo("source", &self.source_repo).await?;
        self.fetch_repo("target", &self.target_repo).await?;

        // Phase 2: Analyze divergence.
        // For now, we assume 'main' branch. This should be configurable.
        let source_head = self.source_repo.find_branch("main", git2::BranchType::Local)?.get().peel_to_commit()?.id();
        let target_head = self.target_repo.find_branch("main", git2::BranchType::Local)?.get().peel_to_commit()?.id();

        // Use the last synced state as the base for comparison.
        // If no state exists, we find the common ancestor between the two heads.
        let base_oid = self.find_sync_base(source_head, target_head)?;

        let (source_ahead, _) = self.source_repo.graph_ahead_behind(source_head, base_oid)?;
        let (target_ahead, _) = self.target_repo.graph_ahead_behind(target_head, base_oid)?;

        println!("Analysis complete:");
        println!("- Source is {} commits ahead.", source_ahead);
        println!("- Target is {} commits ahead.", target_ahead);

        // Phase 3: Apply changes based on a strategy.
        if source_ahead > 0 && target_ahead > 0 {
            // DIVERGENCE: The safest action is to stop and inform the user.
            return Err("Repositories have diverged! Manual intervention required.".into());
        } else if source_ahead > 0 {
            // TODO: Implement logic to apply source commits to target.
            println!("Applying {} commits from source to target...", source_ahead);
            // self.apply_commits(&self.source_repo, &self.target_repo, source_head, base_oid)?;
            return Ok("Sync from source to target not yet implemented.".to_string());
        } else if target_ahead > 0 {
            // TODO: Implement logic to apply target commits to source.
            println!("Applying {} commits from target to source...", target_ahead);
            return Ok("Sync from target to source not yet implemented.".to_string());
        } else {
            return Ok("Repositories are already in sync.".to_string());
        }
    }

    /// Helper to fetch updates for a given repository.
    async fn fetch_repo(&self, name: &str, repo: &Repository) -> SyncResult<()> {
        println!("Fetching updates for {} repository...", name);
        let mut remote = repo.find_remote("origin")?;
        // This is where you would configure credentials for private repos.
        remote.fetch(&["main"], None, None)?;
        Ok(())
    }

    /// Finds the common base for synchronization.
    fn find_sync_base(&self, source_oid: Oid, target_oid: Oid) -> SyncResult<Oid> {
        // A robust strategy: if we have a synced state for both, find their
        // common ancestor. For now, we simplify.
        if let (Some(s_oid_str), Some(t_oid_str)) = (&self.state.last_source_synced_oid, &self.state.last_target_synced_oid) {
            // In a real scenario, we'd verify these oids still exist and find
            // their common ancestor. Here we'll just use the source for simplicity.
            return Ok(Oid::from_str(s_oid_str)?);
        }
        // If no state, find the merge base between the current heads.
        self.source_repo.merge_base(source_oid, target_oid)
            .map_err(|e| e.into())
    }

    /// Persists the current state to disk.
    fn _save_state(&self) -> SyncResult<()> {
        let state_json = serde_json::to_string_pretty(&self.state)?;
        std::fs::write(&self.state_path, state_json)?;
        Ok(())
    }
}


// --- 4. The Command Orchestrator ---
/// Handles the `sync-run` command dispatched from the FFI layer.
pub async fn handle_run_sync(args: &[String]) -> Result<String, String> {
    if args.len() != 2 {
        return Err("Usage: sync-run <path_to_source_repo> <path_to_target_repo>".to_string());
    }

    let mut engine = SyncEngine::new(&args[0], &args[1])
        .map_err(|e| format!("Failed to initialize sync engine: {}", e))?;

    engine.run().await.map_err(|e| e.to_string())
}