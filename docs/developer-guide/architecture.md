# phgit Architecture

`phgit` is built on a modular, polyglot, message-passing architecture. This design is a deliberate choice to use the best language for each task, promoting safety, performance, and extensibility. At its heart is a lightweight C core that acts as an orchestrator for a suite of external modules.

```mermaid
graph TD
    subgraph User Interaction
        A[CLI / TUI]
    end

    subgraph "phgit Core (C Language)"
        B[Orchestrator & Entry Point]
        B -- "Loads modules via FFI" --> C{Dynamic Module Loader}
        C -- "Loads" --> D[Rust Modules (.so, .dll, .dylib)]
        C -- "Loads" --> E[C++ Modules (.so, .dll, .dylib)]
        C -- "Initializes" --> F[Lua Scripting Engine]
        F -- "Executes" --> G[User Plugins (.lua)]
    end

    subgraph "Module & Plugin Ecosystem"
        D -- "Memory Safety, Concurrency" --> D1[git_ops]
        D -- "Stateful Logic, Performance" --> D2[sync_engine]
        D -- "Asynchronous I/O (Tokio)" --> D3[issue_tracker]
        E -- "RAII, OOP Design" --> E1[liblogger]
        G -- "User-Defined Aliases" --> G1[custom_aliases.lua]
        G -- "User-Defined Hooks" --> G2[pre_push_hook.lua]
    end

    A --> B
```

## Core Components

### The C Core (The Orchestrator)

The core of `phgit` is written in C for several key reasons:
- **Portability:** C has a minimal runtime and is the most portable language for system-level programming.
- **Stable ABI:** The C Application Binary Interface (ABI) is stable and well-understood, making it the perfect foundation for a Foreign Function Interface (FFI). This allows modules written in other languages (like Rust, C++, Go, etc.) to communicate with the core.
- **Control:** It provides low-level control over memory and process management, which is essential for an orchestrator that loads and manages other components.

The core is responsible for:
1.  **Parsing command-line arguments.**
2.  **Initializing subsystems** like logging and configuration.
3.  **Discovering and dynamically loading** all modules (shared libraries) from the `modules` directory.
4.  **Initializing the Lua scripting engine** and loading all user plugins.
5.  **Dispatching commands** to the appropriate module or Lua script.
6.  **Providing core services** (like logging) to modules via a well-defined API.

### The C++ Libraries (High-Performance Components)

C++ is used for specific, self-contained libraries where its features provide a clear advantage over C.
- **Object-Oriented Programming (OOP):** C++ allows for better encapsulation and design for complex components.
- **RAII (Resource Acquisition Is Initialization):** This C++ pattern simplifies resource management (memory, file handles, mutexes), leading to more robust and leak-free code. A prime example is the `liblogger` component, which uses RAII to manage log files and threading primitives safely.

### The Rust Modules (Safe and Concurrent Logic)

Rust is the language of choice for the majority of `phgit`'s business logic. All modules that handle complex operations, interact with the network, or perform critical tasks are written in Rust.
- **Memory Safety:** Rust's ownership and borrow checker guarantees memory safety at compile time, eliminating entire classes of bugs like null pointer dereferences, buffer overflows, and data races. This is critical for a tool that operates on user data and repositories.
- **Fearless Concurrency:** Rust makes it easier to write correct concurrent and asynchronous code, which is essential for modules like the `issue_tracker` that perform network requests without blocking the main application.
- **Performance:** Rust provides performance on par with C and C++, making it ideal for performance-sensitive tasks like the `sync_engine`.

### The Lua Scripts (User-Facing Extensibility)

Lua is embedded into the C core as a small, fast, and safe scripting language. It is not used for core logic but is provided as a way for **end-users** to customize and extend `phgit`'s behavior without needing to compile code.
- **Sandboxed:** The Lua environment is controlled by the C core, limiting what scripts can do and preventing them from crashing the main application.
- **Easy to Learn:** Lua has a simple syntax, making it accessible to users who are not professional programmers.
- **Dynamic:** Users can add or modify Lua scripts and have the changes reflected on the next run of `phgit`, without any need for recompilation.

## Communication: The FFI Contract

The magic that holds this polyglot system together is the **Foreign Function Interface (FFI)**. The core defines a strict, C-compatible API in the `phgit_core_api.h` header file. This file acts as a contract that all modules must adhere to.

- Any module, whether written in Rust, C++, or another language, must export a specific set of C-compatible functions.
- The C core uses `dlopen()` (or `LoadLibrary` on Windows) to load these modules as shared libraries at runtime.
- It then uses `dlsym()` (or `GetProcAddress`) to find the addresses of the required functions.
- When a module is initialized, the core passes it a `phgitCoreContext` struct. This struct contains function pointers back to core services, allowing the module to log messages or access configuration in a controlled way (a form of dependency injection).

This architecture makes `phgit` a true platform. New functionality can be added by simply dropping a new shared library into the `modules` directory, and the core will automatically discover and integrate it.
