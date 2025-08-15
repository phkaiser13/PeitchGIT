# Core Development Guide

This guide is for advanced developers who intend to modify or contribute to the `gitph` C core. The core is the central orchestrator of the application, responsible for loading modules, managing subsystems, and dispatching commands.

## 1. Code Structure

The C core source code is located in `src/core/`. It is organized into several subdirectories, each with a specific responsibility:

- `main/`: Contains the main entry point of the application (`main.c`).
- `cli/`: Handles parsing of command-line arguments and dispatching them to the correct module.
- `config/`: Manages loading and accessing configuration files.
- `module_loader/`: Responsible for discovering and dynamically loading shared library modules.
- `scripting/`: Contains the bridge to the embedded Lua engine.
- `ui/`: Implements the interactive Text-based User Interface (TUI).
- `platform/`: Contains platform-specific code (e.g., for Windows vs. POSIX systems) to ensure portability.

## 2. The Application Lifecycle (`main.c`)

The application's entry point, `main()`, provides a clear overview of the startup and shutdown sequence. Understanding this sequence is critical for core development.

### Startup Sequence

The core initializes subsystems in a specific, non-negotiable order to ensure dependencies are met.

1.  `platform_global_init()`: Performs any required platform-specific setup first.
2.  `logger_init()`: The logging system is initialized early so that all subsequent subsystems can log their status.
3.  `config_load()`: The configuration is loaded. This is not fatal if it fails; the application can proceed with default values.
4.  `lua_bridge_init()`: The Lua scripting engine is initialized. This must happen before modules are loaded, as modules might depend on Lua being available.
5.  `modules_load()`: The module loader searches the `./modules` directory for shared libraries, loads them, and registers their commands. This is a critical step; if it fails, the application will exit.

After initialization, the core inspects the command-line arguments (`argc`):
- If `argc < 2`, no command was provided, so `tui_show_main_menu()` is called to enter interactive mode.
- Otherwise, the arguments are passed to `cli_dispatch_command()` to be executed.

### Shutdown Sequence

A `goto cleanup` pattern is used to ensure a graceful shutdown. The cleanup functions are called in the reverse order of initialization to safely deallocate all resources.

1.  `modules_cleanup()`: Calls the `module_cleanup` function in every loaded module.
2.  `lua_bridge_cleanup()`: Releases all resources used by the Lua engine.
3.  `config_cleanup()`: Frees memory used by the configuration manager.
4.  `logger_cleanup()`: Flushes and closes the log file.
5.  `platform_global_cleanup()`: Performs any platform-specific cleanup.

## 3. The Build System (`CMake` and `Makefile`)

`gitph` uses `CMake` as its primary build system, with a `Makefile` wrapper for convenience.

- **`CMakeLists.txt` (Root):** This is the main build script. It finds the necessary compilers and libraries, and it includes the `src` and `tests` directories.
- **`src/CMakeLists.txt`:** This script defines the `gitph` executable. It links the core C source files together and ensures that the C++ `liblogger` is also compiled and linked. It also specifies the directories where headers can be found.
- **`Makefile`:** This provides simple, user-friendly commands like `make`, `make clean`, and `make install`. It is a wrapper around the more complex `cmake` commands, making the build process easier for developers. For example, `make` will automatically create a `build` directory, run `cmake`, and then run the actual build command (`make` or `ninja`).

When you run `make`, the build system compiles the C core into an executable (`build/bin/gitph`) and also places the compiled Rust/C++ modules (which are built separately via Cargo or other means) into the `build/modules/` directory, from where the executable can load them at runtime.

## 4. Coding Conventions

The C code in the core follows a few consistent conventions:

- **Comments:** The code is extensively commented using C-style `/* ... */` blocks. Header comments explain the purpose of the file, and function headers use a Doxygen-like format to explain parameters and return values.
- **Naming:** Functions and variables use `snake_case`. Header guards use the pattern `GITPH_DIR_FILE_H`.
- **Error Handling:** Functions that can fail typically return a `GitphStatus` enum. This provides a consistent way to handle errors across the application.
- **Headers:** Each `.c` file has a corresponding `.h` file that declares its public interface. Internal or static functions are not exposed in the header.
