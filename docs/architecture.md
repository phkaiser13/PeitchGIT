# Ph Git (peight git) - System Architecture

## 1. Introduction

This document provides a detailed architectural overview of `gitph`, a polyglot command-line tool designed to be a modern, extensible, and high-performance Git and DevOps assistant.

The core philosophy of `gitph` is to leverage the unique strengths of multiple programming languages to build a system where each component is optimized for its specific task. This results in a modular, scalable, and efficient application that combines the low-level control of C, the safety and performance of Rust, the concurrency of Go, and the scripting flexibility of Lua.

## 2. High-Level Architectural Diagram

The architecture is centered around a lightweight **C Core** that acts as an orchestrator. It dynamically loads and communicates with a suite of external modules, which are self-contained shared libraries.


    +---------------------------------------------------------------------------------+
    |                                  User Interface                                 |
    |                      (CLI Parser / Interactive TUI)                             |
    +---------------------------------------------------------------------------------+
    |
    | (Calls)
    v
    +---------------------------------------------------------------------------------+
    |                                 THE C CORE (Orchestrator)                       |
    |---------------------------------------------------------------------------------|
    |  main.c           - Application entry point & lifecycle management              |
    |  cli_parser.c     - Dispatches commands to the correct module                   |
    |  module_loader.c  - Dynamically loads/unloads shared libraries (.so, .dll)      |
    |  config_manager.c - Manages settings from .gitph.conf                          |
    |  lua_bridge.c     - Manages the Lua scripting engine for plugins                |
    |  platform.c       - OS-specific abstractions (Windows/POSIX)                    |
    +---------------------------------------------------------------------------------+
    ^                   |                                |
    | (Callbacks)       | (Loads & Executes)             | (Provides Context)
    |                   v                                v
    +--------------------+--------------------------------+--------------------------+
    |   Shared Libraries |      The FFI & IPC Backbone      |      Plugin System       |
    |--------------------|--------------------------------|--------------------------|
    | - liblogger (C++)  | - gitph_core_api.h (C Contract)  | - Lua Scripts (.lua)     |
    | - libcommon (C)    | - JSON/Protobuf (Data Exchange)|   - Custom Aliases       |
    |                    |                                |   - Event Hooks          |
    +--------------------+--------------------------------+--------------------------+
    |
    | (Loads & Executes)
    v
    +---------------------------------------------------------------------------------+
    |                               THE MODULE ECOSYSTEM                              |
    |---------------------------------------------------------------------------------|
    | [Rust Modules]                  | [Go Modules]                  | [C++ Modules] |
    | - git_ops (Git CLI wrapper)     | - api_client (GitHub API)     | - ci_cd_visualizer |
    | - issue_tracker (Async client)| - devops_automation (Terraform) |               |
    | - sync_engine (libgit2-based)   | - ci_cd_parser (YAML -> JSON) |               |
    +---------------------------------------------------------------------------------+


## 3. The C Core: The Orchestrator

The C core (`src/core/`) is the central nervous system of `gitph`. It is lightweight and has no business logic of its own. Its sole purpose is to manage the application's lifecycle and orchestrate the interactions between the user and the various functional modules.

### Key Subsystems of the C Core:

* **`main` (`src/core/main/`)**: The application's entry point. It initializes all core subsystems in the correct order (platform, logger, config, modules, Lua), determines whether to run in interactive (TUI) or command-line mode, and ensures a graceful shutdown by cleaning up all resources.

* **Module Loader (`src/core/module_loader/`)**: This is the heart of the dynamic architecture. It is responsible for:
    1.  Scanning the `./modules` directory for shared libraries (`.so` on Linux, `.dll` on Windows).
    2.  Using platform-specific APIs (`dlopen`/`LoadLibrary`) to load these libraries into memory.
    3.  **Validating** that each library conforms to the `gitph_core_api.h` contract by resolving pointers to five required functions (`module_get_info`, `module_init`, `module_exec`, `module_cleanup`).
    4.  Calling the module's `init` function, passing it the `GitphCoreContext` so the module can access core services like logging.
    5.  Maintaining a global registry of all successfully loaded modules.

* **CLI Parser (`src/core/cli/`)**: When `gitph` is run with arguments (e.g., `gitph SND`), this component takes over. It identifies the command and queries the Module Loader to find which module has registered that command. It then invokes that module's `exec` function, passing along the relevant arguments.

* **Configuration Manager (`src/core/config/`)**: This subsystem parses and provides access to settings from a `.gitph.conf` file. It uses a simple hash table for efficient `key=value` lookups, allowing modules to retrieve configuration values through the `GitphCoreContext`.

* **Lua Scripting Bridge (`src/core/scripting/`)**: This component embeds a Lua interpreter into the application. It exposes a curated set of C functions to the Lua environment (e.g., `gitph.log()`), allowing users to write simple scripts to extend `gitph`'s functionality with custom aliases and hooks.

* **Platform Abstraction Layer (`src/core/platform/`)**: This isolates all operating system-specific code. It provides a consistent interface for tasks like clearing the terminal screen, finding the user's home directory, and enabling ANSI color support on Windows.

* **Text-based UI (`src/core/ui/`)**: When `gitph` is run without arguments, it enters an interactive mode managed by the TUI. The TUI dynamically generates a menu of options by querying the Module Loader for all commands from all loaded modules.

## 4. The Communication Backbone: FFI and IPC

The ability for components written in different languages to communicate seamlessly is the most critical aspect of this architecture.

### 4.1. The C-ABI Contract (`gitph_core_api.h`)

This single C header file is the **immutable contract** that binds the entire system together. It defines a C Application Binary Interface (ABI) that any language can target. Every module, regardless of its source language (Rust, Go, C++), must be compiled into a shared library that exports five specific C functions:

1.  `module_get_info()`: Returns metadata about the module.
2.  `module_init()`: Sets up the module.
3.  `module_exec()`: Executes a command.
4.  `module_cleanup()`: Frees resources.

This contract ensures that the C core can load and interact with any module in a standardized way, without needing to know the language it was written in.

### 4.2. Data Serialization (JSON)

For more complex data exchange between modules, `gitph` uses **JSON** as a language-agnostic interchange format. The `ci_cd_manager` is a prime example:

1.  The **Go parser** reads a complex YAML workflow file and serializes it into a well-defined JSON string.
2.  This JSON string is passed back to the C core.
3.  The C core then passes this string to the **C++ visualizer**, which deserializes the JSON into its own object model for rendering.

This decouples the parser from the visualizer; they only need to agree on the JSON schema, not on language-specific data structures. The `rpc_data.proto` file also exists to support a more efficient binary format (Protocol Buffers) for future IPC needs.

## 5. The Module Ecosystem

Modules are where all the business logic of `gitph` resides. Each is chosen for its specific strengths.

### 5.1. Rust Modules (`src/modules/{git_ops, issue_tracker, sync_engine}/`)

**Why Rust?** For performance-critical tasks that require memory safety and low-level system control without the manual memory management of C/C++.

* **`git_ops`**: Provides fundamental Git commands (`status`, `SND`). It acts as an intelligent wrapper around the system's `git` executable, orchestrating multiple Git calls into a single user command.
* **`issue_tracker`**: An asynchronous client for fetching data from services like GitHub Issues. It uses the `tokio` runtime and `reqwest` library to perform non-blocking network I/O, ensuring the UI remains responsive during API calls.
* **`sync_engine`**: The most complex module. It performs bi-directional repository synchronization by using the `git2-rs` library for direct, programmatic access to the Git object database. This allows it to perform sophisticated commit graph analysis that would be impossible by just wrapping the CLI. It maintains its own persistent state in a JSON file to track sync progress.

### 5.2. Go Modules (`src/modules/{api_client, ci_cd_manager, devops_automation}/`)

**Why Go?** For its excellent concurrency model (goroutines), rich standard library for networking and JSON, and fast compilation, making it ideal for building network clients and CLI tool wrappers.

* **`api_client`**: A client for interacting with Git provider APIs (e.g., GitHub). It uses Go's standard `net/http` package to fetch repository data.
* **`ci_cd_manager` (Parser)**: As described in the IPC section, this Go component is responsible for parsing YAML workflow files. Go's excellent support for YAML and JSON makes it a perfect fit for this data transformation task.
* **`devops_automation`**: A wrapper for external DevOps tools like `terraform` and `vault`. It showcases two execution strategies: streaming I/O for interactive commands (`terraform plan`) and capturing output for data that needs to be parsed (`vault read`).

### 5.3. C++ Modules (`src/modules/ci_cd_manager/visualizer/`)

**Why C++?** For creating high-performance components that can leverage complex third-party libraries and the power of object-oriented design with RAII for safe resource management.

* **`ci_cd_visualizer`**: This component receives a JSON string from the Go parser and uses the `nlohmann/json` library to deserialize it into a C++ object model. It then renders a structured, human-readable view of the CI/CD pipeline, demonstrating C++'s strength in data modeling and structured output.

## 6. Build System (`CMakeLists.txt`)

Orchestrating a multi-language build is complex. `gitph` uses **CMake** as its primary build system. The top-level `CMakeLists.txt` is the "conductor" that:
1.  Compiles the C/C++ core and shared libraries.
2.  Defines custom commands to invoke the native toolchains for other languages:
    * It calls `cargo build --release` to compile the Rust modules into `.so`/`.dll` files.
    * It calls `go build -buildmode=c-shared` to compile the Go modules.
3.  Ensures all compiled artifacts (the main executable and all module libraries) are placed in the correct output directories (`build/bin/` and `build/bin/modules/`).

A convenience `Makefile` is provided as a simple wrapper around the more verbose CMake commands.

## 7. Extensibility via Lua Plugins

The final layer of the architecture is user-level extensibility. The **Lua Scripting Bridge** (`lua_bridge.c`) loads all `.lua` files from the `src/plugins/` directory at startup. These scripts can interact with the `gitph` core through a safe, exposed API. This allows users to:

* **Create Custom Aliases**: Map short, memorable names to longer `gitph` commands.
* **Implement Event Hooks**: Define functions like `on_pre_push(remote, branch)` that are automatically called by the core at specific points in the execution flow. This allows users to enforce custom policies (e.g., run a linter, check commit message format) before a push occurs.

This powerful feature allows users to tailor `gitph` to their specific workflows without needing to recompile the entire application.
