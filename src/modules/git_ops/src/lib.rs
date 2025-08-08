/* Copyright (C) 2025 Pedro Henrique
 * lib.rs - FFI bridge and entry point for the git_ops Rust module.
 *
 * This file serves as the main library entry point and the Foreign Function
 * Interface (FFI) layer for the `git_ops` module. It is responsible for
 * exposing a C-compatible API that adheres to the contract defined in the
 * core `gitph_core_api.h` header.
 *
 * Key responsibilities:
 * - Defining C-compatible data structures (`#[repr(C)]`).
 * - Implementing the five required module functions (`module_get_info`,
 *   `module_init`, `module_exec`, `module_cleanup`) with C calling conventions.
 * - Safely handling data transfer between the C core and Rust (e.g.,
 *   converting C strings to Rust strings).
 * - Dispatching commands received from the core to the appropriate Rust
 *   functions within the `commands` submodule.
 * - Managing a global state, such as the context received from the core,
 *   in a thread-safe manner using a Mutex.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later */

// Import libc for C-compatible types
use libc::{c_char, c_int};
use std::ffi::{CStr, CString};
use std::sync::Mutex;

// Declare the other modules in our crate. We will create these files next.
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
pub enum GitphLogLevel {
    Debug,
    Info,
    Warn,
    Error,
    Fatal,
}

// A function pointer type for the logger callback from the C core.
type LogFn = extern "C" fn(GitphLogLevel, *const c_char, *const c_char);

// FIX 1: Replace raw data pointers `*const ()` with proper optional function pointers.
// REASON: `Option<extern "C" fn()>` is Send + Sync, unlike `*const ()`. This makes
// the entire GitphCoreContext struct Send + Sync, resolving the lazy_static! error.
#[repr(C)]
pub struct GitphCoreContext {
    log: Option<LogFn>,
    // Add other context functions here as they are defined in the C API.
    _get_config_value: Option<extern "C" fn()>, // Placeholder for future use
    _print_ui: Option<extern "C" fn()>,         // Placeholder for future use
}

// Make the context globally and safely accessible within the Rust module.
// This now works because GitphCoreContext is Send.
lazy_static::lazy_static! {
    static ref CORE_CONTEXT: Mutex<Option<GitphCoreContext>> = Mutex::new(None);
}

// Helper function to log messages back to the C core.
// This abstracts away the FFI complexity for the rest of the Rust code.
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

// FIX 2: Create a wrapper struct for GitphModuleInfo.
// REASON: GitphModuleInfo contains raw pointers, so it is not `Sync`. By wrapping it
// and implementing `unsafe impl Sync`, we are telling the compiler that we guarantee
// it's safe to be shared across threads. This is true because the pointers inside
// will only ever point to other 'static read-only data.
struct SafeModuleInfo(GitphModuleInfo);
unsafe impl Sync for SafeModuleInfo {}

// Define the static data for our module's information.
// `b"...\0"` creates null-terminated C-compatible byte strings with a 'static lifetime.
static MODULE_NAME: &[u8] = b"git_ops\0";
static MODULE_VERSION: &[u8] = b"1.0.0\0";
static MODULE_DESC: &[u8] = b"Provides fundamental Git commands like add, commit, push.\0";

// Criamos um array estático de ponteiros para os nossos comandos.
// Este array em si terá um tempo de vida 'static.
const SUPPORTED_COMMANDS_PTRS: &[*const c_char] = &[
    b"SND\0".as_ptr() as *const c_char,
    b"rls\0".as_ptr() as *const c_char,
    b"psor\0".as_ptr() as *const c_char,
    b"cnb\0".as_ptr() as *const c_char,
    b"cb\0".as_ptr() as *const c_char,
    b"status\0".as_ptr() as *const c_char,
    std::ptr::null(), // Terminador nulo para o array, conforme o padrão C.
];

// Agora, podemos criar com segurança o MODULE_INFO estático dentro do nosso wrapper `Sync`.
// O campo `commands` agora aponta para o nosso array de ponteiros estático.
static MODULE_INFO: SafeModuleInfo = SafeModuleInfo(GitphModuleInfo {
    name: MODULE_NAME.as_ptr() as *const c_char,
    version: MODULE_VERSION.as_ptr() as *const c_char,
    description: MODULE_DESC.as_ptr() as *const c_char,
    // O ponteiro para o array de ponteiros é seguro para ser compartilhado.
    commands: SUPPORTED_COMMANDS_PTRS.as_ptr(),
});

#[no_mangle]
pub extern "C" fn module_get_info() -> *const GitphModuleInfo {
    // Return a pointer to the actual GitphModuleInfo struct inside our wrapper.
    &MODULE_INFO.0
}

#[no_mangle]
pub extern "C" fn module_init(context: *const GitphCoreContext) -> GitphStatus {
    if context.is_null() {
        return GitphStatus::ErrorInitFailed;
    }
    // Safely store the received context in our global static variable.
    if let Ok(mut guard) = CORE_CONTEXT.lock() {
        // We create a copy of the context data.
        // This is safe because the lifetime of `context` is guaranteed by the C core
        // to be valid during this call.
        *guard = Some(unsafe { std::ptr::read(context) });
    } else {
        // This would happen if the mutex is "poisoned" (a thread panicked while holding the lock).
        return GitphStatus::ErrorInitFailed;
    }
    log_to_core(GitphLogLevel::Info, "git_ops module initialized successfully.");
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
        log_to_core(GitphLogLevel::Error, "Execution called with no arguments.");
        return GitphStatus::ErrorInvalidArgs;
    }

    // Dispatch to the correct command handler based on the first argument.
    let command = args[0].as_str();
    log_to_core(GitphLogLevel::Debug, &format!("Executing command: {}", command));

    let result = match command {
        "SND" => commands::handle_send(),
        "status" => commands::handle_status(),
        // Add other command handlers here...
        _ => {
            log_to_core(GitphLogLevel::Error, &format!("Unknown command '{}' for git_ops module.", command));
            Err("Unknown command".to_string())
        }
    };

    match result {
        Ok(_) => GitphStatus::Success,
        Err(e) => {
            log_to_core(GitphLogLevel::Error, &format!("Command failed: {}", e));
            GitphStatus::ErrorExecFailed
        }
    }
}

#[no_mangle]
pub extern "C" fn module_cleanup() {
    log_to_core(GitphLogLevel::Info, "git_ops module cleaned up.");
}