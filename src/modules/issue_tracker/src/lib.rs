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
// Adding `Copy` and `Clone` to make handling easier, especially with `ptr::read`.
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

// This wrapper is necessary because GitphModuleInfo contains raw pointers,
// making it not `Sync`. By wrapping it and implementing `unsafe impl Sync`,
// we assert to the compiler that we guarantee thread-safe access. This holds
// true because the pointers within will only ever point to other 'static,
// read-only data, which is inherently thread-safe.
struct SafeModuleInfo(GitphModuleInfo);
unsafe impl Sync for SafeModuleInfo {}


// --- Static Module Information ---

// Define module metadata as static, null-terminated byte slices.
// This ensures they live for the entire program duration ('static) and are stored
// in the read-only data section of the binary.
static MODULE_NAME: &[u8] = b"issue_tracker\0";
static MODULE_VERSION: &[u8] = b"1.0.0\0";
static MODULE_DESC: &[u8] = b"Interacts with issue tracking services like GitHub Issues.\0";

// CORRECTION: Define the list of supported commands.
// REASON: The original `&[*const u8]` is not `Sync` and cannot be in a `static` variable.
// The fix is to create a `static` array of C-style pointers (`*const c_char`).
// This array is `Sync` because its size is known at compile time.
// It points to our static, null-terminated byte strings.
static CMD_ISSUE_GET: &[u8] = b"issue-get\0";
static SUPPORTED_COMMANDS_PTRS: [*const c_char; 2] = [
    CMD_ISSUE_GET.as_ptr() as *const c_char,
    std::ptr::null(), // The list must be null-terminated for C compatibility.
];

// Use the `Sync` wrapper for the static variable to satisfy the compiler.
// This static variable now correctly and safely constructs the C-compatible struct.
static MODULE_INFO: SafeModuleInfo = SafeModuleInfo(GitphModuleInfo {
    name: MODULE_NAME.as_ptr() as *const c_char,
    version: MODULE_VERSION.as_ptr() as *const c_char,
    description: MODULE_DESC.as_ptr() as *const c_char,
    // Point to our static array of command pointers.
    commands: SUPPORTED_COMMANDS_PTRS.as_ptr(),
});

#[no_mangle]
pub extern "C" fn module_get_info() -> *const GitphModuleInfo {
    // Return a pointer to the inner, C-compatible data structure.
    &MODULE_INFO.0
}

#[no_mangle]
pub extern "C" fn module_init(context: *const GitphCoreContext) -> GitphStatus {
    if context.is_null() {
        // Cannot log here as we have no context.
        return GitphStatus::ErrorInitFailed;
    }
    // Store the context and ensure the runtime is initialized.
    if let Ok(mut guard) = CORE_CONTEXT.lock() {
        // `ptr::read` safely copies the data from the C pointer.
        *guard = Some(unsafe { std::ptr::read(context) });
    } else {
        // Mutex was poisoned, a panic occurred while it was locked.
        return GitphStatus::ErrorInitFailed;
    }

    // Eagerly initialize the runtime by referencing it.
    let _ = &*RUNTIME;
    log_to_core(GitphLogLevel::Info, "issue_tracker module initialized.");
    GitphStatus::Success
}

#[no_mangle]
pub extern "C" fn module_exec(argc: c_int, argv: *const *const c_char) -> GitphStatus {
    // Safely parse arguments from C into a Vec<String>.
    let args: Vec<String> = (0..argc as isize)
        .map(|i| unsafe { CStr::from_ptr(*argv.offset(i)).to_string_lossy().into_owned() })
        .collect();

    if args.is_empty() {
        log_to_core(GitphLogLevel::Error, "Execution called with no arguments.");
        return GitphStatus::ErrorInvalidArgs;
    }

    let command = args[0].as_str();
    // Clone user_args to give it a 'static lifetime for the async block.
    let user_args = args[1..].to_vec();

    // Execute the async logic on the global runtime and block until it completes.
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