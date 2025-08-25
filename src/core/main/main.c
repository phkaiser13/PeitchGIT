/*
 * Copyright (C) 2025 Pedro Henrique / phkaiser13
 *
 * File: src/core/main/main.c
 *
 * [
 * This file serves as the primary entry point for the entire application. Its main
 * responsibilities are orchestrated at a high level, ensuring a clean and structured
 * application lifecycle.
 *
 * Architecture:
 * The `main` function follows a clear, three-phase process:
 * 1.  Initialization: It begins by setting up essential components, such as parsing
 * command-line arguments using the `cli_parser` module and loading the application
 * configuration via the `config_manager`.
 *
 * 2.  Dispatch: Once the setup is complete, `main` delegates the core logic execution
 * to the `dispatcher` module. It calls `dispatch_command`, passing the parsed
 * arguments. This is a critical design choice that keeps `main` decoupled from the
 * specifics of any single command, making the system highly modular.
 *
 * 3.  Shutdown: After the dispatcher returns an exit code, `main` performs necessary
 * cleanup operations, such as freeing allocated memory for CLI arguments, and then
 * exits, returning the status code from the executed command to the operating system.
 *
 * Role in the System:
 * As the entry point, this file is the foundation upon which the application runs. By
 * maintaining a high-level, orchestral role, it enhances code readability, maintainability,
 * and scalability. Changes to command logic are isolated within their respective modules
 * and the dispatcher, leaving this core file stable and focused on its primary purpose.
 * ]
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stdlib.h>

#include "main.h"
#include "../cli/cli_parser.h"
#include "../config/config_manager.h"
#include "../dispatcher/dispatcher.h" // Include the new dispatcher header

// --- Main Application Entry Point ---

/**
 * @brief The main entry point of the application.
 *
 * Orchestrates the application's lifecycle: initialization, command execution, and shutdown.
 *
 * @param argc The number of command-line arguments.
 * @param argv An array of command-line argument strings.
 *
 * @return An integer exit code. 0 for success, non-zero for failure.
 */
int main(int argc, char *argv[])
{
    // --- Phase 1: Initialization ---

    // Allocate memory for the structure that will hold the parsed CLI arguments.
    // Using calloc to ensure the memory is zero-initialized, which is a good practice.
    CliArguments *args = calloc(1, sizeof(CliArguments));
    if (args == NULL)
    {
        // A critical failure if we cannot even allocate this small amount of memory.
        // There's no point in continuing.
        fprintf(stderr, "Fatal: Failed to allocate memory for CLI arguments.\n");
        return 1; // Exit with a non-zero status code.
    }

    // Parse the command-line arguments provided by the user. The `parse_cli` function
    // will populate the `args` structure with the recognized command and any associated
    // options or values.
    parse_cli(argc, argv, args);

    // Placeholder for configuration loading. In a real application, this would load
    // settings from a file (e.g., config.yaml) to configure modules like logging,
    // API clients, etc.
    // load_configuration("path/to/config.yaml");

    // Placeholder for initializing the logging system.
    // logger_init(LOG_LEVEL_INFO);

    printf("Application initialized.\n");

    // --- Phase 2: Dispatch ---

    // Delegate the command handling to the dispatcher. The `dispatch_command` function
    // contains the primary logic for routing the command to the correct module
    // (e.g., a C function or a Rust module via FFI).
    // This keeps the main function clean and focused.
    int exit_code = dispatch_command(args);

    // --- Phase 3: Shutdown ---

    printf("Application shutting down.\n");

    // Free the memory allocated for the CLI arguments. It is crucial to clean up
    // all dynamically allocated resources to prevent memory leaks.
    free_cli_args(args);

    // Placeholder for any other shutdown procedures, like closing log files,
    // disconnecting from databases, etc.
    // logger_shutdown();

    // Return the exit code provided by the dispatcher. This allows the application
    // to signal success (0) or a specific error (non-zero) to the calling shell or script.
    return exit_code;
}