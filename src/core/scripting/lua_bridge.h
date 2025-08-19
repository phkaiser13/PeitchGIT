/* Copyright (C) 2025 Pedro Henrique / phkaiser13
 * lua_bridge.h - Enhanced interface for the Lua scripting engine bridge.
 *
 * This header defines the comprehensive public API for interacting with the 
 * embedded Lua interpreter. This enhanced module provides:
 *
 * - Dynamic command registration from Lua scripts
 * - Comprehensive configuration management access
 * - Advanced hook system for event-driven plugins
 * - Rich utility functions for file system and environment access
 * - Performance-optimized memory management with exponential growth arrays
 *
 * The enhanced bridge exposes a powerful `phgit` table to Lua with functions like:
 * - `phgit.register_command()` - Register custom CLI commands
 * - `phgit.config_get()` / `phgit.config_set()` - Configuration management
 * - `phgit.register_hook()` - Event-driven plugin execution
 * - `phgit.file_exists()`, `phgit.getenv()` - System utilities
 *
 * This empowers users to create sophisticated extensions, complex workflows,
 * and dynamic command sets without requiring C/C++ compilation.
 *
 * SPDX-License-Identifier: Apache-2.0 */

#ifndef LUA_BRIDGE_H
#define LUA_BRIDGE_H

#include "../../ipc/include/phgit_core_api.h" // For phgitStatus
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initializes the enhanced Lua state and loads all plugin scripts.
 *
 * This function creates a new Lua state, opens the standard Lua libraries,
 * and registers the comprehensive set of C functions into the Lua global state
 * within a `phgit` table. It then scans the `plugins` directory and executes 
 * each `.lua` file found, allowing them to register custom commands, hooks,
 * and complex extension logic.
 *
 * Enhanced features:
 * - Configuration management access
 * - Dynamic command registration
 * - Hook-based event system
 * - File system and environment utilities
 *
 * @return phgit_SUCCESS on success, or an error code on failure.
 */
phgitStatus lua_bridge_init(void);

/**
 * @brief Executes a Lua-registered custom command.
 *
 * This function looks up and executes a command that was registered via
 * `phgit.register_command()` from a Lua script. It provides full argument
 * passing and return value handling for complex command logic.
 *
 * @param command_name The name of the registered command to execute.
 * @param argc The number of string arguments to pass to the command.
 * @param argv An array of string arguments.
 * @return phgit_SUCCESS if the command executed successfully, 
 *         phgit_ERROR_NOT_FOUND if the command doesn't exist,
 *         or phgit_ERROR_EXEC_FAILED if execution failed.
 */
phgitStatus lua_bridge_execute_command(const char* command_name, int argc, const char** argv);

/**
 * @brief Executes all functions registered for a specific hook.
 *
 * This function triggers all Lua functions that have been registered for a
 * specific hook via `phgit.register_hook()`. Hooks enable event-driven plugin
 * execution at critical points in the application lifecycle.
 *
 * Common hooks:
 * - "pre-commit" - Before committing changes
 * - "post-commit" - After successful commit
 * - "pre-push" - Before pushing to remote
 * - "post-checkout" - After checking out a branch
 *
 * @param hook_name The name of the hook to trigger.
 * @param argc The number of string arguments to pass to hook functions.
 * @param argv An array of string arguments.
 * @return phgit_SUCCESS if all hook functions executed successfully,
 *         phgit_ERROR_NOT_FOUND if no functions are registered for the hook,
 *         or phgit_ERROR_EXEC_FAILED if any function failed.
 */
phgitStatus lua_bridge_run_hook(const char* hook_name, int argc, const char** argv);

/**
 * @brief Checks if a command is registered with the Lua bridge.
 *
 * This function allows the CLI parser to determine if a command should be
 * handled by the Lua bridge rather than the native C command handlers.
 *
 * @param command_name The command name to check.
 * @return true if the command is registered, false otherwise.
 */
bool lua_bridge_has_command(const char* command_name);

/**
 * @brief Returns the number of Lua-registered commands.
 *
 * Useful for diagnostics and help system integration.
 *
 * @return The count of registered Lua commands.
 */
size_t lua_bridge_get_command_count(void);

/**
 * @brief Retrieves the description of a Lua-registered command.
 *
 * This enables the help system to display information about Lua commands
 * alongside native commands.
 *
 * @param command_name The name of the command.
 * @return Pointer to the description string, or NULL if command not found.
 *         The returned pointer is valid until lua_bridge_cleanup() is called.
 */
const char* lua_bridge_get_command_description(const char* command_name);

/**
 * @brief Closes the Lua state and frees all associated resources.
 *
 * This function performs comprehensive cleanup:
 * - Closes the Lua virtual machine
 * - Frees all command registry entries
 * - Cleans up hook registry and function lists
 * - Releases all dynamically allocated memory
 *
 * Must be called at application shutdown to ensure clean exit and prevent
 * memory leaks from both the Lua VM and the dynamic registries.
 */
void lua_bridge_cleanup(void);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // LUA_BRIDGE_H