/* Copyright (C) 2025 Pedro Henrique / phkaiser13
 * lib.rs - FFI bridge and async entry point for the sync_engine module.
 * ... (comentários) ...
 * SPDX-License-Identifier: Apache-2.0 */

use libc::{c_char, c_int};
use std::ffi::{CStr, CString};
use std::sync::Mutex;

use lazy_static::lazy_static;
use tokio::runtime::Runtime;

mod sync;

// --- FFI Boilerplate (Types, Context, Logger) ---
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

struct SafeModuleInfo(GitphModuleInfo);
unsafe impl Sync for SafeModuleInfo {}

static MODULE_NAME: &[u8] = b"sync_engine\0";
static MODULE_VERSION: &[u8] = b"1.0.0\0";
static MODULE_DESC: &[u8] = b"Performs complex, bi-directional repository synchronization.\0";

// ✅ SUGESTÃO: Usar `const` em vez de `static` para o array de ponteiros. É o padrão mais seguro.
const SUPPORTED_COMMANDS_PTRS: &[*const c_char] = &[
    b"sync-run\0".as_ptr() as *const c_char,
    b"sync-config\0".as_ptr() as *const c_char,
    b"sync-status\0".as_ptr() as *const c_char,
    std::ptr::null(),
];

static MODULE_INFO: SafeModuleInfo = SafeModuleInfo(GitphModuleInfo {
    name: MODULE_NAME.as_ptr() as *const c_char,
    version: MODULE_VERSION.as_ptr() as *const c_char,
    description: MODULE_DESC.as_ptr() as *const c_char,
    commands: SUPPORTED_COMMANDS_PTRS.as_ptr(),
});

// --- FFI Function Implementations ---

#[no_mangle]
pub extern "C" fn module_get_info() -> *const GitphModuleInfo {
    &MODULE_INFO.0
}

#[no_mangle]
pub extern "C" fn module_init(context: *const GitphCoreContext) -> GitphStatus {
    if context.is_null() {
        return GitphStatus::ErrorInitFailed;
    }

    // ✅ SUGESTÃO: Usar `ptr::read` para consistência, sem depender de `Copy`.
    if let Ok(mut guard) = CORE_CONTEXT.lock() {
        *guard = Some(unsafe { std::ptr::read(context) });
    } else {
        return GitphStatus::ErrorInitFailed;
    }

    // ✅ CORREÇÃO: Força a inicialização do runtime de forma idiomática.
    {
        let _guard = RUNTIME.lock().unwrap();
    }

    log_to_core(GitphLogLevel::Info, "sync_engine module initialized.");
    GitphStatus::Success
}

#[no_mangle]
pub extern "C" fn module_exec(argc: c_int, argv: *const *const c_char) -> GitphStatus {
    if argv.is_null() || argc < 1 {
        log_to_core(GitphLogLevel::Error, "Execution failed: No command provided.");
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
