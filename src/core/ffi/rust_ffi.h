/*
 * Copyright (C) 2025 Pedro Henrique / phkaiser13
 *
 * File: src/core/ffi/rust_ffi.h
 *
 * [
 * This header file defines the Foreign Function Interface (FFI) contract between the core C
 * application and the Rust modules. It contains the function prototypes for all Rust functions
 * that are exposed to the C side. This creates a clean, well-defined boundary between the two
 * languages, allowing them to interoperate effectively.
 *
 * Architecture:
 * Each function declared here corresponds to a Rust function annotated with `#[no_mangle]`
 * and `extern "C"`. The use of fixed-width integer types (e.g., `int32_t`) and standard C
 * types (`const char *`) is intentional to ensure ABI compatibility across different platforms
 * and compiler versions. This header is the single source of truth for the C code regarding
 * the available Rust functionality.
 *
 * Role in the System:
 * This FFI layer is crucial for our polyglot architecture. It allows us to leverage Rust's
 * strengths in safety, performance, and its rich ecosystem for specific tasks (like policy
 * checking, Git operations, etc.), while keeping the main application orchestration in C.
 * The `dispatcher.c` module is the primary consumer of this header, using it to call into
 * the Rust business logic.
 * ]
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef RUST_FFI_H
#define RUST_FFI_H

#include <stdint.h> // Required for fixed-width integer types like int32_t

// --- FFI Function Prototypes ---

/**
 * @brief A placeholder test function to verify the FFI setup.
 *
 * This function is typically used during initial development to ensure that the C application
 * can correctly link against and call into the Rust library.
 */
void rust_hello_world(void);

/**
 * @brief Runs a policy check using the Rust `policy_engine` module.
 *
 * This function invokes the `conftest` tool via a Rust wrapper to validate Kubernetes
 * manifest files against a set of Rego policies. It abstracts away the complexities of
 * subprocess management and output parsing from the C caller.
 *
 * @param policy_path A null-terminated C string representing the path to the directory
 * containing the .rego policy files.
 * @param manifest_path A null-terminated C string representing the path to the Kubernetes
 * manifest file or directory to be checked.
 *
 * @return An `int32_t` status code:
 * - 0: All policy checks passed successfully.
 * - 1: A policy violation was found, or an error occurred during execution (e.g.,
 * `conftest` not found, file paths are invalid, JSON output parsing failed).
 * Detailed error messages will be printed to stderr by the Rust module.
 *
 * Pre-conditions:
 * - `policy_path` and `manifest_path` must be valid, null-terminated C strings.
 * - The `conftest` executable must be present in the system's PATH.
 *
 * Post-conditions:
 * - Policy validation is performed.
 * - Diagnostic output is printed to stdout/stderr.
 * - A status code indicating the outcome is returned.
 */
int32_t run_policy_check(const char *policy_path, const char *manifest_path);

// Add other Rust FFI function declarations here as the application grows.
// For example:
// int32_t git_ops_sync(const char *repo_url, const char *local_path);
// int32_t k8s_apply_manifest(const char *manifest_path, bool dry_run);

#endif // RUST_FFI_H