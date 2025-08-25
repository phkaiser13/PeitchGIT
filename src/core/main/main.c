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
// #include "../cli/cli_parser.h" // Temporarily disabled
#include "../config/config_manager.h"
// #include "../dispatcher/dispatcher.h" // Temporarily disabled

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

    /*
    // Temporarily disable CLI parsing and dispatching
    CliArguments *args = calloc(1, sizeof(CliArguments));
    if (args == NULL)
    {
        fprintf(stderr, "Fatal: Failed to allocate memory for CLI arguments.\n");
        return 1;
    }
    parse_cli(argc, argv, args);
    */

    printf("Application initialized (CLI parsing is temporarily disabled).\n");

    // --- Phase 2: Dispatch ---

    /*
    // Temporarily disable command dispatching
    int exit_code = dispatch_command(args);
    */
    int exit_code = 0; // Assume success for now

    // --- Phase 3: Shutdown ---

    printf("Application shutting down.\n");

    /*
    // Free the memory allocated for the CLI arguments.
    free_cli_args(args);
    */

    return exit_code;
}