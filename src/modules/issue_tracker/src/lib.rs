/* Copyright (C) 2025 Pedro Henrique
 * lib.rs - FFI bridge and async entry point for the issue_tracker module.
 *
 * This file serves as the FFI layer, bridging the synchronous C world with
 * the asynchronous Rust world of this module. It implements the gitph API
 * contract and handles the crucial task of managing the Tokio async runtime.
 *
 * The core design pattern here is to create a single, static Tokio runtime
 * that lives for the duration of the application. When a C function like
 * `module_exec` is called, it uses `runtime.block_on()` to execute the
 * async business logic and wait for its completion, effectively translating
 * the async operation into a sync result for the C core.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later */

// FFI/C compatibility
use libc::{c_char, c_int};
use std::ffi::{CStr, CString};
use std::sync::Mutex;

// Async runtime and dependencies
use lazy_static::lazy_static;
use tokio::runtime::Runtime;

// Internal modules we will create next
mod issue_service;

// --- C API Data Structure Definitions (matching gitph_core_api.h) ---
#[repr(C)]
pub enum GitphStatus {
    Success = 0,
    ErrorGeneral = -1,
    ErrorInvalidArgs = -2,
    ErrorNotFound = -3,
    ErrorInitFailed = -4,
    ErrorExecFailed = -5,
}

#[repr(C)]
pub enum GitphLogLevel {
    Debug,
    Info,
    Warn,
    Error,
    Fatal,
}

type LogFn = extern "C" fn(GitphLogLevel, *const c_char, *const c_char);

#[repr(C)]
// Adicionando `Copy` e `Clone` para facilitar o manuseio, especialmente com `ptr::read`.
#[derive(Copy, Clone)]
pub struct GitphCoreContext {
    log: Option<LogFn>,
    _get_config_value: Option<extern "C" fn()>,
    _print_ui: Option<extern "C" fn()>,
}

// --- Global State Management ---

// A global, thread-safe Tokio runtime.
lazy_static! {
    static ref RUNTIME: Runtime = Runtime::new().expect("Failed to create Tokio runtime");
}

// Global, thread-safe storage for the context from the C core.
lazy_static! {
    static ref CORE_CONTEXT: Mutex<Option<GitphCoreContext>> = Mutex::new(None);
}

// Helper for logging back to the C core.
fn log_to_core(level: GitphLogLevel, message: &str) {
    if let Ok(guard) = CORE_CONTEXT.lock() {
        if let Some(context) = &*guard {
            if let Some(log_fn) = context.log {
                let module_name = CString::new("ISSUE_TRACKER").unwrap();
                let msg = CString::new(message).unwrap();
                log_fn(level, module_name.as_ptr(), msg.as_ptr());
            }
        }
    }
}

// --- C-compatible API Implementation ---

#[repr(C)]
pub struct GitphModuleInfo {
    name: *const c_char,
    version: *const c_char,
    description: *const c_char,
    commands: *const *const c_char,
}

// FIX 1: Create a wrapper struct for GitphModuleInfo.
// REASON: GitphModuleInfo contains raw pointers, so it is not `Sync`. By wrapping it
// and implementing `unsafe impl Sync`, we are telling the compiler that we guarantee
// it's safe to be shared across threads. This is true because the pointers inside
// will only ever point to other 'static read-only data.
struct SafeModuleInfo(GitphModuleInfo);
unsafe impl Sync for SafeModuleInfo {}


static MODULE_NAME: &[u8] = b"issue_tracker\0";
static MODULE_VERSION: &[u8] = b"1.0.0\0";
static MODULE_DESC: &[u8] = b"Interacts with issue tracking services like GitHub Issues.\0";
static SUPPORTED_COMMANDS: &[*const u8] = &[
    b"issue-get\0".as_ptr(),
    std::ptr::null(),
];

// FIX 2: Use the `Sync` wrapper for the static variable.
// REASON: This satisfies the compiler's requirement that static variables be `Sync`.
static MODULE_INFO: SafeModuleInfo = SafeModuleInfo(GitphModuleInfo {
    name: MODULE_NAME.as_ptr() as *const c_char,
    version: MODULE_VERSION.as_ptr() as *const c_char,
    description: MODULE_DESC.as_ptr() as *const c_char,
    commands: SUPPORTED_COMMANDS.as_ptr() as *const *const c_char,
});

#[no_mangle]
pub extern "C" fn module_get_info() -> *const GitphModuleInfo {
    // FIX 3: Return a pointer to the inner data.
    &MODULE_INFO.0
}

#[no_mangle]
pub extern "C" fn module_init(context: *const GitphCoreContext) -> GitphStatus {
    if context.is_null() {
        return GitphStatus::ErrorInitFailed;
    }
    // Store the context and ensure the runtime is initialized.
    if let Ok(mut guard) = CORE_CONTEXT.lock() {
        *guard = Some(unsafe { std::ptr::read(context) });
    } else {
        return GitphStatus::ErrorInitFailed; // Mutex was poisoned
    }

    // Eagerly initialize the runtime by referencing it.
    let _ = &*RUNTIME;
    log_to_core(GitphLogLevel::Info, "issue_tracker module initialized.");
    GitphStatus::Success
}

#[no_mangle]
pub extern "C" fn module_exec(argc: c_int, argv: *const *const c_char) -> GitphStatus {
    let args: Vec<String> = (0..argc as isize)
        .map(|i| unsafe { CStr::from_ptr(*argv.offset(i)).to_string_lossy().into_owned() })
        .collect();

    if args.is_empty() {
        log_to_core(GitphLogLevel::Error, "Execution called with no arguments.");
        return GitphStatus::ErrorInvalidArgs;
    }

    let command = args[0].as_str();
    let user_args = args[1..].to_vec(); // Clone user_args to give it a 'static lifetime for the async block

    // Execute the async logic on the runtime and block until it completes.
    let result = RUNTIME.block_on(async {
        match command {
            "issue-get" => issue_service::handle_get_issue(&user_args).await,
            _ => Err(format!("Unknown command '{}'", command)),
        }
    });

    match result {
        Ok(_) => GitphStatus::Success,
        Err(e) => {
            let error_message = format!("Command failed: {}", e);
            log_to_core(GitphLogLevel::Error, &error_message);
            GitphStatus::ErrorExecFailed
        }
    }
}

#[no_mangle]
pub extern "C" fn module_cleanup() {
    log_to_core(GitphLogLevel::Info, "issue_tracker module cleaned up.");
}