/*
 * Copyright (C) 2025 Pedro Henrique / phkaiser13
 *
 * File: src/core/dispatcher/dispatcher.c
 *
 * [
 * This implementation file for the Command Dispatcher contains the core logic for routing
 * user commands to their respective handlers. It embodies the principle of separation of
 * concerns by acting as an intermediary between the command-line interface (CLI) parsing
 * and the actual business logic implemented in various C and Rust modules.
 *
 * Architecture:
 * The `dispatch_command` function serves as the central switching yard. It uses a `switch`
 * statement on the `command` enum from the `CliArguments` struct. Each case in the switch
 * corresponds to a specific command the user can issue (e.g., 'apply', 'diff', 'sync').
 *
 * Initially, the command handlers within each case are placeholders. This modular structure
 * allows for incremental development. As functionality is built out in the respective modules
 * (like `git_ops`, `sync_engine`, `k8s_preview`), the placeholder comments will be replaced
 * with concrete function calls. For Rust modules, these calls will go through the Foreign
 * Function Interface (FFI) defined in `rust_ffi.h`.
 *
 * Error handling is centralized. A default case in the switch handles unknown commands,
 * ensuring graceful failure. Each command handler is expected to return a status code,
 * which is then propagated up to the main function to set the application's exit code.
 *
 * Role in the System:
 * This dispatcher is the engine of the application. It translates user intent, captured by the CLI,
 * into concrete actions. By keeping this routing logic separate, we make the system easier
 * to maintain and extend. Adding a new command becomes a matter of defining it in the
 * CLI parser, adding a case to this dispatcher, and implementing the corresponding
 * function, without disturbing the existing command flows.
 * ]
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <string.h>

#include "dispatcher.h"
#include "../ffi/rust_ffi.h" // For calling into Rust modules

// --- Function Implementations ---

/**
 * @brief Dispatches commands based on parsed command-line arguments.
 *
 * This function contains the primary control flow logic for the application. It
 * inspects the `command` field of the `CliArguments` struct and executes the
 * corresponding code block.
 *
 * @param args A constant pointer to the CliArguments struct populated by the CLI parser.
 *
 * @return 0 on success, non-zero on failure.
 */
int dispatch_command(const CliArguments *args)
{
    // Pre-condition check: Ensure the arguments structure is not null.
    // This is a defensive programming practice to prevent null pointer dereferences.
    if (args == NULL)
    {
        fprintf(stderr, "Dispatcher Error: Received null arguments.\n");
        return 1; // Return a non-zero error code.
    }

    // Use a switch statement to route to the correct command handler based on the
    // parsed command enum. This is more efficient and readable than a long chain
    // of if-else statements.
    switch (args->command)
    {
    case CMD_APPLY:
        printf("Executing APPLY command...\n");
        // Placeholder: Here we would call a function to handle the 'apply' logic.
        // This might involve calling into the `sync_engine` Rust module via FFI.
        // e.g., return rust_ffi_apply(args->path, args->dry_run);
        break;

    case CMD_DIFF:
        printf("Executing DIFF command...\n");
        // Placeholder: Call a function for the 'diff' logic.
        // This would likely use `git_ops` and `kube_diff` Rust modules.
        // e.g., return rust_ffi_diff(args->path1, args->path2);
        break;

    case CMD_SYNC:
        printf("Executing SYNC command...\n");
        // Placeholder: Call a function for the 'sync' logic.
        // This would be a core operation of the `sync_engine`.
        // e.g., return rust_ffi_sync(args->repo, args->flags);
        break;

    case CMD_PREVIEW:
        printf("Executing PREVIEW command...\n");
        // Placeholder: Call a function for the 'preview' environment logic.
        // This would interact with the `k8s_preview` Rust module.
        // e.g., return rust_ffi_create_preview(args->branch, args->options);
        break;

    case CMD_RELEASE:
        printf("Executing RELEASE command...\n");
        // Placeholder: Call a function to manage releases.
        // This would interact with the `release_orchestrator` Rust module.
        // e.g., return rust_ffi_create_release(args->version, args->strategy);
        break;

    case CMD_POLICY_CHECK:
        printf("Executing POLICY-CHECK command...\n");
        // Placeholder: This would invoke the OPA policy checking logic, potentially
        // managed by a Rust module that wraps the OPA engine.
        // e.g., return rust_ffi_check_policies(args->path, args->policy_dir);
        break;

    case CMD_UNKNOWN:
        // This case handles any command that is not recognized by the CLI parser.
        fprintf(stderr, "Error: Unknown command.\n");
        // We can also print usage information here to guide the user.
        // print_usage();
        return 1; // Return error code for unknown command.

    default:
        // This default case is a safeguard. In theory, the CLI parser should always
        // set the command, even if it's to CMD_UNKNOWN. This prevents unhandled cases.
        fprintf(stderr, "Dispatcher Error: Unhandled command enum value: %d\n", args->command);
        return 1; // Return a generic error code.
    }

    // If the command was recognized and executed without returning an error,
    // we return 0 to indicate success.
    return 0;
}