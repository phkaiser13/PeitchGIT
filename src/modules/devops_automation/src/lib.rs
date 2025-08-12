/* Copyright (C) 2025 Pedro Henrique / phkaiser13
* src/lib.rs - Unified FFI bridge and dispatcher for devops_automation.
*
* This file is the single entry point for the C core to interact with this
* module. It implements the `gitph_core_api.h` contract and contains a
* central dispatcher (`module_exec`) that routes commands to the appropriate
* handler function in the `commands.rs` module.
*
* It reuses the same safe FFI patterns established in the other Rust modules,
* such as `once_cell` for static data and a clear separation between safe
* Rust code and the `unsafe` FFI boundary.
*
* SPDX-License-Identifier: Apache-2.0 */

mod commands;
mod process_wrapper;

use libc::{c_char, c_int};
use once_cell::sync::Lazy;
use std::ffi::{CStr, CString};
use std::ptr;

// --- C ABI Definitions (should be identical to other modules) ---

#[repr(C)]
pub enum Status {
    Success = 0,
    ErrorInitFailed = 1,
    ErrorInvalidArgs = 2,
    ErrorExecFailed = 3,
}

#[repr(C)]
pub enum LogLevel {
    Debug = 0,
    Info = 1,
    Warn = 2,
    Error = 3,
    Fatal = 4,
}

#[repr(C)]
pub struct ModuleInfo {
    name: *const c_char,
    version: *const c_char,
    description: *const c_char,
    commands: *const *const c_char,
}

// --- Module Info Statics ---

static MODULE_NAME: Lazy<CString> = Lazy::new(|| CString::new("devops_automation").unwrap());
static MODULE_VERSION: Lazy<CString> = Lazy::new(|| CString::new("1.0.1").unwrap());
static MODULE_DESCRIPTION: Lazy<CString> = Lazy::new(|| CString::new("Wrapper for DevOps tools like Terraform and Vault [Rust]").unwrap());

static TF_PLAN_CMD: Lazy<CString> = Lazy::new(|| CString::new("tf-plan").unwrap());
static TF_APPLY_CMD: Lazy<CString> = Lazy::new(|| CString::new("tf-apply").unwrap());
static VAULT_READ_CMD: Lazy<CString> = Lazy::new(|| CString::new("vault-read").unwrap());

static SUPPORTED_COMMANDS: Lazy<[*const c_char; 4]> = Lazy::new(|| [
    TF_PLAN_CMD.as_ptr(),
    TF_APPLY_CMD.as_ptr(),
    VAULT_READ_CMD.as_ptr(),
    ptr::null(), // Null terminator
]);

static MODULE_INFO: Lazy<ModuleInfo> = Lazy::new(|| ModuleInfo {
    name: MODULE_NAME.as_ptr(),
    version: MODULE_VERSION.as_ptr(),
    description: MODULE_DESCRIPTION.as_ptr(),
    commands: SUPPORTED_COMMANDS.as_ptr(),
});

// --- C-compatible API Implementation ---

#[no_mangle]
pub extern "C" fn module_get_info() -> *const ModuleInfo {
    &*MODULE_INFO
}

#[no_mangle]
pub extern "C" fn module_init(_context: *const std::ffi::c_void) -> Status {
    // In this module, we don't need the context, but we keep the signature for consistency.
    println!("[DEVOPS_AUTOMATION] devops_automation (Rust) module initialized.");
    Status::Success
}

#[no_mangle]
pub extern "C" fn module_exec(argc: c_int, argv: *const *const c_char) -> Status {
    let args = unsafe { c_args_to_vec(argc, argv) };
    if args.is_empty() {
        eprintln!("[DEVOPS_AUTOMATION ERROR] Execution called with no command.");
        return Status::ErrorInvalidArgs;
    }

    let command = &args[0];
    let user_args = &args[1..];

    let result = match command.as_str() {
        "tf-plan" => commands::handle_terraform_plan(user_args),
        "tf-apply" => commands::handle_terraform_apply(user_args),
        "vault-read" => commands::handle_vault_read(user_args),
        _ => Err(format!("Unknown command '{}' for devops_automation module", command).into()),
    };

    match result {
        Ok(_) => Status::Success,
        Err(e) => {
            eprintln!("\n[gitph ERROR] Command failed: {}", e);
            Status::ErrorExecFailed
        }
    }
}

#[no_mangle]
pub extern "C" fn module_cleanup() {
    println!("[DEVOPS_AUTOMATION] devops_automation (Rust) module cleaned up.");
    // All CString memory is managed by `Lazy` and cleaned up at program exit.
}

/// Helper to convert C argc/argv to a safe `Vec<String>`.
unsafe fn c_args_to_vec(argc: c_int, argv: *const *const c_char) -> Vec<String> {
    if argc <= 0 || argv.is_null() {
        return Vec::new();
    }
    std::slice::from_raw_parts(argv, argc as usize)
        .iter()
        .map(|&p| CStr::from_ptr(p).to_string_lossy().into_owned())
        .collect()
}