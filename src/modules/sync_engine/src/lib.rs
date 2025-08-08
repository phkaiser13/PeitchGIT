/* Copyright (C) 2025 Pedro Henrique
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
#[derive(Copy, Clone)]
pub struct GitphCoreContext {
    log: Option<LogFn>,
    _get_config_value: Option<extern "C" fn()>,
    _print_ui: Option<extern "C" fn()>,
}

// IMPROVEMENT 1: The `Mutex` around `Runtime` was removed.
// REASON: `tokio::runtime::Runtime` is already thread-safe. Its methods can be
// called from multiple threads without needing an external Mutex,
// which removes a point of contention and simplifies the code.
lazy_static! {
    static ref RUNTIME: Runtime = Runtime::new().expect("Failed to create Tokio runtime");
    static ref CORE_CONTEXT: Mutex<Option<GitphCoreContext>> = Mutex::new(None);
}

fn log_to_core(level: GitphLogLevel, message: &str) {
    if let Ok(guard) = CORE_CONTEXT.lock() {
        if let Some(context) = &*guard {
            if let Some(log_fn) = context.log {
                let module_name = CString::new("SYNC_ENGINE").unwrap();
                let msg = CString::new(message).unwrap();
                log_fn(level, module_name.as_ptr(), msg.as_ptr());
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

// FIX 1: Create the wrapper structure to guarantee `Sync`.
// REASON: This is the idiomatic solution to inform the compiler that the raw
// pointers inside `GitphModuleInfo` are safe for sharing because they
// point to static and immutable data.  1 
struct SafeModuleInfo(GitphModuleInfo);
unsafe impl Sync for SafeModuleInfo {}

static MODULE_NAME: &[u8] = b"sync_engine\0";
static MODULE_VERSION: &[u8] = b"1.0.0\0";
static MODULE_DESC: &[u8] = b"Performs complex, bi-directional repository synchronization.\0";

// CORREÇÃO: A definição de `SUPPORTED_COMMANDS` foi reestruturada.
// RAZÃO: O erro original ocorria porque um array de ponteiros brutos (`*const u8`)
// não é `Sync`.  4  A solução é definir as strings como `&'static [u8]` (que são `Sync`)
// e então criar um array estático de ponteiros para esses dados. Isso garante
// que os ponteiros são para memória estática e imutável, tornando a estrutura
// segura para compartilhamento entre threads.
static CMD_SYNC_RUN: &[u8] = b"sync-run\0";
static CMD_SYNC_CONFIG: &[u8] = b"sync-config\0";
static CMD_SYNC_STATUS: &[u8] = b"sync-status\0";

static SUPPORTED_COMMANDS: [*const c_char; 4] = [
    CMD_SYNC_RUN.as_ptr() as *const c_char,
    CMD_SYNC_CONFIG.as_ptr() as *const c_char,
    CMD_SYNC_STATUS.as_ptr() as *const c_char,
    std::ptr::null(), // Null terminator for the C-style array of strings
];

// FIX 2: Use the `SafeModuleInfo` wrapper for the static variable.
static MODULE_INFO: SafeModuleInfo = SafeModuleInfo(GitphModuleInfo {
    name: MODULE_NAME.as_ptr() as *const c_char,
    version: MODULE_VERSION.as_ptr() as *const c_char,
    description: MODULE_DESC.as_ptr() as *const c_char,
    // Now we use the pointer to our well-defined static array.
    commands: SUPPORTED_COMMANDS.as_ptr(),
});

// --- FFI Function Implementations ---

#[no_mangle]
pub extern "C" fn module_get_info() -> *const GitphModuleInfo {
    // FIX 3: Return a pointer to the inner data of the wrapper.
    &MODULE_INFO.0
}

#[no_mangle]
pub extern "C" fn module_init(context: *const GitphCoreContext) -> GitphStatus {
    // IMPROVEMENT 2: Robust error handling for the Mutex.
    // REASON: Avoids a `panic` at the FFI boundary if the Mutex is "poisoned",
    // which is undefined behavior.
    if context.is_null() {
        return GitphStatus::ErrorInitFailed;
    }
    if let Ok(mut guard) = CORE_CONTEXT.lock() {
        // This is unsafe because we are dereferencing a raw pointer from C.
        // We trust the C caller to provide a valid pointer.
        *guard = Some(unsafe { std::ptr::read(context) });
    } else {
        return GitphStatus::ErrorInitFailed;
    }

    // Eagerly initialize the reference to the runtime.
    let _ = &*RUNTIME;
    log_to_core(GitphLogLevel::Info, "sync_engine module initialized.");
    GitphStatus::Success
}

#[no_mangle]
pub extern "C" fn module_exec(argc: c_int, argv: *const *const c_char) -> GitphStatus {
    // This is unsafe because we are dereferencing raw pointers from C.
    // We trust the C caller to provide valid, null-terminated strings.
    let args: Vec<String> = (0..argc as isize)
        .map(|i| unsafe { CStr::from_ptr(*argv.offset(i)).to_string_lossy().into_owned() })
        .collect();

    if args.is_empty() {
        log_to_core(GitphLogLevel::Error, "Execution called with no arguments.");
        return GitphStatus::ErrorInvalidArgs;
    }

    let command = args 0 .as_str();
    // IMPROVEMENT 3: Clone the arguments for the async block.
    // REASON: Ensures that the data passed to the `async` block has a `'static`
    // lifetime, avoiding compilation errors related to lifetimes.
    let user_args = args[1..].to_vec();

    let result = RUNTIME.block_on(async {
        match command {
            "sync-run" => sync::handle_run_sync(&user_args).await,
            // Add other handlers here as they are implemented
            // "sync-config" => sync::handle_config_sync(&user_args).await,
            // "sync-status" => sync::handle_status_sync(&user_args).await,
            _ => Err(format!("Unknown command '{}' for sync_engine", command)),
        }
    });

    match result {
        Ok(success_message) => {
            log_to_core(GitphLogLevel::Info, &success_message);
            GitphStatus::Success
        }
        Err(e) => {
            let error_message = format!("Command failed: {}", e);
            log_to_core(GitphLogLevel::Error, &error_message);
            GitphStatus::ErrorExecFailed
        }
    }
}

#[no_mangle]
pub extern "C" fn module_cleanup() {
    log_to_core(GitphLogLevel::Info, "sync_engine module cleaned up.");
}