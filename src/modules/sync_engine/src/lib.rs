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
use once_cell::sync::Lazy;
use std::ffi::{CStr};
use std::ptr;
use std::slice;
use std::sync::Mutex;
use tokio::runtime::Runtime;

mod sync;

// --- Tipos FFI ---
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

#[repr(C)]
pub struct GitphModuleInfo {
    name: *const c_char,
    version: *const c_char,
    description: *const c_char,
    commands: *const *const c_char, // pointer to null-terminated array of *const c_char
}

// --- Estado Global (inicializado de forma segura) ---
static RUNTIME: Lazy<Mutex<Runtime>> = Lazy::new(|| {
    let rt = Runtime::new().expect("Failed to create Tokio runtime");
    Mutex::new(rt)
});

static CORE_CONTEXT: Lazy<Mutex<Option<GitphCoreContext>>> = Lazy::new(|| Mutex::new(None));

// Use literais de bytes com nul para construir &'static CStr sem alocação dinâmica.
static MODULE_NAME_BYTES: &[u8] = b"sync_engine\0";
static MODULE_VERSION_BYTES: &[u8] = b"1.0.0\0";
static MODULE_DESC_BYTES: &[u8] = b"Performs complex, bi-directional repository synchronization.\0";

static NAME_CSTR: Lazy<&'static CStr> = Lazy::new(|| unsafe {
    CStr::from_bytes_with_nul_unchecked(MODULE_NAME_BYTES)
});
static VERSION_CSTR: Lazy<&'static CStr> = Lazy::new(|| unsafe {
    CStr::from_bytes_with_nul_unchecked(MODULE_VERSION_BYTES)
});
static DESC_CSTR: Lazy<&'static CStr> = Lazy::new(|| unsafe {
    CStr::from_bytes_with_nul_unchecked(MODULE_DESC_BYTES)
});

// Comandos estáticos (ponteiros para bytes estáticos). O vetor é alocado uma vez e nunca mutado.
static SUPPORTED_COMMANDS_PTRS: Lazy<Vec<*const c_char>> = Lazy::new(|| {
    let mut v = Vec::with_capacity(4);
    // cada as_ptr() vem de um &'static CStr -> ponteiro válido por todo o tempo de execução
    v.push(unsafe { CStr::from_bytes_with_nul_unchecked(b"sync-run\0") }.as_ptr());
    v.push(unsafe { CStr::from_bytes_with_nul_unchecked(b"sync-config\0") }.as_ptr());
    v.push(unsafe { CStr::from_bytes_with_nul_unchecked(b"sync-status\0") }.as_ptr());
    v.push(ptr::null()); // terminador nulo como em C
    v
});

// GitphModuleInfo estático construído a partir dos pointers acima.
static MODULE_INFO: Lazy<GitphModuleInfo> = Lazy::new(|| GitphModuleInfo {
    name: NAME_CSTR.as_ptr(),
    version: VERSION_CSTR.as_ptr(),
    description: DESC_CSTR.as_ptr(),
    commands: SUPPORTED_COMMANDS_PTRS.as_ptr(),
});

// --- Função de log auxiliar ---
fn log_to_core(level: GitphLogLevel, message: &str) {
    if let Ok(mut guard) = CORE_CONTEXT.lock() {
        if let Some(context) = &*guard {
            if let Some(log_fn) = context.log {
                // module name já é um CStr estático
                let module_name = NAME_CSTR.as_ptr();
                // construir CStr temporário: convertemos para bytes com nul e usamos from_bytes_with_nul_unchecked
                // mas como message vem de Rust, temos que garantir que não contenha NULs.
                if message.as_bytes().contains(&0) {
                    let fallback = unsafe { CStr::from_bytes_with_nul_unchecked(b"Log message contained interior nul\0") };
                    log_fn(GitphLogLevel::Error, module_name, fallback.as_ptr());
                    return;
                }
                // criar uma CString exigiria alocação; em vez disso, crio um Vec com nul e uso as_ptr
                let mut buf = Vec::with_capacity(message.len() + 1);
                buf.extend_from_slice(message.as_bytes());
                buf.push(0);
                // segurança: buf é mantido só dentro da chamada
                let cptr = buf.as_ptr() as *const c_char;
                // note: buf será dropado no fim da função, mas o ponteiro é usado apenas dentro da chamada ao log_fn
                log_fn(level, module_name, cptr);
            }
        }
    }
}

// --- FFI Exports ---
#[no_mangle]
pub extern "C" fn module_get_info() -> *const GitphModuleInfo {
    &*MODULE_INFO as *const GitphModuleInfo
}

#[no_mangle]
pub extern "C" fn module_init(context: *const GitphCoreContext) -> GitphStatus {
    if context.is_null() {
        return GitphStatus::ErrorExecFailed;
    }

    // Segurança: copiar o contexto fornecido (é um Plain Old Data pequeno)
    unsafe {
        let ctx = *context;
        if let Ok(mut guard) = CORE_CONTEXT.lock() {
            *guard = Some(ctx);
        } else {
            return GitphStatus::ErrorExecFailed;
        }
    }

    // Garantir que runtime foi inicializado (Lazy já inicializa quando acessado)
    let _ = RUNTIME.lock().map_err(|_| GitphStatus::ErrorExecFailed);

    log_to_core(GitphLogLevel::Info, "sync_engine module initialized.");
    GitphStatus::Success
}

#[no_mangle]
pub extern "C" fn module_exec(argc: c_int, argv: *const *const c_char) -> GitphStatus {
    if argv.is_null() || argc < 1 {
        log_to_core(GitphLogLevel::Error, "Execution failed: argv is null or argc < 1");
        return GitphStatus::ErrorExecFailed;
    }

    // converte argv em slice seguro
    let raw_args: &[*const c_char] = unsafe { slice::from_raw_parts(argv, argc as usize) };

    // transforma em Vec<String> com atençao a ponteiros nulos
    let mut args = Vec::with_capacity(raw_args.len());
    for &p in raw_args.iter() {
        if p.is_null() {
            args.push(String::new());
        } else {
            // CStr::from_ptr é seguro se p vier de C; convertemos com to_string_lossy para evitar panics
            let s = unsafe { CStr::from_ptr(p) }.to_string_lossy().into_owned();
            args.push(s);
        }
    }

    if args.is_empty() || args[0].is_empty() {
        log_to_core(GitphLogLevel::Error, "Execution failed: No command provided.");
        return GitphStatus::ErrorExecFailed;
    }

    let command = args[0].as_str();
    let user_args = &args[1..];

    // pegar runtime
    let runtime_guard = match RUNTIME.lock() {
        Ok(g) => g,
        Err(_) => {
            log_to_core(GitphLogLevel::Error, "Failed to lock runtime mutex.");
            return GitphStatus::ErrorExecFailed;
        }
    };

    // executar a tarefa async bloqueando no runtime existente
    let result = runtime_guard.block_on(async {
        match command {
            "sync-run" => sync::handle_run_sync(user_args).await,
            "sync-config" => sync::handle_config(user_args).await,
            "sync-status" => sync::handle_status(user_args).await,
            other => Err(format!("Unknown command '{}' for sync_engine", other)),
        }
    });

    match result {
        Ok(success_message) => {
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