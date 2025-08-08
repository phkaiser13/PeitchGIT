/* Copyright (C) 2025 Pedro Henrique
 * main.c - Main entry point for the gitph application.
 *
 * This file contains the main function, which serves as the orchestrator for
 * the entire application lifecycle. Its primary responsibilities include:
 *
 * 1. Initializing core subsystems in the correct order: platform-specific
 *    code, configuration management, logging, and the dynamic module loader.
 * 2. Parsing the initial command-line arguments to determine the execution mode.
 *    - If no commands are provided, it launches the interactive Text-based
 *      User Interface (TUI).
 *    - If a command is provided (e.g., "gitph SND"), it dispatches the command
 *      to the CLI handler.
 * 3. Ensuring a graceful shutdown by cleaning up all initialized subsystems
 *    before the program terminates.
 *
 * This file is intentionally kept high-level to provide a clear overview of
 * the application's control flow, delegating specialized logic to their
 * respective modules.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include <stdio.h>
#include <stdlib.h>

// Core module headers - these will be created in subsequent steps
#include "platform/platform.h"      // For platform-specific initializations
#include "config/config_manager.h"  // For loading and accessing configuration
#include "ui/tui.h"                 // For the interactive user interface
#include "cli/cli_parser.h"         // For parsing and dispatching commands
#include "module_loader/loader.h"   // For loading and managing modules
#include "scripting/lua_bridge.h"   // For initializing the scripting engine

// Shared library headers
#include "libs/liblogger/Logger.h"  // For the C++ based logging system (via C wrapper)

// Version is typically set by the build system (e.g., CMake/Makefile)
#ifndef GITPH_VERSION
#define GITPH_VERSION "0.1.0-dev"
#endif

/**
 * @brief Prints the application's version information and exits.
 */
static void print_version(void) {
    printf("gitph version %s\n", GITPH_VERSION);
    printf("Copyright (C) 2025 Pedro Henrique\n");
    printf("License: GPL-3.0-or-later\n");
}

/**
 * @brief Prints the main help message with basic usage and exits.
 *        Detailed help for commands is handled by the modules themselves.
 */
static void print_help(const char* app_name) {
    printf("Usage: %s [command] [options]\n\n", app_name);
    printf("A modern, polyglot Git helper to streamline your workflow.\n\n");
    printf("Run '%s' without arguments to enter the interactive menu.\n\n", app_name);
    printf("Core Commands:\n");
    printf("  --help      Show this help message and exit.\n");
    printf("  --version   Show version information and exit.\n\n");
    printf("For a full list of commands, enter the interactive menu.\n");
}

/**
 * @brief The main entry point of the gitph application.
 *
 * @param argc The number of command-line arguments.
 * @param argv An array of command-line argument strings.
 * @return EXIT_SUCCESS on successful execution, EXIT_FAILURE on error.
 */
int main(int argc, char* argv[]) {
    // 1. Initialize Core Subsystems
    // Platform-specific setup (e.g., console encoding on Windows)
    platform_global_init();

    // Initialize the logging system (the logger itself is C++, but we'll have a C API)
    // Note: We will create a C wrapper for the C++ logger.
    logger_init("gitph_log.txt");
    logger_log(LOG_LEVEL_INFO, "MAIN", "Gitph application starting.");

    // Load configuration from file (e.g., .gitph.conf)
    if (config_load("gitph.conf") != GITPH_SUCCESS) {
        logger_log(LOG_LEVEL_ERROR, "MAIN", "Failed to load configuration. Using defaults.");
        // This is not a fatal error; the app can run with default settings.
    }

    // Initialize the Lua scripting engine
    if (lua_bridge_init() != GITPH_SUCCESS) {
        logger_log(LOG_LEVEL_FATAL, "MAIN", "Failed to initialize Lua scripting engine. Exiting.");
        goto cleanup;
    }

    // Discover and load all external modules from the 'modules' directory
    if (modules_load("./modules") != GITPH_SUCCESS) {
        logger_log(LOG_LEVEL_FATAL, "MAIN", "Failed to load core modules. Exiting.");
        goto cleanup;
    }
    logger_log(LOG_LEVEL_INFO, "MAIN", "All modules loaded successfully.");


    // 2. Determine Execution Mode
    if (argc < 2) {
        // No arguments provided: Enter interactive TUI mode
        logger_log(LOG_LEVEL_INFO, "MAIN", "No arguments detected. Starting interactive TUI mode.");
        tui_show_main_menu();
    } else {
        // Arguments provided: Dispatch to CLI handler
        // Note: Basic flags like --help are handled here for fast response.
        if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
            print_help(argv[0]);
        } else if (strcmp(argv[1], "--version") == 0) {
            print_version();
        } else {
            logger_log(LOG_LEVEL_INFO, "MAIN", "Command detected. Dispatching to CLI handler.");
            cli_dispatch_command(argc, (const char**)argv);
        }
    }

    logger_log(LOG_LEVEL_INFO, "MAIN", "Gitph application shutting down.");

    // 3. Graceful Shutdown
cleanup:
    modules_cleanup();
    lua_bridge_cleanup();
    config_cleanup();
    logger_cleanup();
    platform_global_cleanup();

    return EXIT_SUCCESS;
}