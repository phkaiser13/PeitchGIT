/* Copyright (C) 2025 Pedro Henrique / phkaiser13
 * lib.rs - FFI bridge and entry point for the git_ops Rust module.
 *
 * This file serves as the main library entry point and the Foreign Function
 * Interface (FFI) layer for the `git_ops` module. It is responsible for
 * exposing a C-compatible API that adheres to the contract defined in the
 * core `gitph_core_api.h` header.
 *
 * Key responsibilities:
 * - Defining C-compatible data structures (`#[repr(C)]`).
 * - Implementing the required module functions (`module_get_info`, `module_init`,
 *   `module_exec`, `module_cleanup`) with C calling conventions.
 * - Safely handling data transfer between the C core and Rust (e.g.,
 *   converting C strings to Rust strings).
 * - Dispatching commands received from the core to the appropriate Rust
 *   functions within the `commands` submodule.
 * - Translating the specific Rust `CommandError` enum into the generic
 *   C-compatible `GitphStatus` enum for the core.
 *
 * SPDX-License-Identifier: Apache-2.0 */

// Import libc for C-compatible types
use libc::{c_char, c_int};
use std::ffi::{CStr, CString};
use std::sync::Mutex;

// Declare the other modules in our crate.
mod commands;
mod git_wrapper;

// --- C API Data Structure Definitions ---
// These must match the definitions in `gitph_core_api.h` exactly.

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
#[derive(Debug)] // Add Debug for easier logging
pub enum GitphLogLevel {
    Debug,
    Info,
    Warn,
    Error,
    Fatal,
}

// A function pointer type for the logger callback from the C core.
type LogFn = extern "C" fn(GitphLogLevel, *const c_char, *const c_char);

#[repr(C)]
pub struct GitphCoreContext {
    log: Option<LogFn>,
    _get_config_value: Option<extern "C" fn()>,
    _print_ui: Option<extern "C" fn()>,
}

// Make the context globally and safely accessible within the Rust module.
lazy_static::lazy_static! {
    static ref CORE_CONTEXT: Mutex<Option<GitphCoreContext>> = Mutex::new(None);
}

// Helper function to log messages back to the C core.
fn log_to_core(level: GitphLogLevel, message: &str) {
    if let Ok(guard) = CORE_CONTEXT.lock() {
        if let Some(context) = &*guard {
            if let Some(log_fn) = context.log {
                let module_name = CString::new("GIT_OPS").unwrap();
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

// A wrapper to safely mark GitphModuleInfo as Sync.
struct SafeModuleInfo(GitphModuleInfo);
unsafe impl Sync for SafeModuleInfo {}

// Define the static data for our module's information.
static MODULE_NAME: &[u8] = b"git_ops\0";
static MODULE_VERSION: &[u8] = b"0.2.0\0"; // Updated version
static MODULE_DESC: &[u8] =
    b"Provides intelligent, workflow-aware Git commands like add, commit, push.\0";

const SUPPORTED_COMMANDS_PTRS: &[*const c_char] = &[
    b"SND\0".as_ptr() as *const c_char,
    b"status\0".as_ptr() as *const c_char,
    // Add other future commands here
    std::ptr::null(), // Null terminator for the C array
];

static MODULE_INFO: SafeModuleInfo = SafeModuleInfo(GitphModuleInfo {
    name: MODULE_NAME.as_ptr() as *const c_char,
    version: MODULE_VERSION.as_ptr() as *const c_char,
    description: MODULE_DESC.as_ptr() as *const c_char,
    commands: SUPPORTED_COMMANDS_PTRS.as_ptr(),
});

#[no_mangle]
pub extern "C" fn module_get_info() -> *const GitphModuleInfo {
    &MODULE_INFO.0
}

#[no_mangle]
pub extern "C" fn module_init(context: *const GitphCoreContext) -> GitphStatus {
    if context.is_null() {
        return GitphStatus::ErrorInitFailed;
    }
    if let Ok(mut guard) = CORE_CONTEXT.lock() {
        *guard = Some(unsafe { std::ptr::read(context) });
    } else {
        return GitphStatus::ErrorInitFailed;
    }
    log_to_core(
        GitphLogLevel::Info,
        "git_ops module initialized successfully.",
    );
    GitphStatus::Success
}

#[no_mangle]
pub extern "C" fn module_exec(argc: c_int, argv: *const *const c_char) -> GitphStatus {
    // Safely convert C-style arguments to a Rust `Vec<String>`.
    let args: Vec<String> = (0..argc as isize)
        .map(|i| unsafe {
            let c_str_ptr = *argv.offset(i);
            CStr::from_ptr(c_str_ptr).to_string_lossy().into_owned()
        })
        .collect();

    if args.is_empty() {
        log_to_core(
            GitphLogLevel::Error,
            "Execution called with no arguments.",
        );
        return GitphStatus::ErrorInvalidArgs;
    }

    // Dispatch to the correct command handler based on the first argument.
    let command = args[0].as_str();
    log_to_core(
        GitphLogLevel::Debug,
        &format!("Executing command: {}", command),
    );

    // The result is now a specific CommandResult from our commands module.
    let result = match command {
        // Pass the arguments to handle_send. `false` means we want user confirmation.
        "SND" => commands::handle_send(&args, false),
        "status" => commands::handle_status(),
        _ => {
            let err_msg = format!("Unknown command '{}' for git_ops module.", command);
            log_to_core(GitphLogLevel::Error, &err_msg);
            // Use the GitError variant for unknown commands.
            Err(commands::CommandError::GitError(err_msg))
        }
    };

    // Translate the specific CommandError into a generic GitphStatus for the C core.
    match result {
        Ok(success_message) => {
            // Log the success message from the command handler.
            log_to_core(GitphLogLevel::Info, &success_message);
            println!("{}", success_message); // Also print to user stdout
            GitphStatus::Success
        }
        Err(e) => match e {
            commands::CommandError::GitError(msg) => {
                log_to_core(GitphLogLevel::Error, &format!("Execution failed: {}", msg));
                GitphStatus::ErrorExecFailed
            }
            commands::CommandError::MissingCommitMessage => {
                let msg = "Missing commit message for 'SND' command. Usage: gitph SND <message>";
                log_to_core(GitphLogLevel::Error, msg);
                println!("{}", msg);
                GitphStatus::ErrorInvalidArgs
            }
            commands::CommandError::NoUpstreamConfigured => {
                let msg = "Current branch has no upstream configured. Cannot push.";
                log_to_core(GitphLogLevel::Error, msg);
                println!("{}", msg);
                GitphStatus::ErrorExecFailed
            }
            // These are not "errors" in the sense of failure, but successful "no-op" states.
            commands::CommandError::NoChanges => {
                let msg = "Working tree clean. No changes to send.";
                log_to_core(GitphLogLevel::Info, msg);
                println!("{}", msg);
                GitphStatus::Success
            }
            commands::CommandError::OperationAborted => {
                let msg = "Operation aborted by user.";
                log_to_core(GitphLogLevel::Info, msg);
                println!("{}", msg);
                GitphStatus::Success
            }
        },
    }
}

#[no_mangle]
pub extern "C" fn module_cleanup() {
    log_to_core(GitphLogLevel::Info, "git_ops module cleaned up.");
}