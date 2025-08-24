/* Copyright (C) 2025 Pedro Henrique / phkaiser13
* File: src/modules/release_orchestrator/src/lib.rs
* This file is the main entry point for the `release_orchestrator` dynamic
* library. It defines the FFI boundary for the C core. Its primary role is to
* act as an orchestrator: it parses the configuration, uses factory functions
* to instantiate the appropriate strategy and service mesh clients, and then
* injects the mesh client into the strategy's execution method. This design
* keeps the entry point clean and decoupled from all implementation details.
* SPDX-License-Identifier: Apache-2.0 */

// Public modules that define the library's structure.
pub mod config;
pub mod mesh;
pub mod strategies;

use crate::config::ReleaseConfig;
use anyhow::Result;
use std::ffi::{c_char, CStr};
use std::panic;

/// The main FFI entry point for the C core to run a release orchestration.
///
/// # Safety
/// The `config_json` pointer must be a valid, null-terminated C string. The C
/// core is responsible for managing the memory of this string.
///
/// # Returns
/// - `0` on success.
/// - `-1` on a null pointer input.
/// - `-2` on a UTF-8 conversion error.
/// - `-3` on a JSON parsing error.
/// - `-4` on a runtime execution error.
/// - `-5` on a panic within the Rust code.
#[no_mangle]
pub extern "C" fn run_release(config_json: *const c_char) -> i32 {
    let result = panic::catch_unwind(|| {
        // 1. Safely handle C string and parse configuration.
        if config_json.is_null() {
            eprintln!("[release_orchestrator] Error: Received a null pointer for config_json.");
            return -1;
        }
        let c_str = unsafe { CStr::from_ptr(config_json) };
        let rust_str = match c_str.to_str() {
            Ok(s) => s,
            Err(e) => {
                eprintln!("[release_orchestrator] Error: Invalid UTF-8 in config string: {}", e);
                return -2;
            }
        };
        let config: ReleaseConfig = match serde_json::from_str(rust_str) {
            Ok(c) => c,
            Err(e) => {
                eprintln!("[release_orchestrator] Error: Failed to parse JSON config: {}", e);
                return -3;
            }
        };

        // 2. Set up the async runtime.
        let runtime = match tokio::runtime::Builder::new_multi_thread().enable_all().build() {
            Ok(rt) => rt,
            Err(e) => {
                eprintln!("[release_orchestrator] Error: Failed to build Tokio runtime: {}", e);
                return -4;
            }
        };

        // 3. Execute the main logic.
        match runtime.block_on(run_release_internal(config)) {
            Ok(_) => 0, // Success
            Err(e) => {
                eprintln!("[release_orchestrator] Error during execution: {:?}", e);
                -4 // Runtime error
            }
        }
    });

    match result {
        Ok(status_code) => status_code,
        Err(_) => {
            eprintln!("[release_orchestrator] Error: A panic occurred. This is a critical bug.");
            -5 // Panic
        }
    }
}

/// The internal async function that contains the core orchestration logic.
async fn run_release_internal(config: ReleaseConfig) -> Result<()> {
    println!("Initializing release orchestration...");

    // 1. Initialize the Kubernetes client.
    let kube_client = kube::Client::try_default().await?;

    // 2. Use the factory to get the correct service mesh client implementation.
    // The `mesh_client` is a trait object, hiding the concrete type (e.g., IstioClient).
    let mesh_client = mesh::get_mesh_client(config.service_mesh, kube_client)?;

    // 3. Use the factory to get the correct strategy implementation.
    // The `strategy` is also a trait object.
    let strategy = strategies::get_strategy(config.strategy)?;

    // 4. Execute the strategy, injecting the mesh client as a dependency.
    // This is the core of the design: the strategy logic calls the mesh logic
    // through the abstract `ServiceMeshClient` trait.
    strategy.execute(&config, mesh_client.as_ref()).await?;

    println!("Release orchestration completed successfully.");
    Ok(())
}