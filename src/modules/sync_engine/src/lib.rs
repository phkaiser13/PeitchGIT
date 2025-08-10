/* Copyright (C) 2025 Pedro Henrique / phkaiser13
 * lib.rs - FFI bridge and async entry point for the sync_engine module.
 *
 * This file serves as the crucial FFI layer between the synchronous C core
 * and the highly complex, asynchronous Rust logic of the synchronization
 * engine. It upholds the established pattern of using a global Tokio runtime
 * to execute and block on async tasks, presenting a simple, synchronous
 * face to the outside world while leveraging modern async performance internally.
 *
 * It is responsible solely for argument translation and dispatching to the
 * `sync` module, which contains the actual state machine and Git object
 * database manipulation logic.
 *
 * SPDX-License-Identifier: Apache-2.0 */

use libc::{c_char, c_int};
use std::ffi::{CStr, CString};
use std::sync::Mutex;

use lazy_static::lazy_static;
use tokio::runtime::Runtime;

// The domain logic module containing the sync implementation.
mod sync;

// --- FFI Boilerplate (Types, Context, Logger) ---
#[repr(C)]
pub enum GitphStatus {
    Success = 0,
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
#[derive(Clone, Copy)]
pub struct GitphCoreContext {
    log: Option<LogFn>,
}

lazy_static! {
    // A single, global Tokio runtime to execute async tasks from a sync context.
    static ref RUNTIME: Mutex<Runtime> = Mutex::new(Runtime::new().expect("Failed to create Tokio runtime"));
    // The context received from the C core, stored safely in a Mutex.
    static ref CORE_CONTEXT: Mutex<Option<GitphCoreContext>> = Mutex::new(None);
}

// Helper function to safely log messages back to the C core.
fn log_to_core(level: GitphLogLevel, message: &str) {
    if let Ok(guard) = CORE_CONTEXT.lock() {
        if let Some(context) = &*guard {
            if let Some(log_fn) = context.log {
                let module_name = CString::new("SYNC_ENGINE").unwrap();
                if let Ok(msg) = CString::new(message) {
                    log_fn(level, module_name.as_ptr(), msg.as_ptr());
                } else {
                    // Handle cases where the log message contains null bytes.
                    let error_msg = CString::new("Error: Log message contained null bytes.").unwrap();
                    log_fn(GitphLogLevel::Error, module_name.as_ptr(), error_msg.as_ptr());
                }
            }
        }
    }
}

// --- Module Metadata ---

#[repr(C)]
pub struct GitphModuleInfo {
    name: *const c_char,
    version: *const c_char,
    description: *const c_char,
    commands: *const *const c_char,
}

// SOLUTION: This wrapper struct and the unsafe `Sync` implementation are the core of the fix.
// We are making an explicit promise to the compiler that, despite containing raw pointers,
// this struct is safe to be shared across threads because the data it points to
// is guaranteed to have a 'static lifetime and is read-only.
struct SafeModuleInfo(GitphModuleInfo);
unsafe impl Sync for SafeModuleInfo {}

// Define module metadata as static byte strings to ensure a 'static lifetime.
// The `b"...\0"` syntax creates a null-terminated byte slice.
static MODULE_NAME: &[u8] = b"sync_engine\0";
static MODULE_VERSION: &[u8] = b"1.0.0\0";
static MODULE_DESC: &[u8] = b"Performs complex, bi-directional repository synchronization.\0";

// This static array holds the pointers to our command strings. Because the byte strings
// are static, their pointers are also valid for the entire program's lifetime.
static SUPPORTED_COMMANDS_PTRS: &[*const c_char] = &[
    b"sync-run\0".as_ptr() as *const c_char,
    b"sync-config\0".as_ptr() as *const c_char,
    b"sync-status\0".as_ptr() as *const c_char,
    std::ptr::null(), // The C-style null terminator for the array.
];

// With the wrapper and static data, we can now safely create the static MODULE_INFO.
// This structure is fully `Sync`, resolving the compiler error.
static MODULE_INFO: SafeModuleInfo = SafeModuleInfo(GitphModuleInfo {
    name: MODULE_NAME.as_ptr() as *const c_char,
    version: MODULE_VERSION.as_ptr() as *const c_char,
    description: MODULE_DESC.as_ptr() as *const c_char,
    commands: SUPPORTED_COMMANDS_PTRS.as_ptr(),
});


// --- FFI Function Implementations ---

#[no_mangle]
pub extern "C" fn module_get_info() -> *const GitphModuleInfo {
    // Return a pointer to the wrapped GitphModuleInfo struct.
    &MODULE_INFO.0
}

#[no_mangle]
pub extern "C" fn module_init(context: *const GitphCoreContext) -> GitphStatus {
    if context.is_null() {
        // Cannot initialize without a valid context from the C core.
        return GitphStatus::ErrorExecFailed;
    }
    // Store the provided context and ensure the Tokio runtime is initialized.
    *CORE_CONTEXT.lock().unwrap() = Some(unsafe { *context });
    let _ = RUNTIME.lock().unwrap(); // Eagerly initialize the runtime.
    log_to_core(GitphLogLevel::Info, "sync_engine module initialized.");
    GitphStatus::Success
}

#[no_mangle]
pub extern "C" fn module_exec(argc: c_int, argv: *const *const c_char) -> GitphStatus {
    if argv.is_null() {
        return GitphStatus::ErrorExecFailed;
    }

    // Safely convert the C-style argc/argv into a Rust `Vec<String>`.
    let args: Vec<String> = (0..argc as isize)
        .map(|i| unsafe {
            let arg_ptr = *argv.offset(i);
            if arg_ptr.is_null() {
                String::new()
            } else {
                CStr::from_ptr(arg_ptr).to_string_lossy().into_owned()
            }
        })
        .collect();

    if args.is_empty() || args[0].is_empty() {
        log_to_core(GitphLogLevel::Error, "Execution failed: No command provided.");
        return GitphStatus::ErrorExecFailed;
    }

    let command = args[0].as_str();
    let user_args = &args[1..];
    let runtime = RUNTIME.lock().unwrap();

    // The bridge from the synchronous C world to the asynchronous Rust world.
    // `block_on` executes the async logic and waits for its result.
    let result = runtime.block_on(async {
        match command {
            "sync-run" => sync::handle_run_sync(user_args).await,
            // Add handlers for other commands like "sync-config" or "sync-status" here.
            _ => Err(format!("Unknown command '{}' for sync_engine", command)),
        }
    });

    // Translate the Rust `Result` into a C-style `GitphStatus`.
    match result {
        Ok(success_message) => {
            println!("{}", success_message);
            log_to_core(GitphLogLevel::Info, &success_message);
            GitphStatus::Success
        }
        Err(e) => {
            log_to_core(GitphLogLevel::Error, &format!("Command failed: {}", e));
            // Also print to stderr for immediate user feedback.
            eprintln!("[gitph SYNC ERROR] {}", e);
            GitphStatus::ErrorExecFailed
        }
    }
}

#[no_mangle]
pub extern "C" fn module_cleanup() {
    log_to_core(GitphLogLevel::Info, "sync_engine module cleaned up.");
    // Resources like the Mutex and Runtime are cleaned up automatically
    // when the application exits. No explicit cleanup is needed here.
}