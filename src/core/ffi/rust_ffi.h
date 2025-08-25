/*
 * Copyright (C) 2025 Pedro Henrique / phkaiser13
 *
 * File: src/core/ffi/rust_ffi.h
 *
 * [
 * This header file defines the public API for the Foreign Function Interface (FFI)
 * layer that bridges the C core of PeitchGIT with the Rust-based functional modules.
 * Its primary role is to provide a clean, high-level C interface for invoking
 * Rust code, abstracting away the complexities of dynamic library loading and
 * function calling conventions.
 *
 * Architecture:
 * The FFI layer is designed to be a set of simple, command-oriented functions. Each
 * function corresponds to a specific Rust module (e.g., `k8s_preview`, `release_orchestrator`)
 * that exposes functionality to the C core. The `cli_parser` module will use these
 * functions to dispatch commands after parsing them.
 *
 * Communication Protocol:
 * The data is passed between C and Rust using a standardized contract:
 * - C to Rust: A null-terminated UTF-8 string containing a JSON object. This JSON
 * object carries all the necessary configuration and parameters for the command.
 * - Rust to C: The Rust modules will return a status code (integer). A value of 0
 * indicates success, while any non-zero value signifies an error. Detailed results
 * or error messages will be handled via stdout/stderr or a future callback mechanism.
 *
 * Memory Management:
 * The C side is responsible for managing the memory of the JSON string passed to Rust.
 * Rust functions are designed to only read this string. If a Rust function needed to
 * return a string to C, it would be responsible for allocating it in a C-compatible
 * manner, and C would be responsible for freeing it. However, the current contract
 * simplifies this by using integer status codes for returns.
 *
 * This file declares the function `ffi_call_preview_module`, which serves as the
 * template for all future FFI function declarations.
 * ]
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef RUST_FFI_H
#define RUST_FFI_H

/**
 * @brief Invokes the main entry point of the Rust 'k8s_preview' module.
 *
 * @param json_config A constant, null-terminated string containing the command
 * configuration in JSON format. The format must match the contract expected
 * by the Rust module.
 * @return An integer status code. Returns 0 on success, and a non-zero value
 * on failure. The specific non-zero code may indicate the type of error.
 *
 * This function serves as the C-side entry point for all 'preview' related
 * commands. It dynamically loads the `k8s_preview` shared library, locates the
 * exported `invoke` function, passes the `json_config` to it, and returns the
 * result. All complexities of the FFI call are handled internally by the
 * implementation in `rust_ffi.c`.
 */
int ffi_call_preview_module(const char *json_config);

/**
 * @brief Invokes the main entry point of the Rust 'release_orchestrator' module.
 *
 * @param json_config A constant, null-terminated string containing the command
 * configuration in JSON format.
 * @return An integer status code, 0 for success, non-zero for failure.
 *
 * (Note: This is a placeholder for future implementation and follows the same
 * architectural pattern as `ffi_call_preview_module`.)
 */
// int ffi_call_release_module(const char *json_config);

/**
 * @brief Invokes the main entry point of the Rust 'kube' interaction module.
 *
 * @param json_config A constant, null-terminated string containing the command
 * configuration in JSON format.
 * @return An integer status code, 0 for success, non-zero for failure.
 *
 * (Note: This is a placeholder for future implementation.)
 */
// int ffi_call_kube_module(const char *json_config);

#endif // RUST_FFI_H