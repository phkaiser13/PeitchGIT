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
 * SPDX-License-Identifier: GPL-3.0-or-later */

use libc::{c_char, c_int};
use std::ffi::{CStr, CString};
use std::sync::Mutex;

use lazy_static::lazy_static;
use tokio::runtime::Runtime;

// The domain logic module we will create next.
mod sync;

// --- FFI Boilerplate (Types, Context, Logger) ---
#[repr(C)]
pub enum GitphStatus {
    Success = 0,
    ErrorExecFailed = -5, /* ... */
}
#[repr(C)]
pub enum GitphLogLevel {
    Info,
    Error, /* ... */
}
type LogFn = extern "C" fn(GitphLogLevel, *const c_char, *const c_char);
#[repr(C)]
#[derive(Clone, Copy)]
pub struct GitphCoreContext {
    log: Option<LogFn>, /* ... */
}

lazy_static! {
    static ref RUNTIME: Mutex<Runtime> = Mutex::new(Runtime::new().expect("Failed to create Tokio runtime"));
    static ref CORE_CONTEXT: Mutex<Option<GitphCoreContext>> = Mutex::new(None);
}

fn log_to_core(level: GitphLogLevel, message: &str) {
    if let Ok(guard) = CORE_CONTEXT.lock() {
        if let Some(context) = &*guard {
            if let Some(log_fn) = context.log {
                // It's safe to unwrap here because the module name is a valid C-string literal.
                let module_name = CString::new("SYNC_ENGINE").unwrap();
                // Attempt to create a CString from the message, handling potential null bytes.
                if let Ok(msg) = CString::new(message) {
                    log_fn(level, module_name.as_ptr(), msg.as_ptr());
                } else {
                    // If the message contains null bytes, log a fallback error message.
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

// A wrapper struct to hold the C-compatible info.
// We will implement Sync for this wrapper to tell the compiler it's safe
// for multi-threaded access, under the condition that its contents are
// truly static and read-only.
struct SyncGitphModuleInfo(GitphModuleInfo);

// SAFETY: This is safe because we guarantee that the data pointed to by the
// fields of `GitphModuleInfo` within this struct has a 'static lifetime
// and is read-only. The strings are from static byte literals, and the
// command list pointer comes from a `lazy_static` Vec that is never modified
// after creation.
unsafe impl Sync for SyncGitphModuleInfo {}

// Store module information in thread-safe, Rust-native static variables.
// The null terminator `\0` is included for C compatibility.
static MODULE_NAME: &[u8] = b"sync_engine\0";
static MODULE_VERSION: &[u8] = b"1.0.0\0";
static MODULE_DESC: &[u8] = b"Performs complex, bi-directional repository synchronization.\0";

// Define supported commands in a Rust-native, thread-safe way.
static SUPPORTED_COMMANDS: &[&[u8]] = &[
    b"sync-run\0",
    b"sync-config\0",
    b"sync-status\0",
];

lazy_static! {
    // This single static variable holds all our module metadata.
    // It's wrapped in `SyncGitphModuleInfo` for which we've implemented `Sync`,
    // assuring the compiler of its thread-safety.
    static ref STATIC_MODULE_INFO: SyncGitphModuleInfo = {
        // This vector holds the C-style list of command pointers. Because it's in a
        // `lazy_static`, its heap allocation will never be freed, making its internal
        // pointer stable and safe to share.
        static C_COMMANDS: Vec<*const c_char> = {
            let mut cmds: Vec<*const c_char> = SUPPORTED_COMMANDS
                .iter()
                .map(|s| s.as_ptr() as *const c_char)
                .collect();
            // C APIs often expect a null-terminated array of pointers.
            cmds.push(std::ptr::null());
            cmds
        };

        // Now, construct the GitphModuleInfo struct using pointers to our
        // static data.
        let info = GitphModuleInfo {
            name: MODULE_NAME.as_ptr() as *const c_char,
            version: MODULE_VERSION.as_ptr() as *const c_char,
            description: MODULE_DESC.as_ptr() as *const c_char,
            // The pointer to the command list is stable and 'static.
            commands: C_COMMANDS.as_ptr(),
        };

        // Finally, wrap it in our Sync-enabled type.
        SyncGitphModuleInfo(info)
    };
}

// --- FFI Function Implementations ---

#[no_mangle]
pub extern "C" fn module_get_info() -> *const GitphModuleInfo {
    // Return a pointer to the inner GitphModuleInfo struct.
    &STATIC_MODULE_INFO.0
}

#[no_mangle]
pub extern "C" fn module_init(context: *const GitphCoreContext) -> GitphStatus {
    // Defensively check for a null context pointer.
    if context.is_null() {
        return GitphStatus::ErrorExecFailed;
    }
    // This is safe because we are the only writer at init time.
    *CORE_CONTEXT.lock().unwrap() = Some(unsafe { *context });
    let _ = RUNTIME.lock().unwrap(); // Eagerly initialize.
    log_to_core(GitphLogLevel::Info, "sync_engine module initialized.");
    GitphStatus::Success
}

#[no_mangle]
pub extern "C" fn module_exec(argc: c_int, argv: *const *const c_char) -> GitphStatus {
    // Defensive check for null pointer.
    if argv.is_null() {
        return GitphStatus::ErrorExecFailed;
    }

    let args: Vec<String> = (0..argc as isize)
        .map(|i| unsafe {
            // Ensure the pointer at each index is not null before dereferencing.
            let arg_ptr = *argv.offset(i);
            if arg_ptr.is_null() {
                // Return an empty string for null arguments, or handle as an error.
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

    let result = runtime.block_on(async {
        match command {
            "sync-run" => sync::handle_run_sync(user_args).await,
            // Add other handlers as they are implemented in sync.rs
            // "sync-config" => sync::handle_config_sync(user_args).await,
            // "sync-status" => sync::handle_status_sync(user_args).await,
            _ => Err(format!("Unknown command '{}' for sync_engine", command)),
        }
    });

    match result {
        Ok(success_message) => {
            println!("{}", success_message);
            log_to_core(GitphLogLevel::Info, &success_message);
            GitphStatus::Success
        }
        Err(e) => {
            log_to_core(GitphLogLevel::Error, &format!("Command failed: {}", e));
            eprintln!("[gitph SYNC ERROR] {}", e);
            GitphStatus::ErrorExecFailed
        }
    }
}

#[no_mangle]
pub extern "C" fn module_cleanup() {
    log_to_core(GitphLogLevel::Info, "sync_engine module cleaned up.");
}