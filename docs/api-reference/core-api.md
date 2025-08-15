# Core API (C-ABI) Reference

This document provides a technical reference for the `gitph` C-ABI, which is the contract between the C core and all dynamically loaded modules. This API is defined in the header file `src/ipc/include/gitph_core_api.h`.

## Required Module Functions

Every `gitph` module **must** export the following four functions with C linkage (`extern "C"`).

---

### `const GitphModuleInfo* module_get_info(void)`

This function is called by the core immediately after the module is loaded. It should return a pointer to a static, read-only `GitphModuleInfo` struct containing metadata about the module.

- **Returns:** A pointer to a `GitphModuleInfo` struct. The pointer and its contents must remain valid for the lifetime of the application.

---

### `GitphStatus module_init(const GitphCoreContext* context)`

This function is called once at startup. It is where the module should perform any necessary initialization, such as allocating memory or setting up internal state. It receives a pointer to the `GitphCoreContext` which provides access to core services.

- **Parameters:**
  - `context` (const GitphCoreContext*): A pointer to the core context object. The module should store this context if it needs to call back into the core later.
- **Returns:** `GITPH_SUCCESS` on success, or an error code from the `GitphStatus` enum on failure.

---

### `GitphStatus module_exec(int argc, const char** argv)`

This function is called whenever a user runs a command that this module has registered (via `module_get_info`).

- **Parameters:**
  - `argc` (int): The number of arguments. `argv[0]` is the command name itself.
  - `argv` (const char**): An array of null-terminated strings representing the arguments.
- **Returns:** `GITPH_SUCCESS` on success, or an error code on failure.

---

### `void module_cleanup(void)`

This function is called once at shutdown. The module must free any resources it allocated during its lifetime to prevent memory leaks.

## Core Data Structures

These are the primary data structures used for communication between the core and the modules.

---

### `struct GitphModuleInfo`

A struct used to pass module metadata to the core.

- **Fields:**
  - `const char* name`: The unique name of the module (e.g., `"git_ops"`).
  - `const char* version`: The module's version string (e.g., `"1.0.0"`).
  - `const char* description`: A brief description of the module.
  - `const char** commands`: A `NULL`-terminated array of command strings that the module handles.

---

### `struct GitphCoreContext`

A struct passed from the core to the module during `module_init`. It contains function pointers that allow the module to safely access core services.

- **Fields (Function Pointers):**
  - `void (*log)(GitphLogLevel level, const char* module_name, const char* message)`: A pointer to the core's simple logging function.
  - `void (*log_fmt)(GitphLogLevel level, const char* module_name, const char* format, ...)`: A pointer to the core's safe, printf-style logging function. This is recommended over the simple `log` function for variable-length messages.
  - `const char* (*get_config_value)(const char* key)`: Retrieves a value from the core's configuration manager.
  - `void (*print_ui)(const char* text)`: Prints text to the user's console, managed by the core's UI system.

---

### `enum GitphStatus`

A standard set of status codes for return values.

- **Values:**
  - `GITPH_SUCCESS` (0)
  - `GITPH_ERROR_GENERAL` (-1)
  - `GITPH_ERROR_INVALID_ARGS` (-2)
  - `GITPH_ERROR_NOT_FOUND` (-3)
  - `GITPH_ERROR_INIT_FAILED` (-4)
  - `GITPH_ERROR_EXEC_FAILED` (-5)

---

### `enum GitphLogLevel`

Defines the severity levels for the logging system.

- **Values:**
  - `LOG_LEVEL_DEBUG`
  - `LOG_LEVEL_INFO`
  - `LOG_LEVEL_WARN`
  - `LOG_LEVEL_ERROR`
  - `LOG_LEVEL_FATAL`
