/* Copyright (C) 2025 Pedro Henrique / phkaiser13
 * cli_parser.c - Implementation of the CLI Parser and Dispatcher.
 *
 * This file implements the logic for routing commands to their respective
 * handlers. It acts as a powerful, two-stage dispatcher that decouples command
 * invocation from execution. This enhanced version now supports commands
 * registered dynamically via the Lua scripting engine.
 *
 * The `cli_dispatch_command` function is the single point of entry. Its logic is:
 * 1. First, query the Lua bridge to see if the command is a user-defined script.
 *    If found, execution is delegated to the Lua engine.
 * 2. If not found in the Lua bridge, it falls back to querying the native
 *    module loader for a compiled C handler.
 * 3. If no handler is found in either system, an "unknown command" error is reported.
 *
 * This design makes the CLI extremely extensible, allowing new functionality to be
 * added via either compiled modules or simple Lua scripts without changing the core.
 *
 * SPDX-License-Identifier: Apache-2.0 */

#include "cli_parser.h"
#include "module_loader/loader.h"
#include "ui/tui.h" // For printing user-facing errors
#include "libs/liblogger/Logger.hpp"
#include "scripting/lua_bridge.h" // To get Lua commands
#include <stdio.h>
#include <string.h>


/**
 * @see cli_parser.h
 */
phgitStatus cli_dispatch_command(int argc, const char** argv) {
    // Basic validation: we need at least the application name and a command.
    if (argc < 2 || argv[1] == NULL || strlen(argv[1]) == 0) {
        tui_print_error("No command provided. Use --help for usage information.");
        logger_log(LOG_LEVEL_WARN, "CLI", "Dispatch called with no command.");
        return phgit_ERROR_INVALID_ARGS;
    }

    const char* command = argv[1];
    logger_log_fmt(LOG_LEVEL_INFO, "CLI", "Attempting to dispatch command: '%s'", command);

    // --- STAGE 1: Check the Lua Bridge for a registered command ---
    if (lua_bridge_has_command(command)) {
        logger_log_fmt(LOG_LEVEL_INFO, "CLI", "Command '%s' is a registered Lua command. Dispatching to bridge.", command);

        // Execute the Lua command. We pass arguments similarly to native modules,
        // making the command name argv[0] from the script's perspective.
        phgitStatus status = lua_bridge_execute_command(command, argc - 1, &argv[1]);

        if (status != phgit_SUCCESS) {
            logger_log_fmt(LOG_LEVEL_ERROR, "CLI", "Execution of Lua command '%s' failed with status code %d.", command, status);
            // The Lua bridge or script itself should have logged a specific error.
            // We provide generic feedback to the user.
            tui_print_error("The scripted command failed to execute successfully.");
        } else {
            logger_log_fmt(LOG_LEVEL_INFO, "CLI", "Lua command '%s' executed successfully.", command);
        }
        return status;
    }

    // --- STAGE 2: Fallback to native C modules ---
    logger_log_fmt(LOG_LEVEL_DEBUG, "CLI", "Command '%s' not found in Lua bridge. Checking native modules.", command);
    const LoadedModule* handler_module = modules_find_handler(command);

    if (handler_module) {
        // A native handler was found, so we can execute it.
        logger_log_fmt(LOG_LEVEL_INFO, "CLI", "Found native handler for '%s' in module '%s'. Executing...", command, handler_module->info.name);

        // We pass the arguments to the module, shifting them so that the
        // command itself is the first argument (argv[0]) from the module's
        // perspective.
        phgitStatus status = handler_module->exec_func(argc - 1, &argv[1]);

        if (status != phgit_SUCCESS) {
            // The module indicated that an error occurred during execution.
            logger_log_fmt(LOG_LEVEL_ERROR, "CLI", "Execution of native command '%s' failed with status code %d.", command, status);

            // Provide generic feedback to the user. The module itself should
            // have already printed a more specific error message.
            tui_print_error("The command failed to execute successfully.");
        } else {
            logger_log_fmt(LOG_LEVEL_INFO, "CLI", "Native command '%s' executed successfully.", command);
        }
        return status;

    }

    // --- STAGE 3: Command not found in either system ---
    char error_msg[128];
    snprintf(error_msg, sizeof(error_msg), "Unknown command: '%s'", command);
    tui_print_error(error_msg);

    logger_log_fmt(LOG_LEVEL_WARN, "CLI", "No handler found for command: '%s'", command);
    return phgit_ERROR_NOT_FOUND;
}