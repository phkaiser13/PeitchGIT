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
    static ref RUNTIME: Mutex<Runtime> = Mutex::new(Runtime::new().expect("Failed to create Tokio runtime"));
    static ref CORE_CONTEXT: Mutex<Option<GitphCoreContext>> = Mutex::new(None);
}

fn log_to_core(level: GitphLogLevel, message: &str) {
    if let Ok(guard) = CORE_CONTEXT.lock() {
        if let Some(context) = &*guard {
            if let Some(log_fn) = context.log {
                let module_name = CString::new("SYNC_ENGINE").unwrap();
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

// --- Module Metadata ---
#[repr(C)]
pub struct GitphModuleInfo {
    name: *const c_char,
    version: *const c_char,
    description: *const c_char,
    commands: *const *const c_char,
}

// --- CORREÇÃO 1: Wrapper para garantir Sync ---
struct SafeModuleInfo(GitphModuleInfo);
unsafe impl Sync for SafeModuleInfo {}

// --- CORREÇÃO 2: Usar dados verdadeiramente estáticos ---
static MODULE_NAME: &[u8] = b"sync_engine\0";
static MODULE_VERSION: &[u8] = b"1.0.0\0";
static MODULE_DESC: &[u8] = b"Performs complex, bi-directional repository synchronization.\0";

// Array estático de ponteiros para os comandos.
static SUPPORTED_COMMANDS_PTRS: &[*const c_char] = &[
    b"sync-run\0".as_ptr() as *const c_char,
    b"sync-config\0".as_ptr() as *const c_char,
    b"sync-status\0".as_ptr() as *const c_char,
    std::ptr::null(), // Terminador nulo
];

// O `lazy_static` agora contém apenas dados que são `Sync`.
lazy_static! {
    static ref MODULE_INFO: SafeModuleInfo = SafeModuleInfo(GitphModuleInfo {
        name: MODULE_NAME.as_ptr() as *const c_char,
        version: MODULE_VERSION.as_ptr() as *const c_char,
        description: MODULE_DESC.as_ptr() as *const c_char,
        commands: SUPPORTED_COMMANDS_PTRS.as_ptr(),
    });
}


// --- FFI Function Implementations ---

#[no_mangle]
pub extern "C" fn module_get_info() -> *const GitphModuleInfo {
    &MODULE_INFO.0
}

#[no_mangle]
pub extern "C" fn module_init(context: *const GitphCoreContext) -> GitphStatus {
    if context.is_null() {
        return GitphStatus::ErrorExecFailed;
    }
    *CORE_CONTEXT.lock().unwrap() = Some(unsafe { *context });
    let _ = RUNTIME.lock().unwrap();
    log_to_core(GitphLogLevel::Info, "sync_engine module initialized.");
    GitphStatus::Success
}

#[no_mangle]
pub extern "C" fn module_exec(argc: c_int, argv: *const *const c_char) -> GitphStatus {
    if argv.is_null() {
        return GitphStatus::ErrorExecFailed;
    }

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

    let result = runtime.block_on(async {
        match command {
            "sync-run" => sync::handle_run_sync(user_args).await,
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
