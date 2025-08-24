/* Copyright (C) 2025 Pedro Henrique / phkaiser13
* File: src/modules/git_sync/src/lib.rs
* This file is the main entry point for the `git_sync` dynamic library. It
* defines the FFI boundary, allowing the C core to invoke the synchronization
* logic. It orchestrates the entire process: cloning the Git repository,
* initializing a Kubernetes client, detecting configuration drift, and
* optionally triggering a reconciliation action.
* SPDX-License-Identifier: Apache-2.0 */

// Public modules that define the library's structure.
pub mod config;
pub mod git_ops;
pub mod kube_diff;

use crate::config::SyncConfig;
use anyhow::Result;
use std::ffi::{c_char, CStr};
use std::panic;

/// The main FFI entry point for the C core to run the git-sync process.
///
/// This function orchestrates the cloning of the repository, comparison of
/// manifests against the live cluster state, and reporting of any drift.
///
/// # Safety
/// The `config_json` pointer must be a valid, null-terminated C string. The C
/// core is responsible for managing the memory of this string.
///
/// # Returns
/// - `0` on success (regardless of whether drift was found).
/// - `-1` on a null pointer input.
/// - `-2` on a UTF-8 conversion error.
/// - `-3` on a JSON parsing error.
/// - `-4` on a runtime execution error (e.g., failed to clone, connect to k8s).
/// - `-5` on a panic within the Rust code.
#[no_mangle]
pub extern "C" fn run_sync(config_json: *const c_char) -> i32 {
    let result = panic::catch_unwind(|| {
        // 1. Safely handle the C string and parse configuration.
        if config_json.is_null() {
            eprintln!("[git_sync_module] Error: Received a null pointer for config_json.");
            return -1;
        }
        let c_str = unsafe { CStr::from_ptr(config_json) };
        let rust_str = match c_str.to_str() {
            Ok(s) => s,
            Err(e) => {
                eprintln!("[git_sync_module] Error: Invalid UTF-8 in config string: {}", e);
                return -2;
            }
        };
        let config: SyncConfig = match serde_json::from_str(rust_str) {
            Ok(c) => c,
            Err(e) => {
                eprintln!("[git_sync_module] Error: Failed to parse JSON config: {}", e);
                return -3;
            }
        };

        // 2. Set up the async runtime.
        let runtime = match tokio::runtime::Builder::new_multi_thread().enable_all().build() {
            Ok(rt) => rt,
            Err(e) => {
                eprintln!("[git_sync_module] Error: Failed to build Tokio runtime: {}", e);
                return -4;
            }
        };

        // 3. Execute the main logic.
        match runtime.block_on(run_sync_internal(config)) {
            Ok(_) => 0, // Success
            Err(e) => {
                eprintln!("[git_sync_module] Error during execution: {:?}", e);
                -4 // Runtime error
            }
        }
    });

    match result {
        Ok(status_code) => status_code,
        Err(_) => {
            eprintln!("[git_sync_module] Error: A panic occurred. This is a critical bug.");
            -5 // Panic
        }
    }
}

/// The internal async function that contains the core logic.
async fn run_sync_internal(config: SyncConfig) -> Result<()> {
    // 1. Clone the repository into a temporary, self-cleaning directory.
    // The `temp_dir` variable owns the directory; when it goes out of scope
    // at the end of this function, the directory and its contents are deleted.
    let temp_dir = git_ops::clone_repo(&config.repo_url, &config.branch)?;
    let manifests_full_path = temp_dir.path().join(&config.manifest_path);

    if !manifests_full_path.exists() || !manifests_full_path.is_dir() {
        anyhow::bail!(
            "Manifest path '{}' does not exist inside the cloned repository.",
            manifests_full_path.display()
        );
    }

    // 2. Initialize the Kubernetes client.
    // This re-uses the same helper logic as the k8s_preview module.
    let client_config = kube::Config::infer().await?;
    let client = kube::Client::try_from(client_config)?;

    // 3. Detect drift.
    println!("Starting drift detection...");
    match kube_diff::detect_drift(&client, &manifests_full_path).await? {
        Some(drift_report) => {
            println!("Drift detected!");
            println!("--- DRIFT REPORT ---");
            println!("{}", drift_report);
            println!("--- END REPORT ---");

            if !config.dry_run && config.create_pr {
                println!("Attempting to create a reconciliation pull request...");
                git_ops::create_reconciliation_pr(
                    temp_dir.path(),
                    &drift_report,
                    "PeitchGIT: Reconcile Kubernetes cluster drift",
                )?;
            } else if config.dry_run {
                println!("Dry run is enabled. No action will be taken.");
            }
        }
        None => {
            println!("No drift detected. Cluster state is synchronized with the Git repository.");
        }
    }

    Ok(())
}