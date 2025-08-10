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
    ErrorInvalidArgs = -2,
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
pub struct GitphCoreContext {
    log: Option<LogFn>,
}

// --- Global State Management ---
lazy_static! {
    // ✅ CORREÇÃO: O Runtime DEVE estar dentro de um Mutex para ser compartilhado
    // e para que o método .lock() exista.
    static ref RUNTIME: Mutex<Runtime> = Mutex::new(Runtime::new().expect("Failed to create Tokio runtime"));

    // O contexto também, pois é inicializado depois
    static ref CORE_CONTEXT: Mutex<Option<GitphCoreContext>> = Mutex::new(None);
}

// Helper for logging back to the C core.
fn log_to_core(level: GitphLogLevel, message: &str) {
    if let Ok(guard) = CORE_CONTEXT.lock() {
        if let Some(context) = guard.as_ref() {
            if let Some(log_fn) = context.log {
                let module_name = CString::new("ISSUE_TRACKER").unwrap();
                if let Ok(msg) = CString::new(message) {
                    log_fn(level, module_name.as_ptr(), msg.as_ptr());
                } else {
                    let error_msg = CString::new("Error: Log message contained null bytes.").unwrap();
                    log_fn(GitphLogLevel::Error, module_name.as_ptr(), error_msg.as_ptr());
                }
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

struct SafeModuleInfo(GitphModuleInfo);
unsafe impl Sync for SafeModuleInfo {}

static MODULE_NAME: &[u8] = b"issue_tracker\0";
static MODULE_VERSION: &[u8] = b"1.0.0\0";
static MODULE_DESC: &[u8] = b"Interacts with issue tracking services like GitHub Issues.\0";

const SUPPORTED_COMMANDS_PTRS: &[*const c_char] = &[
    b"issue-get\0".as_ptr() as *const c_char,
    std::ptr::null(),
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
        // O Mutex está "poisoned" (envenenado)
        return GitphStatus::ErrorInitFailed;
    }

    // Força a inicialização do lazy_static RUNTIME de forma idiomática.
    {
        let _guard = RUNTIME.lock().unwrap();
        // O lock é mantido até o fim deste bloco e depois liberado.
        // Isso satisfaz o compilador e garante a inicialização.
    }

    log_to_core(GitphLogLevel::Info, "issue_tracker module initialized.");
    GitphStatus::Success
}

#[no_mangle]
pub extern "C" fn module_exec(argc: c_int, argv: *const *const c_char) -> GitphStatus {
    if argv.is_null() || argc < 1 {
        log_to_core(GitphLogLevel::Error, "Execution called with no arguments.");
        return GitphStatus::ErrorInvalidArgs;
    }

    let args: Vec<String> = (0..argc as isize)
        .map(|i| unsafe {
            let arg_ptr = *argv.offset(i);
            CStr::from_ptr(arg_ptr).to_string_lossy().into_owned()
        })
        .collect();

    let command = args[0].as_str();
    let user_args = &args[1..];

    // Agora .lock() existe porque RUNTIME é um Mutex<Runtime>
    let runtime = RUNTIME.lock().unwrap();

    let result = runtime.block_on(async {
        match command {
            "issue-get" => issue_service::handle_get_issue(user_args).await,
            _ => {
                let err_msg = format!("Unknown command '{}' for issue_tracker module", command);
                // Logar o erro aqui é uma boa prática
                log_to_core(GitphLogLevel::Error, &err_msg);
                Err(err_msg)
            }
        }
    });

    match result {
        Ok(_) => GitphStatus::Success,
        Err(e) => {
            // O log do erro já pode ter sido feito dentro do match, mas podemos logar de novo
            // para garantir que a falha final seja registrada.
            log_to_core(GitphLogLevel::Error, &format!("Command execution failed: {}", e));
            GitphStatus::ErrorExecFailed
        }
    }
}

#[no_mangle]
pub extern "C" fn module_cleanup() {
    log_to_core(GitphLogLevel::Info, "issue_tracker module cleaned up.");
}
