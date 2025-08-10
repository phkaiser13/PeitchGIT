/* Copyright (C) 2025 Pedro Henrique / phkaiser13
 * lua_bridge.h - Interface for the Lua scripting engine bridge.
 *
 * This header defines the public API for interacting with the embedded Lua
 * interpreter. This module is responsible for initializing a Lua state,
 * loading and executing user-provided scripts from the `plugins` directory,
 * and exposing core application functionalities to those scripts.
 *
 * By exposing functions like `gitph.log()` or `gitph.register_command()`, we
 * empower users to create powerful extensions, custom workflows, and aliases
 * directly in a simple scripting language. This is a key feature for making
 * gitph highly customizable.
 *
 * SPDX-License-Identifier: Apache-2.0 */

#ifndef LUA_BRIDGE_H
#define LUA_BRIDGE_H

#include "../../ipc/include/gitph_core_api.h" // For GitphStatus

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initializes the Lua state and loads all plugin scripts.
 *
 * This function creates a new Lua state, opens the standard Lua libraries,
 * and registers custom C functions into the Lua global state (e.g., inside a
 * `gitph` table). It then scans the `plugins` directory and executes each
 * `.lua` file found, allowing them to register custom commands or hooks.
 *
 * @return GITPH_SUCCESS on success, or an error code on failure.
 */
GitphStatus lua_bridge_init(void);

/**
 * @brief Executes a specific function within the loaded Lua scripts.
 *
 * This function can be used to trigger "hooks" at specific points in the
 * application's lifecycle (e.g., "pre-push", "post-commit").
 *
 * @param function_name The name of the global Lua function to call.
 * @param argc The number of string arguments to pass to the Lua function.
 * @param argv An array of string arguments.
 * @return GITPH_SUCCESS if the function was called successfully, or an error
 *         code if the function doesn't exist or an error occurred during
 *         its execution.
 */
GitphStatus lua_bridge_run_hook(const char* function_name, int argc, const char** argv);

/**
 * @brief Closes the Lua state and frees all associated resources.
 *
 * This must be called at application shutdown to ensure a clean exit and
 * prevent memory leaks from the Lua virtual machine.
 */
void lua_bridge_cleanup(void);


#ifdef __cplusplus
} // extern "C"
#endif

#endif // LUA_BRIDGE_H