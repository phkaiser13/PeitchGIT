# Module Development Guide

This guide provides a comprehensive walkthrough for creating your own modules to extend `phgit`'s functionality. Modules are the primary way to add new, complex features to `phgit`. They are dynamically loaded shared libraries that can be written in any language that can export C symbols, though Rust is the recommended and most supported choice.

This guide will use the existing `git_ops` module as a real-world example.

## 1. The Module's Role

A `phgit` module is a self-contained unit of functionality. It is responsible for:
- Registering one or more commands with the `phgit` core.
- Executing the logic for those commands.
- Interacting with the core application (for logging, configuration, etc.) in a safe, controlled manner.

## 2. The FFI Contract: `phgit_core_api.h`

The foundation of module development is the C-ABI contract defined in `src/ipc/include/phgit_core_api.h`. This header file defines four functions that **every module must export** and the data structures used to communicate with the core.

### Required Exported Functions

- `PFN_module_get_info`: Returns metadata about the module (name, version, commands).
- `PFN_module_init`: Called once at startup to initialize the module. It receives the `phgitCoreContext`.
- `PFN_module_exec`: Called whenever a user runs one of the module's registered commands.
- `PFN_module_cleanup`: Called once at shutdown for the module to clean up its resources.

### Key Data Structures

- `phgitModuleInfo`: A struct containing your module's metadata.
- `phgitCoreContext`: A struct containing function pointers that your module can use to call back into the `phgit` core (e.g., for logging).
- `phgitStatus`: An enum for returning success or error codes back to the core.

## 3. Creating a Rust Module: A Step-by-Step Guide

Let's walk through how the `git_ops` module is implemented in Rust.

### Step 3.1: `Cargo.toml` Configuration

To compile a Rust project into a C-compatible shared library, you need to configure the crate type in your `Cargo.toml`.

```toml
[lib]
name = "git_ops"
crate-type = ["cdylib"] # Compile to a C-Dynamic Library
```

### Step 3.2: Replicating the C-ABI in Rust

Your `lib.rs` file must replicate the C data structures from `phgit_core_api.h`. You use the `#[repr(C)]` attribute to ensure the memory layout is compatible with C.

```rust
// src/modules/git_ops/src/lib.rs

// These must match the C definitions exactly.
#[repr(C)]
pub enum phgitStatus {
    Success = 0,
    ErrorGeneral = -1,
    // ... and so on
}

#[repr(C)]
pub struct phgitCoreContext {
    log: Option<extern "C" fn(...)>,
    // ... other function pointers
}

#[repr(C)]
pub struct phgitModuleInfo {
    name: *const libc::c_char,
    version: *const libc::c_char,
    description: *const libc::c_char,
    commands: *const *const libc::c_char,
}
```

### Step 3.3: Implementing `module_get_info`

This function returns a pointer to a static struct containing your module's information.

```rust
// src/modules/git_ops/src/lib.rs

// Define the static data for the module info.
// Note the null terminators (\0) for C compatibility.
static MODULE_NAME: &[u8] = b"git_ops\0";
static MODULE_VERSION: &[u8] = b"0.2.0\0";
static MODULE_DESC: &[u8] = b"Provides intelligent Git commands.\0";

// Define the commands this module handles.
// The list must be null-terminated.
const SUPPORTED_COMMANDS_PTRS: &[*const libc::c_char] = &[
    b"SND\0".as_ptr() as *const libc::c_char,
    b"status\0".as_ptr() as *const libc::c_char,
    std::ptr::null(), // Null terminator
];

// Create the static info struct.
static MODULE_INFO: phgitModuleInfo = phgitModuleInfo {
    name: MODULE_NAME.as_ptr() as *const libc::c_char,
    version: MODULE_VERSION.as_ptr() as *const libc::c_char,
    description: MODULE_DESC.as_ptr() as *const libc::c_char,
    commands: SUPPORTED_COMMANDS_PTRS.as_ptr(),
};

// Export the function with a C-compatible name.
#[no_mangle]
pub extern "C" fn module_get_info() -> *const phgitModuleInfo {
    &MODULE_INFO
}
```

### Step 3.4: Implementing `module_init`

This function receives the core context and should store it for later use. A `lazy_static` mutex is a good way to store this context safely in a global variable.

```rust
// src/modules/git_ops/src/lib.rs

lazy_static::lazy_static! {
    static ref CORE_CONTEXT: Mutex<Option<phgitCoreContext>> = Mutex::new(None);
}

#[no_mangle]
pub extern "C" fn module_init(context: *const phgitCoreContext) -> phgitStatus {
    if context.is_null() {
        return phgitStatus::ErrorInitFailed;
    }
    // Store the context for later use by the log_to_core helper function.
    if let Ok(mut guard) = CORE_CONTEXT.lock() {
        *guard = Some(unsafe { std::ptr::read(context) });
    } else {
        return phgitStatus::ErrorInitFailed;
    }
    // ...
    phgitStatus::Success
}
```

### Step 3.5: Implementing `module_exec`

This is the workhorse function. It receives the command-line arguments from the core, dispatches them to the correct internal Rust function, and translates the result back into a `phgitStatus`.

```rust
#[no_mangle]
pub extern "C" fn module_exec(argc: c_int, argv: *const *const c_char) -> phgitStatus {
    // 1. Safely convert C arguments to a Rust Vec<String>.
    let args: Vec<String> = (0..argc as isize)
        .map(|i| unsafe {
            let c_str_ptr = *argv.offset(i);
            CStr::from_ptr(c_str_ptr).to_string_lossy().into_owned()
        })
        .collect();

    // 2. Get the command (the first argument).
    let command = args[0].as_str();

    // 3. Dispatch to your internal Rust logic.
    let result = match command {
        "SND" => commands::handle_send(/* ... */),
        "status" => commands::handle_status(/* ... */),
        _ => Err(/* ... */),
    };

    // 4. Translate your internal Rust error type to a phgitStatus.
    match result {
        Ok(success_message) => {
            println!("{}", success_message); // Print to user
            phgitStatus::Success
        }
        Err(e) => {
            eprintln!("Error: {}", e); // Print error to user
            phgitStatus::ErrorExecFailed
        }
    }
}
```

### Step 3.6: Implementing `module_cleanup`

This function is for freeing any resources your module allocated during its lifetime.

```rust
#[no_mangle]
pub extern "C" fn module_cleanup() {
    // This is a good place to log a shutdown message.
    log_to_core(phgitLogLevel::Info, "git_ops module cleaned up.");
}
```

## 4. Building and Deploying the Module

To build your module, simply run `cargo build --release` in your module's directory. This will produce a shared library file:
- `target/release/libgit_ops.so` on Linux
- `target/release/libgit_ops.dylib` on macOS
- `target/release/git_ops.dll` on Windows

Finally, copy this shared library file into the `build/modules/` directory of your main `phgit` project. The next time you run `phgit`, the core will automatically discover, load, and register the commands from your new module.
