/*
 * Copyright (C) 2025 Pedro Henrique / phkaiser13
 *
 * File: src/core/main/main.c
 *
 * [
 * This file contains the main function, which serves as the orchestrator for
 * the entire application lifecycle. Its primary responsibilities include:
 *
 * 1.  Initializing core subsystems in the correct order: platform-specific
 * code, configuration management, logging, scripting, and the FFI layer for
 * Rust modules.
 * 2.  Delegating the parsing of command-line arguments to the `cli_parser` module.
 * This is a significant change from the previous architecture, centralizing all
 * CLI logic into its own dedicated component.
 * 3.  Launching either the interactive Text-based User Interface (TUI) or the
 * Command-Line Interface (CLI) handler based on the provided arguments.
 * 4.  Dispatching the parsed command via `cli_dispatch_command`, which acts as the
 * bridge to the Rust FFI layer.
 * 5.  Ensuring a graceful shutdown by cleaning up all initialized subsystems
 * before the program terminates, regardless of the execution path.
 *
 * This refactored version fully integrates the new CLI architecture, creating a clean
 * separation of concerns between the application's entry point and its command
 * processing logic.
 * ]
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Core module headers
#include "platform/platform.h"
#include "config/config_manager.h"
#include "ui/tui.h"
#include "cli/cli_parser.h"
#include "module_loader/loader.h"
#include "scripting/lua_bridge.h"
#include "ffi/rust_ffi.h" // Include the new FFI header

// Shared library headers
#include "libs/liblogger/Logger.hpp"

// Define application constants to avoid hardcoded strings
#define PHGIT_VERSION "0.2.0-dev" // Updated version
#define PHGIT_CONFIG_FILE "phgit.conf"
#define PHGIT_LOG_FILE "phgit_log.txt"
#define PHGIT_MODULES_DIR "./modules"

// Forward declarations for static functions
static void print_version(void);
static int initialize_subsystems(void);
static void cleanup_subsystems(void);
static int process_arguments(int argc, char *argv[]);

/**
 * @brief The main entry point of the phgit application.
 *
 * Orchestrates the application's lifecycle: initialization, execution,
 * and cleanup. It sets up all necessary subsystems, processes command-line
 * arguments to decide the execution mode (TUI or CLI), and ensures a
 * graceful shutdown.
 *
 * @param argc The number of command-line arguments.
 * @param argv An array of command-line argument strings.
 * @return EXIT_SUCCESS on successful execution, EXIT_FAILURE on critical error.
 */
int main(int argc, char *argv[])
{
	int exit_code = EXIT_SUCCESS;

	// 1. Initialize Core Subsystems
	if (initialize_subsystems() != 0)
	{
		// A critical subsystem failed to initialize. The logger might not be
		// available, so a printf fallback is used.
		fprintf(stderr,
			"A critical subsystem failed to initialize. Exiting.\n");
		exit_code = EXIT_FAILURE;
		goto cleanup; // Proceed to cleanup what might have been initialized.
	}

	// 2. Process Arguments and Determine Execution Mode
	exit_code = process_arguments(argc, argv);

	// 3. Graceful Shutdown
cleanup:
	logger_log(LOG_LEVEL_INFO, "MAIN", "phgit application shutting down.");
	cleanup_subsystems();

	return exit_code;
}

/**
 * @brief Initializes all core application subsystems in the correct order.
 *
 * This function handles the setup of platform-specific features, logging,
 * configuration, scripting engine, and module loading.
 *
 * @return 0 on success, -1 on a critical failure.
 */
static int initialize_subsystems(void)
{
	// Platform-specific setup (e.g., console encoding on Windows)
	platform_global_init();

	// Initialize the logging system first to make it available to other subsystems.
	logger_init(PHGIT_LOG_FILE);
	logger_log(LOG_LEVEL_INFO, "MAIN", "phgit application starting.");

	// Load configuration from file. This is not treated as a fatal error;
	// the application can run with default settings.
	if (config_load(PHGIT_CONFIG_FILE) != 0)
	{
		logger_log(LOG_LEVEL_WARN, "MAIN",
			   "Failed to load configuration. Using defaults.");
	}

	// Initialize the Lua scripting engine. This is critical for functionality.
	if (lua_bridge_init() != 0)
	{
		logger_log(
			LOG_LEVEL_FATAL, "MAIN",
			"Failed to initialize Lua scripting engine. Exiting.");
		return -1;
	}

	// Discover and load all external modules. This is critical.
	if (modules_load(PHGIT_MODULES_DIR) != 0)
	{
		logger_log(LOG_LEVEL_FATAL, "MAIN",
			   "Failed to load core modules. Exiting.");
		return -1;
	}
	logger_log(LOG_LEVEL_INFO, "MAIN", "All modules loaded successfully.");

	return 0; // Success
}

/**
 * @brief Processes command-line arguments and launches the appropriate mode.
 *
 * This function has been refactored to use the new CLI parsing and dispatching
 * system. It first attempts to parse the command-line arguments. If successful,
 * it dispatches the command. If no arguments are provided, it falls back to
 * starting the interactive TUI.
 *
 * @param argc The number of command-line arguments.
 * @param argv An array of command-line argument strings.
 * @return The exit code from the dispatched command, or EXIT_SUCCESS for TUI/help.
 */
static int process_arguments(int argc, char *argv[])
{
	if (argc < 2)
	{
		// No arguments provided: Enter interactive TUI mode
		logger_log(LOG_LEVEL_INFO, "MAIN",
			   "No arguments detected. Starting interactive TUI mode.");
		tui_show_main_menu();
		return EXIT_SUCCESS;
	}
	else
	{
		// Arguments provided: Dispatch to the new CLI handler.
		logger_log(LOG_LEVEL_INFO, "MAIN",
			   "Command detected. Dispatching to CLI handler.");

		CommandLineArgs parsed_args;
		if (cli_parse_args(argc, argv, &parsed_args))
		{
			// Parsing was successful, dispatch the command.
			// The return code from the command (ultimately from Rust) is returned.
			int dispatch_status = cli_dispatch_command(&parsed_args);
			free_command_line_args(&parsed_args);
			return dispatch_status == 0 ? EXIT_SUCCESS :
						      EXIT_FAILURE;
		}
		else
		{
			// Parsing failed. cli_parse_args already printed an error message.
			// Return a failure code.
			logger_log(LOG_LEVEL_ERROR, "MAIN",
				   "CLI argument parsing failed.");
			return EXIT_FAILURE;
		}
	}
}

/**
 * @brief Cleans up and terminates all initialized subsystems.
 *
 * This function is called during graceful shutdown to release resources,
 * ensuring no memory leaks or dangling file handlers. The order of cleanup
 * is the reverse of initialization.
 */
static void cleanup_subsystems(void)
{
	modules_cleanup();
	lua_bridge_cleanup();
	config_cleanup();
	logger_cleanup();
	platform_global_cleanup();
}

/**
 * @brief Prints the application's version information.
 *
 * Note: Direct version printing via a flag (`--version`) is now handled by
 * the `cli_parser` module to keep all command logic centralized. This function
 * remains for potential internal use.
 */
static void print_version(void)
{
	printf("phgit version %s\n", PHGIT_VERSION);
	printf("Copyright (C) 2025 Pedro Henrique / phkaiser13\n");
	printf("License: Apache-2.0\n");
}