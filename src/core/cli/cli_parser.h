/* Copyright (C) 2025 Pedro Henrique / phkaiser13
* cli_parser.h - Interface for the Command Line Interface Parser and Dispatcher.
 *
 * This header defines the public API for the CLI handling module. This module
 * serves as the central dispatcher for all non-interactive commands. It is
 * responsible for taking the raw command-line arguments (`argc`, `argv`),
 * identifying the primary command, and delegating execution to the appropriate
 * loaded module.
 *
 * This component is critical for code reuse, as it is called both directly
 * from `main` for commands like `gitph SND`, and from the `tui` module, which
 * simulates a command-line call to execute an action selected from the menu.
 *
 * SPDX-License-Identifier: Apache-2.0 */

#ifndef CLI_PARSER_H
#define CLI_PARSER_H

#ifdef __cplusplus
extern "C" {
#endif

    /**
     * @brief Parses and dispatches a command to the appropriate module.
     *
     * This function takes the command-line arguments, extracts the main command
     * (expected to be in `argv[1]`), and uses the module loader to find a module
     * that handles this command. If a handler is found, its `module_exec` function
     * is called with the relevant arguments.
     *
     * If no handler is found for the command, an appropriate error message is
     * logged and displayed to the user.
     *
     * @param argc The total number of arguments, including the application name.
     * @param argv An array of argument strings. `argv[0]` is the app name,
     *             `argv[1]` is the command.
     */
    void cli_dispatch_command(int argc, const char** argv);


#ifdef __cplusplus
} // extern "C"
#endif

#endif // CLI_PARSER_H