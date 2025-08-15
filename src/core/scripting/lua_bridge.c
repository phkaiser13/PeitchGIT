/* Copyright (C) 2025 Pedro Henrique / phkaiser13
 * lua_bridge.c - Implementation of the Lua scripting engine bridge.
 *
 * This file implements the bridge between the C core and the Lua scripting
 * engine. It manages the lifecycle of the Lua VM, exposes a curated set of
 * safe C functions to the Lua environment, and loads user-provided scripts.
 * This version has been corrected to eliminate all fixed-size log buffers,
 * using the safe `logger_log_fmt` function to prevent buffer overflows when
 * reporting errors from the Lua subsystem.
 *
 * The core of the bridge is a global table named `gitph` that is injected
 * into the Lua state. This table contains functions that scripts can call,
 * such as `gitph.log()`. These C functions are carefully written to interact
 * with the Lua stack, pulling arguments and pushing return values. This
 * provides a powerful yet controlled way for users to extend the application.
 *
 * SPDX-License-Identifier: Apache-2.0 */

#include "lua_bridge.h"
#include "libs/liblogger/Logger.h"
#include "platform/platform.h"
#include "cli/cli_parser.h"
#include <string.h>

// Lua headers
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>


// Platform-specific includes for directory traversal
#ifdef PLATFORM_WINDOWS
#include <windows.h>
#else
#include <dirent.h>
#endif

// --- Module-level static variables ---

// The single, global Lua state for the entire application.
static lua_State* g_lua_state = NULL;


// --- C Functions Exposed to Lua ---

/**
 * @brief Lua binding for the core logging function.
 *
 * Exposes `logger_log` to Lua scripts.
 * Lua usage: `gitph.log("INFO", "My message from Lua")`
 *
 * @param L The Lua state.
 * @return The number of return values pushed onto the stack (0).
 */
static int l_gitph_log(lua_State* L) {
    int n_args = lua_gettop(L);
    if (n_args != 2) {
        return luaL_error(L, "gitph.log expects 2 arguments: level (string) and message (string)");
    }

    const char* level_str = luaL_checkstring(L, 1);
    const char* message = luaL_checkstring(L, 2);

    GitphLogLevel level = LOG_LEVEL_INFO; // Default
    if (strcmp(level_str, "DEBUG") == 0) level = LOG_LEVEL_DEBUG;
    else if (strcmp(level_str, "WARN") == 0) level = LOG_LEVEL_WARN;
    else if (strcmp(level_str, "ERROR") == 0) level = LOG_LEVEL_ERROR;
    else if (strcmp(level_str, "FATAL") == 0) level = LOG_LEVEL_FATAL;

    logger_log(level, "LUA_PLUGIN", message);
    return 0; // No return values
}

/**
 * @brief Lua binding for the core command dispatcher.
 *
 * Exposes `cli_dispatch_command` to Lua scripts.
 * Lua usage: `gitph.run_command("status")`
 *
 * @param L The Lua state.
 * @return The number of return values pushed onto the stack (0).
 */
static int l_gitph_run_command(lua_State* L) {
    const char* command_str = luaL_checkstring(L, 1);

    // We need to simulate argc/argv. For now, we only support simple commands
    // without arguments from Lua. This can be expanded later.
    const char* argv[] = { "gitph", command_str, NULL };
    int argc = 2;

    cli_dispatch_command(argc, argv);
    return 0;
}

// Array defining the functions to be registered in the `gitph` Lua table.
static const struct luaL_Reg gitph_lib[] = {
    {"log", l_gitph_log},
    {"run_command", l_gitph_run_command},
    {NULL, NULL} // Sentinel
};


// --- Public API Implementation ---

/**
 * @see lua_bridge.h
 */
GitphStatus lua_bridge_init(void) {
    if (g_lua_state) {
        logger_log(LOG_LEVEL_WARN, "LUA_BRIDGE", "Lua bridge already initialized.");
        return GITPH_SUCCESS;
    }

    // 1. Create Lua state and load standard libraries
    g_lua_state = luaL_newstate();
    if (!g_lua_state) {
        logger_log(LOG_LEVEL_FATAL, "LUA_BRIDGE", "Failed to create Lua state.");
        return GITPH_ERROR_INIT_FAILED;
    }
    luaL_openlibs(g_lua_state);

    // 2. Create the `gitph` library table and register our C functions
    luaL_newlib(g_lua_state, gitph_lib);
    lua_setglobal(g_lua_state, "gitph");

    // 3. Scan and load all scripts from the "plugins" directory
    const char* plugin_dir = "plugins";
    // NOTE: The fixed-size `log_buffer` has been removed to prevent buffer overflows.

#ifdef PLATFORM_WINDOWS
    char search_path[MAX_PATH];
    snprintf(search_path, sizeof(search_path), "%s\\*.lua", plugin_dir);
    WIN32_FIND_DATA fd;
    HANDLE hFind = FindFirstFile(search_path, &fd);
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            char full_path[MAX_PATH];
            snprintf(full_path, sizeof(full_path), "%s\\%s", plugin_dir, fd.cFileName);
            if (luaL_dofile(g_lua_state, full_path) != LUA_OK) {
                // SAFETIFY: Replaced snprintf + logger_log with a single safe call.
                logger_log_fmt(LOG_LEVEL_ERROR, "LUA_BRIDGE", "Failed to load plugin '%s': %s", full_path, lua_tostring(g_lua_state, -1));
                lua_pop(g_lua_state, 1); // Pop error message from stack
            }
        } while (FindNextFile(hFind, &fd) != 0);
        FindClose(hFind);
    }
#else // POSIX
    DIR* d = opendir(plugin_dir);
    if (d) {
        struct dirent* dir;
        while ((dir = readdir(d)) != NULL) {
            if (strstr(dir->d_name, ".lua")) {
                char full_path[1024];
                snprintf(full_path, sizeof(full_path), "%s/%s", plugin_dir, dir->d_name);
                if (luaL_dofile(g_lua_state, full_path) != LUA_OK) {
                    // SAFETIFY: Replaced snprintf + logger_log with a single safe call.
                    logger_log_fmt(LOG_LEVEL_ERROR, "LUA_BRIDGE", "Failed to load plugin '%s': %s", full_path, lua_tostring(g_lua_state, -1));
                    lua_pop(g_lua_state, 1); // Pop error message
                }
            }
        }
        closedir(d);
    }
#endif

    logger_log(LOG_LEVEL_INFO, "LUA_BRIDGE", "Lua scripting engine initialized.");
    return GITPH_SUCCESS;
}

/**
 * @see lua_bridge.h
 */
GitphStatus lua_bridge_run_hook(const char* function_name, int argc, const char** argv) {
    if (!g_lua_state) return GITPH_ERROR_GENERAL;

    // Get the global function from Lua
    lua_getglobal(g_lua_state, function_name);

    if (!lua_isfunction(g_lua_state, -1)) {
        lua_pop(g_lua_state, 1); // Pop non-function value
        return GITPH_ERROR_NOT_FOUND; // Hook doesn't exist, which is not an error
    }

    // Push arguments onto the stack
    for (int i = 0; i < argc; ++i) {
        lua_pushstring(g_lua_state, argv[i]);
    }

    // Call the function with `argc` arguments and expect 0 return values
    if (lua_pcall(g_lua_state, argc, 0, 0) != LUA_OK) {
        // SAFETIFY: Replaced snprintf + logger_log with a single safe call.
        logger_log_fmt(LOG_LEVEL_ERROR, "LUA_BRIDGE", "Error running hook '%s': %s", function_name, lua_tostring(g_lua_state, -1));
        lua_pop(g_lua_state, 1); // Pop error message
        return GITPH_ERROR_EXEC_FAILED;
    }

    return GITPH_SUCCESS;
}

/**
 * @see lua_bridge.h
 */
void lua_bridge_cleanup(void) {
    if (g_lua_state) {
        lua_close(g_lua_state);
        g_lua_state = NULL;
        logger_log(LOG_LEVEL_INFO, "LUA_BRIDGE", "Lua bridge cleaned up.");
    }
}