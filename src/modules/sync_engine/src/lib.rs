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

// MELHORIA 1: O `Mutex` em volta do `Runtime` foi removido.
// RAZÃO: `tokio::runtime::Runtime` já é thread-safe. Seus métodos podem ser
// chamados de múltiplas threads sem a necessidade de um Mutex externo,
// o que remove um ponto de contenção e simplifica o código.
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

// FIX 1: Criar a estrutura wrapper para garantir `Sync`.
// RAZÃO: Esta é a solução idiomática para informar ao compilador que os ponteiros
// brutos dentro de `GitphModuleInfo` são seguros para compartilhamento, pois
// apontam para dados estáticos e imutáveis.
struct SafeModuleInfo(GitphModuleInfo);
unsafe impl Sync for SafeModuleInfo {}

static MODULE_NAME: &[u8] = b"sync_engine\0";
static MODULE_VERSION: &[u8] = b"1.0.0\0";
static MODULE_DESC: &[u8] = b"Performs complex, bi-directional repository synchronization.\0";
static SUPPORTED_COMMANDS: &[*const u8] = &[
    b"sync-run\0".as_ptr(),
    b"sync-config\0".as_ptr(),
    b"sync-status\0".as_ptr(),
    std::ptr::null(),
];

// FIX 2: Usar o wrapper `SafeModuleInfo` para a variável estática.
static MODULE_INFO: SafeModuleInfo = SafeModuleInfo(GitphModuleInfo {
    name: MODULE_NAME.as_ptr() as *const c_char,
    version: MODULE_VERSION.as_ptr() as *const c_char,
    description: MODULE_DESC.as_ptr() as *const c_char,
    commands: SUPPORTED_COMMANDS.as_ptr() as *const *const c_char,
});

// --- FFI Function Implementations ---

#[no_mangle]
pub extern "C" fn module_get_info() -> *const GitphModuleInfo {
    // FIX 3: Retornar um ponteiro para o dado interno do wrapper.
    &MODULE_INFO.0
}

#[no_mangle]
pub extern "C" fn module_init(context: *const GitphCoreContext) -> GitphStatus {
    // MELHORIA 2: Tratamento de erro robusto para o Mutex.
    // RAZÃO: Evita um `panic` na fronteira FFI se o Mutex estiver "envenenado",
    // o que é um comportamento indefinido.
    if context.is_null() {
        return GitphStatus::ErrorInitFailed;
    }
    if let Ok(mut guard) = CORE_CONTEXT.lock() {
        *guard = Some(unsafe { std::ptr::read(context) });
    } else {
        return GitphStatus::ErrorInitFailed;
    }

    // Eagerly initialize a referência ao runtime.
    let _ = &*RUNTIME;
    log_to_core(GitphLogLevel::Info, "sync_engine module initialized.");
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
    // MELHORIA 3: Clonar os argumentos para o bloco async.
    // RAZÃO: Garante que os dados passados para o bloco `async` tenham um tempo de
    // vida `'static`, evitando erros de compilação relacionados a lifetimes.
    let user_args = args[1..].to_vec();

    let result = RUNTIME.block_on(async {
        match command {
            "sync-run" => sync::handle_run_sync(&user_args).await,
            // Adicione outros handlers aqui conforme forem implementados
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