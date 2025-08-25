/*
 * Copyright (C) 2025 Pedro Henrique / phkaiser13
 *
 * File: src/core/dispatcher/dispatcher.h
 *
 * [
 * This header file defines the public API for the Command Dispatcher module. The dispatcher
 * is a critical architectural component responsible for bridging the gap between the command-line
 * interface (CLI) and the core application logic. Its primary role is to take the parsed command-line
 * arguments and orchestrate the execution of the corresponding functionality within the system.
 *
 * Architecture:
 * The dispatcher acts as a central hub or a router. It decouples the CLI parsing logic from the
 * business logic of the individual modules. This separation of concerns is paramount for a clean,
 * maintainable, and scalable architecture. By using a single dispatch function, `dispatch_command`,
 * we create a clear and predictable entry point for all application actions initiated by the user.
 *
 * The `dispatch_command` function will receive a structure representing the parsed arguments from
 * the `cli_parser` module. It will then use a series of control structures (e.g., switch-case or if-else if)
 * to determine which core function or FFI call to a Rust module needs to be executed.
 *
 * Role in the System:
 * This module is the heart of the application's control flow. It ensures that user commands are correctly
 * interpreted and routed to the appropriate handlers, making the system's behavior explicit and easy to trace.
 * It also provides a single point for adding new commands and functionality without modifying the main
 * application entry point (`main.c`), thus promoting modularity and extensibility.
 * ]
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef DISPATCHER_H
#define DISPATCHER_H

#include "../cli/cli_parser.h" // Includes the definition for CliArguments structure

// --- Function Prototypes ---

/**
 * @brief Dispatches commands based on parsed command-line arguments.
 *
 * This is the main entry point into the application's business logic from the CLI.
 * It takes the fully parsed arguments and makes decisions on which modules and functions
 * to call to execute the requested action. It handles command routing, option processing,
 * and calling into the appropriate C functions or Rust FFI boundaries.
 *
 * @param args A pointer to the CliArguments struct, which contains the parsed command-line
 * arguments and options. This struct is populated by the `parse_cli` function
 * in the cli_parser module. The dispatcher will read from this struct but will
 * not modify it.
 *
 * @return An integer representing the exit code of the executed command.
 * - 0 for success.
 * - A non-zero value for failure, with different values indicating different
 * types of errors (e.g., command not found, module error, etc.). This allows
 * the main function to return a meaningful status to the operating system.
 *
 * Pre-conditions:
 * - The `args` pointer must not be NULL.
 * - The `CliArguments` struct pointed to by `args` must be properly initialized and
 * populated by `parse_cli`.
 *
 * Post-conditions:
 * - The corresponding command will be executed.
 * - An appropriate exit code will be returned to the caller.
 *
 * Design Rationale:
 * By centralizing the command dispatch logic here, we keep the main() function clean and focused
 * on high-level application setup and teardown. It also makes adding new commands a more
 * structured process: one only needs to add a case to this dispatcher and the corresponding
 * implementation, rather than cluttering `main.c`.
 */
int dispatch_command(const CliArguments *args);

#endif // DISPATCHER_H