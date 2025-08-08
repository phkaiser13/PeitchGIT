/* Copyright (C) 2025 Pedro Henrique
 * cli_parser.c - Implementation of the CLI Parser and Dispatcher.
 *
 * This file implements the logic for routing commands to their respective
 * modules. It acts as a simple, yet powerful, intermediary that decouples the
 * command invocation from the command execution.
 *
 * The `cli_dispatch_command` function is the single point of entry. It does not
 * contain any business logic itself; its sole purpose is to query the module

 * loader for a "handler" for a given command string and, if one is found,
 * to invoke that handler's `exec_func`. This design keeps the core clean and
 * extensible, as adding new commands only requires adding a new module, with
 * no changes needed in this file.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "cli_parser.h"
#include "module_loader/loader.h"
#include "ui/tui.h" // For printing user-facing errors
#include "libs/liblogger/Logger.h"
#include <stdio.h>
#include <string.h>

/**
 * @see cli_parser.h
 */
void cli_dispatch_command(int argc, const char** argv) {
    // Basic validation: we need at least the application name and a command.
    if (argc < 2 || argv[1] == NULL || strlen(argv[1]) == 0) {
        tui_print_error("No command provided. Use --help for usage information.");
        logger_log(LOG_LEVEL_WARN, "CLI", "Dispatch called with no command.");
        return;
    }

    const char* command = argv[1];
    char log_buffer[256];
    snprintf(log_buffer, sizeof(log_buffer), "Attempting to dispatch command: '%s'", command);
    logger_log(LOG_LEVEL_INFO, "CLI", log_buffer);

    // Ask the module loader to find which module handles this command.
    const LoadedModule* handler_module = modules_find_handler(command);

    if (handler_module) {
        // A handler was found, so we can execute it.
        snprintf(log_buffer, sizeof(log_buffer), "Found handler for '%s' in module '%s'. Executing...", command, handler_module->info.name);
        logger_log(LOG_LEVEL_INFO, "CLI", log_buffer);

        // We pass the arguments to the module, but we shift them so that the
        // command itself is the first argument (argv[0]) from the module's
        // perspective. This simplifies parsing within the modules.
        // For example, `gitph SND -m "msg"` becomes `SND -m "msg"` for the module.
        GitphStatus status = handler_module->exec_func(argc - 1, &argv[1]);

        if (status != GITPH_SUCCESS) {
            // The module indicated that an error occurred during execution.
            snprintf(log_buffer, sizeof(log_buffer), "Execution of command '%s' failed with status code %d.", command, status);
            logger_log(LOG_LEVEL_ERROR, "CLI", log_buffer);

            // Provide generic feedback to the user. The module itself should
            // have already printed a more specific error message.
            tui_print_error("The command failed to execute successfully.");
        } else {
            snprintf(log_buffer, sizeof(log_buffer), "Command '%s' executed successfully.", command);
            logger_log(LOG_LEVEL_INFO, "CLI", log_buffer);
        }

    } else {
        // No module in our registry claimed this command.
        char error_msg[128];
        snprintf(error_msg, sizeof(error_msg), "Unknown command: '%s'", command);
        tui_print_error(error_msg);

        snprintf(log_buffer, sizeof(log_buffer), "No handler found for command: '%s'", command);
        logger_log(LOG_LEVEL_WARN, "CLI", log_buffer);
    }
}