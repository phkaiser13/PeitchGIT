/*
 * Copyright (C) 2025 Pedro Henrique / phkaiser13
 *
 * File: src/modules/policy_engine/src/lib.rs
 *
 * [
 * This Rust library file implements the core logic for the policy engine module. It is designed
 * to be called from a C application via a Foreign Function Interface (FFI). Its primary responsibility
 * is to execute a policy check using an external tool (e.g., Conftest) against a given set of
 * Kubernetes manifest files and Rego policies.
 *
 * Architecture:
 * The library exposes a single C-callable function, `run_policy_check`. This function encapsulates
 * all the necessary steps to perform the policy validation:
 * 1.  Safely handles C string pointers passed from the FFI boundary.
 * 2.  Constructs and executes a command-line call to the `conftest` tool.
 * 3.  Captures the stdout and stderr of the `conftest` process for detailed analysis.
 * 4.  Parses the JSON output from `conftest` to determine the success or failure of the policy checks.
 * 5.  Returns a simple, C-compatible integer status code (0 for success, 1 for failure), abstracting
 * away the complexities of Rust's error handling from the C caller.
 *
 * The use of `std::process::Command` ensures that the policy check is run in a separate, isolated
 * process, enhancing the robustness of the main application. Error handling is managed using the
 * `anyhow` crate for internal ergonomics, while presenting a clear and simple contract at the FFI boundary.
 *
 * Role in the System:
 * This module is the concrete implementation of the `policy-check` command. It bridges the C-based
 * core application with the Go-based Conftest tool, leveraging Rust's strengths in process management,
 * safety, and JSON parsing to provide a reliable and secure policy validation mechanism.
 * ]
 *
 * SPDX-License-Identifier: Apache-2.0
 */

use std::ffi::CStr;
use std::os::raw::c_char;
use std::process::Command;
use serde::Deserialize;

// --- Data Structures for Deserializing Conftest JSON Output ---

/// Represents a single result within the Conftest JSON output array.
/// We are only interested in the 'failures' field for determining the outcome.
/// The `#[derive(Deserialize)]` attribute automatically generates the code
/// to deserialize from a JSON object into this struct.
#[derive(Deserialize, Debug)]
struct CheckResult {
    // We don't need most fields, but `failures` is critical.
    // It's an optional array of failure messages. If it's `None` or empty, the check passed.
    failures: Option<Vec<FailureDetail>>,
}

/// Represents the details of a single failure message.
/// This is used to check if the `failures` array is non-empty.
#[derive(Deserialize, Debug)]
struct FailureDetail {
    // We only need to know if this struct exists, its fields are not important for now.
    // However, `msg` is a useful field for future enhancements (e.g., detailed error reporting).
    #[allow(dead_code)]
    msg: String,
}

/// Represents the top-level structure of the Conftest JSON output, which is an array of CheckResult.
type ConftestOutput = Vec<CheckResult>;

// --- FFI Function Implementation ---

/**
 * @brief Runs a policy check using Conftest on the specified files.
 *
 * This function is exposed to C and orchestrates the execution of `conftest`. It takes two
 * C strings as input: the path to the policy directory and the path to the Kubernetes manifest files.
 * It returns 0 if all policies pass and 1 if there is any failure or an error occurs during execution.
 *
 * Pre-conditions:
 * - `policy_path` must be a valid, null-terminated C string pointing to the policy directory.
 * - `manifest_path` must be a valid, null-terminated C string pointing to the manifest file or directory.
 * - The `conftest` executable must be available in the system's PATH.
 *
 * Post-conditions:
 * - The function will print diagnostic information to stdout/stderr.
 * - It will return an integer status code.
 *
 * FFI Safety Considerations:
 * - `#[no_mangle]` prevents the Rust compiler from changing the function's name, making it linkable from C.
 * - `extern "C"` makes the function adhere to the C calling convention.
 * - The function uses `CStr::from_ptr` inside an `unsafe` block to handle the raw C pointers. This is the
 * primary FFI safety boundary. All subsequent code is safe Rust.
 * - All potential Rust errors (`Result<T, E>`) are caught and converted into a simple integer return code,
 * which is a safe and common pattern for FFI.
 */
#[no_mangle]
pub extern "C" fn run_policy_check(policy_path: *const c_char, manifest_path: *const c_char) -> i32 {
    // Wrap the core logic in a helper function that can use Rust's `Result` for error handling.
    // This keeps the main FFI function clean and focused on the C-to-Rust transition.
    match run_internal(policy_path, manifest_path) {
        Ok(success) => {
            if success {
                println!("[SUCCESS] All policies passed.");
                0 // Return 0 for success
            } else {
                println!("[FAILURE] One or more policies failed.");
                1 // Return 1 for policy failure
            }
        }
        Err(e) => {
            // If any error occurred (e.g., command not found, failed to parse output), print it
            // and return a non-zero exit code.
            eprintln!("[ERROR] Policy check execution failed: {:?}", e);
            1 // Return 1 for execution error
        }
    }
}

/// Internal helper function that contains the main logic and uses `anyhow::Result` for robust error handling.
fn run_internal(policy_path: *const c_char, manifest_path: *const c_char) -> anyhow::Result<bool> {
    // --- 1. Safely Convert C Strings to Rust Strings ---
    // This is the only `unsafe` block required. We trust the C caller to provide valid,
    // null-terminated strings. `CStr` provides a safe wrapper around these pointers.
    let policy_str = unsafe { CStr::from_ptr(policy_path) }.to_str()?;
    let manifest_str = unsafe { CStr::from_ptr(manifest_path) }.to_str()?;

    println!(
        "Running policy check...\n  Policies: {}\n  Manifests: {}",
        policy_str, manifest_str
    );

    // --- 2. Execute the Conftest Subprocess ---
    // We use `std::process::Command` to build and run the external command.
    // `conftest test -p <policies> <manifests> -o json`
    // The `-o json` flag is crucial as it provides structured, machine-readable output.
    let output = Command::new("conftest")
        .arg("test")
        .arg("-p")
        .arg(policy_str)
        .arg(manifest_str)
        .arg("-o")
        .arg("json")
        .output()?; // The `?` operator propagates errors up the call stack.

    // --- 3. Check for Command Execution Errors ---
    // If the command failed to execute (e.g., `conftest` not found), stderr will contain the error message.
    if !output.status.success() && output.stdout.is_empty() {
        let stderr = String::from_utf8_lossy(&output.stderr);
        anyhow::bail!("Conftest command failed to execute: {}", stderr);
    }

    let stdout = String::from_utf8_lossy(&output.stdout);
    println!("Conftest output:\n{}", stdout);

    // --- 4. Parse JSON and Determine Outcome ---
    // An empty JSON output or `[]` means no tests were run, which we can treat as success.
    if stdout.trim().is_empty() || stdout.trim() == "[]" {
        return Ok(true);
    }

    // Deserialize the JSON output into our Rust structs.
    let results: ConftestOutput = serde_json::from_str(&stdout)?;

    // --- 5. Check for Failures ---
    // Iterate through the results. If any result contains a non-empty `failures` array,
    // the policy check has failed. `all()` is a convenient iterator adapter for this check.
    let all_passed = results.iter().all(|r| {
        match &r.failures {
            Some(failures) => failures.is_empty(),
            None => true, // No `failures` key means success for that check.
        }
    });

    Ok(all_passed)
}