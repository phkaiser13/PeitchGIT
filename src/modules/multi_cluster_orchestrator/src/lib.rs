/* Copyright (C) 2025 Pedro Henrique / phkaiser13
* File: src/modules/multi_cluster_orchestrator/src/lib.rs
* This file is the main entry point for the `multi_cluster_orchestrator` dynamic
* library. It defines the FFI boundary that allows the C core to invoke the
* Rust logic. Its primary responsibility is to safely handle data from C, set
* up the asynchronous Tokio runtime, orchestrate the module's business logic
* via the `ClusterManager`, and then format and report the results of the
* multi-cluster operation back to the user.
* SPDX-License-Identifier: Apache-2.0 */

mod cluster_manager;

use crate::cluster_manager::{ClusterManager, MultiClusterConfig};
use std::ffi::{c_char, CStr};
use std::panic;

/// The main FFI entry point for the C core to run multi-cluster orchestration.
///
/// This function takes a JSON configuration specifying clusters, targets, and an
/// action, then executes the action concurrently across the targets.
///
/// # Safety
/// The `config_json` pointer must be a valid, null-terminated C string. The C
/// core is responsible for managing the memory of this string.
///
/// # Returns
/// - `0` if the action succeeded on ALL target clusters.
/// - `-1` on a null pointer input.
/// - `-2` on a UTF-8 conversion error.
/// - `-3` on a JSON parsing error.
/// - `-4` on a runtime or initialization error (e.g., failed to build Tokio runtime).
/// - `-5` on a panic within the Rust code.
/// - `-6` if one or more cluster operations failed during execution.
#[no_mangle]
pub extern "C" fn run_multi_cluster_orchestrator(config_json: *const c_char) -> i32 {
    // Use `catch_unwind` to prevent panics from crossing the FFI boundary,
    // which is undefined behavior.
    let result = panic::catch_unwind(|| {
        // 1. Safely handle the C string and deserialize the configuration.
        if config_json.is_null() {
            eprintln!("[multi_cluster] Error: Received a null pointer for config_json.");
            return -1;
        }
        let c_str = unsafe { CStr::from_ptr(config_json) };
        let rust_str = match c_str.to_str() {
            Ok(s) => s,
            Err(e) => {
                eprintln!("[multi_cluster] Error: Invalid UTF-8 in config string: {}", e);
                return -2;
            }
        };
        let config: MultiClusterConfig = match serde_json::from_str(rust_str) {
            Ok(c) => c,
            Err(e) => {
                eprintln!("[multi_cluster] Error: Failed to parse JSON config: {}", e);
                return -3;
            }
        };

        // 2. Create a Tokio runtime to execute our async logic.
        let runtime = match tokio::runtime::Builder::new_multi_thread()
            .enable_all()
            .build()
        {
            Ok(rt) => rt,
            Err(e) => {
                eprintln!("[multi_cluster] Error: Failed to build Tokio runtime: {}", e);
                return -4;
            }
        };

        // 3. Execute the main logic within the async runtime.
        let execution_result = runtime.block_on(async {
            // Initialize the ClusterManager, which connects to all defined clusters.
            let manager = match ClusterManager::new(&config.cluster_configs).await {
                Ok(m) => m,
                Err(e) => return Err(e),
            };

            // Execute the action across the specified targets.
            Ok(manager.execute_action(&config.targets, &config.action).await)
        });

        // 4. Process and report the results, determining the final status code.
        match execution_result {
            Ok(results) => {
                println!("\n--- Multi-Cluster Operation Report ---");
                let mut all_successful = true;
                for res in &results {
                    match &res.outcome {
                        Ok(msg) => {
                            println!("[SUCCESS] Cluster: {}", res.cluster_name);
                            println!("          Details: {}", msg);
                        }
                        Err(e) => {
                            all_successful = false;
                            println!("[FAILURE] Cluster: {}", res.cluster_name);
                            println!("          Error: {}", e);
                        }
                    }
                }
                println!("--- End of Report ---");

                if all_successful {
                    println!("\nAll operations completed successfully.");
                    0 // Success
                } else {
                    eprintln!("\nOne or more cluster operations failed.");
                    -6 // Partial or total failure
                }
            }
            Err(e) => {
                eprintln!("[multi_cluster] Error during initialization or execution: {:?}", e);
                -4 // Runtime error
            }
        }
    });

    match result {
        Ok(status_code) => status_code,
        Err(_) => {
            eprintln!("[multi_cluster] Error: A panic occurred. This is a critical bug.");
            -5 // Panic
        }
    }
}