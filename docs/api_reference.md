# gitph API Reference

This document provides a detailed reference for the `gitph` API, covering the core C application, its polyglot modules, and the scripting interface. It is intended for developers who wish to extend, maintain, or understand the inner workings of the system.

## Table of Contents

1.  [Architectural Overview](https://www.google.com/search?q=%231-architectural-overview)
2.  [Core C API Contract (`gitph_core_api.h`)](https://www.google.com/search?q=%232-core-c-api-contract-gitph_core_apih)
      * [Enums](https://www.google.com/search?q=%23enums)
      * [Structs](https://www.google.com/search?q=%23structs)
      * [Module Function Pointers](https://www.google.com/search?q=%23module-function-pointers)
3.  [Core C Subsystems](https://www.google.com/search?q=%233-core-c-subsystems)
      * [Module Loader (`loader.h`)](https://www.google.com/search?q=%23module-loader-loaderh)
      * [CLI Parser (`cli_parser.h`)](https://www.google.com/search?q=%23cli-parser-cli_parserh)
      * [Configuration Manager (`config_manager.h`)](https://www.google.com/search?q=%23configuration-manager-config_managerh)
      * [Platform Abstraction Layer (`platform.h`)](https://www.google.com/search?q=%23platform-abstraction-layer-platformh)
      * [Lua Scripting Bridge (`lua_bridge.h`)](https://www.google.com/search?q=%23lua-scripting-bridge-lua_bridgeh)
      * [Text-based UI (`tui.h`)](https://www.google.com/search?q=%23text-based-ui-tuih)
4.  [Shared Libraries](https://www.google.com/search?q=%234-shared-libraries)
      * [Logger (`Logger.h`)](https://www.google.com/search?q=%23logger-loggerh)
      * [Common Utilities (`utils.h`)](https://www.google.com/search?q=%23common-utilities-utilsh)
5.  [Polyglot Modules API](https://www.google.com/search?q=%235-polyglot-modules-api)
      * [Rust Modules](https://www.google.com/search?q=%23rust-modules)
          * [`git_ops`](https://www.google.com/search?q=%23git_ops-rust)
          * [`issue_tracker`](https://www.google.com/search?q=%23issue_tracker-rust)
          * [`sync_engine`](https://www.google.com/search?q=%23sync_engine-rust)
      * [Go Modules](https://www.google.com/search?q=%23go-modules)
          * [`api_client`](https://www.google.com/search?q=%23api_client-go)
          * [`ci_cd_manager`](https://www.google.com/search?q=%23ci_cd_manager-goc)
          * [`devops_automation`](https://www.google.com/search?q=%23devops_automation-go)
6.  [Inter-Process Communication (IPC)](https://www.google.com/search?q=%236-inter-process-communication-ipc)
      * [CI/CD Workflow Data](https://www.google.com/search?q=%23cicd-workflow-data)
7.  [Lua Scripting API](https://www.google.com/search?q=%237-lua-scripting-api)

-----

## 1\. Architectural Overview

`gitph` utilizes a polyglot architecture centered around a lightweight **C core**. This core acts as an orchestrator, responsible for loading, managing, and dispatching commands to a suite of external modules. These modules are compiled as C-compatible shared libraries (`.so`, `.dll`) and can be written in any language that can export C symbols, such as C++, Rust, and Go.

This design provides several key advantages:

  * **Modularity:** Features are encapsulated within independent modules, which can be developed, tested, and updated separately.
  * **Best Tool for the Job:** Each language is used for its specific strengths:
      * **C:** Low-level system control, application lifecycle, and dynamic loading.
      * **C++:** High-performance, object-oriented libraries (e.g., logging, visualization).
      * **Rust:** Memory-safe, high-performance, and critical logic (e.g., Git operations, stateful sync).
      * **Go:** Excellent concurrency for network-bound tasks and CLI wrappers (e.g., API clients, DevOps tools).
  * **Extensibility:** New functionality can be added simply by creating a new module that adheres to the Core API Contract.
  * **User Customization:** An embedded Lua scripting engine allows users to add their own aliases and hooks without recompiling the application.

The entire system is bound by a single C header file, `src/ipc/include/gitph_core_api.h`, which defines the contract all modules must follow.

## 2\. Core C API Contract (`gitph_core_api.h`)

This header is the single most important file in the project. It defines the stable interface between the C core and all dynamically loaded modules.

### Enums

#### `GitphStatus`

A consistent set of status codes used across the application for error handling.

```c
typedef enum {
    GITPH_SUCCESS = 0,
    GITPH_ERROR_GENERAL = -1,
    GITPH_ERROR_INVALID_ARGS = -2,
    GITPH_ERROR_NOT_FOUND = -3,
    GITPH_ERROR_INIT_FAILED = -4,
    GITPH_ERROR_EXEC_FAILED = -5
} GitphStatus;
```

#### `GitphLogLevel`

Defines the severity levels for messages passed to the core logging system.

```c
typedef enum {
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARN,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_FATAL
} GitphLogLevel;
```

### Structs

#### `GitphModuleInfo`

A struct returned by modules to describe their identity and capabilities. The pointers within must remain valid for the application's lifetime.

```c
typedef struct {
    const char* name;         // Unique module name (e.g., "git_ops")
    const char* version;      // Version string (e.g., "1.0.0")
    const char* description;  // Brief purpose of the module
    const char** commands;    // NULL-terminated array of command strings
} GitphModuleInfo;
```

#### `GitphCoreContext`

An object passed from the core to each module during initialization. It provides the module with safe access to core functionalities via function pointers (a form of dependency injection).

```c
typedef struct {
    void (*log)(GitphLogLevel level, const char* module_name, const char* message);
    const char* (*get_config_value)(const char* key);
    void (*print_ui)(const char* text);
} GitphCoreContext;
```

### Module Function Pointers

Every module **must** export five C functions that match these typedefs. The module loader resolves these functions by name.

  * **`PFN_module_get_info`**: `const GitphModuleInfo* (*)(void)`
      * Returns a pointer to the module's static metadata.
  * **`PFN_module_init`**: `GitphStatus (*)(const GitphCoreContext* context)`
      * Initializes the module, receiving the core context. Called once.
  * **`PFN_module_exec`**: `GitphStatus (*)(int argc, const char** argv)`
      * Executes a command. `argv[0]` is the command name.
  * **`PFN_module_cleanup`**: `void (*)(void)`
      * Cleans up all resources used by the module. Called once on shutdown.

## 3\. Core C Subsystems

The C core is composed of several distinct subsystems, each with a clear responsibility.

### Module Loader (`loader.h`)

This is the heart of the dynamic architecture. It discovers, loads, validates, and manages modules.

  * `GitphStatus modules_load(const char* directory_path)`: Scans a directory for shared libraries (`.so`, `.dll`), loads them, resolves the five required functions, calls their `module_init`, and adds them to a global registry.
  * `const LoadedModule* modules_find_handler(const char* command)`: Searches the registry to find which module handles a given command string.
  * `const LoadedModule** modules_get_all(int* count)`: Returns a list of all successfully loaded modules, primarily for the TUI to build its menu.
  * `void modules_cleanup(void)`: Calls each module's `cleanup_func` and unloads the library from memory.

### CLI Parser (`cli_parser.h`)

This module is the central dispatcher for all non-interactive commands.

  * `void cli_dispatch_command(int argc, const char** argv)`: Takes command-line arguments, uses `modules_find_handler` to find the responsible module, and invokes its `exec_func`. It does not contain any business logic itself, making the core highly extensible.

### Configuration Manager (`config_manager.h`)

Manages loading and accessing settings from a `key=value` configuration file.

  * `GitphStatus config_load(const char* filename)`: Parses a configuration file (e.g., `gitph.conf`), ignoring comments and whitespace, and stores the settings in an internal hash table for efficient lookup.
  * `const char* config_get_value(const char* key)`: Retrieves a configuration value by its key.
  * `void config_cleanup(void)`: Frees all memory used by the configuration manager.

### Platform Abstraction Layer (`platform.h`)

Isolates all OS-specific code, allowing the rest of the application to be portable. It defines macros like `PATH_SEPARATOR` and `MODULE_EXTENSION`.

  * `bool platform_global_init(void)`: On Windows, this is critical for enabling ANSI escape sequence support in the console. On POSIX systems, it may be a no-op.
  * `void platform_global_cleanup(void)`: Restores any system state changed by `platform_global_init`.
  * `void platform_clear_screen(void)`: Clears the terminal screen.
  * `bool platform_get_home_dir(char* buffer, size_t buffer_size)`: Safely retrieves the user's home directory path.

### Lua Scripting Bridge (`lua_bridge.h`)

Manages the embedded Lua interpreter for user-level extensibility.

  * `GitphStatus lua_bridge_init(void)`: Initializes the Lua VM, registers custom C functions into the global `gitph` table, and loads all `.lua` scripts from the `plugins` directory.
  * `GitphStatus lua_bridge_run_hook(const char* function_name, int argc, const char** argv)`: Allows the C core to call specific global Lua functions, enabling an event-driven hook system (e.g., `on_pre_push`).
  * `void lua_bridge_cleanup(void)`: Closes the Lua state and frees all associated resources.

### Text-based UI (`tui.h`)

Handles the interactive menu-driven interface.

  * `void tui_show_main_menu(void)`: The main entry point for interactive mode. It dynamically builds a menu from the commands exposed by all loaded modules and enters a loop to prompt the user and dispatch their choices.
  * `void tui_print_error(const char* message)`: Prints a consistently formatted error message.
  * `bool tui_prompt_user(const char* prompt, char* buffer, size_t buffer_size)`: Displays a prompt and safely reads a line of user input.

## 4\. Shared Libraries

These are internal libraries used by the C core and potentially other modules.

### Logger (`Logger.h`)

A robust, thread-safe logging system written in **C++** but exposed via a C-compatible interface.

  * **C++ Class**: `Logger` is a thread-safe singleton that handles file I/O and message formatting. It uses `std::mutex` to prevent race conditions.
  * **C API**:
      * `int logger_init(const char* filename)`: Initializes the logger.
      * `void logger_log(GitphLogLevel level, const char* module_name, const char* message)`: The primary logging function exposed to all C code and other modules via the `GitphCoreContext`.
      * `void logger_cleanup()`: Closes the log file.

### Common Utilities (`utils.h`)

A foundational C library providing safe and reusable utility functions.

  * `char* common_safe_strdup(const char* s)`: A memory-safe version of `strdup` that terminates the program on allocation failure.
  * `char* common_path_join(const char* base, const char* leaf)`: Intelligently joins two path components with the correct platform-specific separator.
  * `char* common_read_file(const char* filepath, size_t* file_size)`: Reads an entire file into a dynamically allocated, null-terminated buffer.

## 5\. Polyglot Modules API

Each module is a self-contained unit of functionality. They adhere to the Core API Contract and are loaded at runtime.

### Rust Modules

#### `git_ops` (Rust)

Provides fundamental Git operations by wrapping the `git` command-line tool.

  * **Source**: `src/modules/git_ops/`
  * **Commands**:
      * `status`: Shows a concise, machine-readable view of the repository status (`git status --porcelain`).
      * `SND`: A high-level "Send" command that automatically stages all changes (`git add .`), commits with a default message, and pushes to the default remote/branch (`git push origin main`).
  * **Internal Logic**: The `git_wrapper.rs` file abstracts the execution of `std::process::Command`, providing clean functions like `git_add()`, `git_commit()`, etc. The `commands.rs` file orchestrates these wrapper functions to implement the business logic for each command.

#### `issue_tracker` (Rust)

An asynchronous client for interacting with issue tracking services like GitHub Issues.

  * **Source**: `src/modules/issue_tracker/`
  * **Commands**:
      * `issue-get <user/repo> <issue_id>`: Fetches and displays the details for a specific issue from a GitHub repository.
  * **Internal Logic**:
      * Uses the `tokio` runtime to perform asynchronous network requests without blocking the main application.
      * Leverages the `reqwest` library for making HTTP calls to the GitHub API.
      * Defines a generic `IssueTrackerService` trait, allowing for future extension to other providers (e.g., Jira, GitLab). The current implementation, `GitHubApiService`, fetches issue data and maps it to a provider-agnostic `Issue` struct.

#### `sync_engine` (Rust)

A sophisticated, stateful engine for performing bi-directional repository synchronization.

  * **Source**: `src/modules/sync_engine/`
  * **Commands**:
      * `sync-run <path_to_source_repo> <path_to_target_repo>`: Initiates the synchronization process.
  * **Internal Logic**:
      * Uses the `git2-rs` library for direct, low-level manipulation of the Git object database, rather than wrapping the CLI. This allows for complex commit graph analysis.
      * Maintains a persistent state by serializing a `SyncState` struct to a JSON file (`.git/gitph_sync_state.json`) within the source repository. This allows the engine to remember the last common ancestor between syncs.
      * The engine can detect divergence (when both repos have unique commits) and will safely abort, requiring manual intervention.

### Go Modules

#### `api_client` (Go)

A client for interacting with Git provider APIs, such as GitHub.

  * **Source**: `src/modules/api_client/`
  * **Commands**:
      * `srp <user/repo>` ("Set Repository"): Fetches and displays metadata about a public GitHub repository, including stars, forks, and description.
  * **Internal Logic**:
      * Built as a C shared library using `cgo`. The `client.go` file implements the FFI bridge.
      * The `github_handler.go` file contains the business logic. It defines a generic `ApiProvider` interface and a concrete `GitHubHandler` implementation, making it extensible to other providers.
      * Uses Go's standard `net/http` and `encoding/json` packages to perform the API call and parse the response.

#### `ci_cd_manager` (Go/C++)

A two-part module for visualizing CI/CD workflows.

  * **Source**: `src/modules/ci_cd_manager/`
  * **Go Parser (`parser.go`):**
      * This component is compiled as a C shared library (`ci_cd_parser.so`).
      * It exports a single function: `ParseWorkflowToJSON(*C.char) *C.char`.
      * This function takes a file path to a YAML workflow file (e.g., a GitHub Actions `.yml` file), parses it using the `gopkg.in/yaml.v3` library, and serializes the structured data into a JSON string.
      * The caller (the C core) is responsible for freeing the memory allocated for the returned C string.
  * **C++ Visualizer (`visualizer/`):**
      * This component is also compiled as a C shared library (`ci_cd_visualizer.so`).
      * It exports a function: `visualize_pipeline_from_json(const char* json_c_str)`.
      * It takes the JSON string generated by the Go parser, uses the `nlohmann/json` library to parse it into a C++ object model, and then prints a formatted, human-readable representation of the pipeline to the console.

#### `devops_automation` (Go)

A wrapper for common DevOps command-line tools like Terraform and Vault.

  * **Source**: `src/modules/devops_automation/`
  * **Commands**:
      * `tf-plan`: Executes `terraform plan` and streams the output directly to the console in real-time.
      * `tf-apply`: Executes `terraform apply` for interactive IaC workflows.
      * `vault-read <secret_path>`: Reads a secret from Vault, parses the JSON output, and displays the key-value pairs.
  * **Internal Logic**:
      * Provides two process execution strategies: `runCommandWithStreaming` for interactive commands and `runCommandAndCapture` for commands that return structured data that needs to be parsed.
      * A central `module_exec` function acts as a dispatcher, routing commands to the appropriate handler (e.g., `handleTerraformPlan`, `handleVaultRead`).

## 6\. Inter-Process Communication (IPC)

While most module interaction is arbitrated by the C core, some components require direct data exchange.

### CI/CD Workflow Data

The `ci_cd_manager` module is a prime example of IPC within `gitph`.

1.  A Go program (`parser.go`) reads a complex `.yml` file.
2.  It parses this YAML into Go structs and then marshals these structs into a **JSON string**. This JSON acts as a stable, language-agnostic data contract.
3.  The C core passes this JSON string to a C++ program (`PipelineVisualizer.cpp`).
4.  The C++ program deserializes the JSON into its own object model and renders the visual output.

A Protocol Buffers schema (`rpc_data.proto`) is also defined in the project, which provides a more efficient and strongly-typed alternative to JSON for future inter-module communication needs.

## 7\. Lua Scripting API

Users can extend `gitph` by placing `.lua` scripts in the `plugins/` directory. The C core executes these scripts at startup, allowing them to register new functionality. A global table named `gitph` is exposed to the Lua environment.

  * `gitph.log(level, message)`: Allows a Lua script to write to the main `gitph` log file. `level` is a string (e.g., "INFO", "ERROR").
      * **Example**: `gitph.log("INFO", "My custom plugin has loaded.")`
  * `gitph.register_alias(alias, command)`: Registers a new command alias.
      * **Example**: `gitph.register_alias("st", "status")` allows the user to run `gitph st` as a shortcut for `gitph status`.
  * **Event Hooks**: Scripts can define global functions that are called by the C core at specific lifecycle events.
      * **Example**: Defining `function on_pre_push(remote, branch)` in a script allows it to run checks before a push operation. Returning `false` from this function will cancel the push.
