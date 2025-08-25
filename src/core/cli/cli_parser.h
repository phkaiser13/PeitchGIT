/*
 * Copyright (C) 2025 Pedro Henrique / phkaiser13
 *
 * File: src/core/cli/cli_parser.h
 *
 * [
 * This header file defines the core components of the Command Line Interface (CLI)
 * parser for PeitchGIT. Its primary purpose is to establish the public API for parsing
 * user commands, dispatching them to the appropriate handlers, and managing the data
 * structures required for command execution.
 *
 * Architecture:
 * The architecture is designed around a command-driven dispatcher. It employs a set of
 * enumerations and structures to represent the hierarchy of commands and their specific
 * arguments. This approach provides a clear, scalable, and maintainable way to extend
 * the CLI with new functionalities.
 *
 * Key components defined in this file:
 * 1.  `CommandType` Enum: An enumeration of all primary commands supported by the CLI.
 * This is the first level of command dispatching.
 * 2.  `SubCommandType` Enum: An enumeration for subcommands, specifically for the new
 * Kubernetes-native operations, allowing for a structured command hierarchy (e.g., `ph kube preview`).
 * 3.  Argument Structs: A series of structures (`PreviewArgs`, `KubeArgs`, `ReleaseArgs`)
 * that hold the parsed arguments for each specific command. This ensures type safety
 * and clarity when passing data to command handlers.
 * 4.  `CommandLineArgs` Struct: A central structure that aggregates all possible command
 * argument structs, along with the command types, providing a unified representation
 * of a parsed command line.
 * 5.  Function Prototypes:
 * - `cli_parse_args`: The main entry point for parsing the command line arguments (`argc`, `argv`).
 * - `cli_dispatch_command`: The core dispatcher function that takes the parsed arguments
 * and invokes the corresponding logic, including the FFI calls to Rust modules.
 * - `free_command_line_args`: A utility function for safely freeing any dynamically
 * allocated memory within the `CommandLineArgs` structure.
 *
 * This file serves as the contract for how the `main` function interacts with the CLI
 * subsystem and how new commands are to be defined and integrated into the system.
 * ]
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CLI_PARSER_H
#define CLI_PARSER_H

#include <stddef.h> // For size_t
#include <stdbool.h> // For bool type

// The maximum length for string arguments, providing a safe buffer size.
#define MAX_ARG_LENGTH 256

/**
 * @enum CommandType
 * @brief Enumerates the primary commands available in the PeitchGIT CLI.
 *
 * This enumeration represents the first-level commands that the user can invoke.
 * The dispatcher will use these values to determine the primary action to be taken.
 * CMD_UNKNOWN is used for unrecognized commands, and CMD_HELP provides usage information.
 */
typedef enum
{
	CMD_UNKNOWN = 0,
	CMD_HELP,
	CMD_PREVIEW,
	CMD_KUBE,
	CMD_RELEASE,
	// Add other primary commands here as the CLI evolves.
} CommandType;

/**
 * @enum SubCommandType
 * @brief Enumerates the subcommands, particularly for Kubernetes-native operations.
 *
 * This allows for a second level of dispatching, creating a more organized and
 * intuitive command structure (e.g., `ph kube preview`). SUB_CMD_NONE is used when a
 * primary command does not have a subcommand.
 */
typedef enum
{
	SUB_CMD_NONE = 0,
	SUB_CMD_PREVIEW_CREATE,
	SUB_CMD_PREVIEW_DESTROY,
	SUB_CMD_PREVIEW_LIST,
	// Future subcommands for `kube`, `release`, etc., would be added here.
} SubCommandType;

/**
 * @struct PreviewArgs
 * @brief Holds the parsed arguments for the 'preview' command.
 *
 * This structure encapsulates all the potential options and values that can be
 * passed to the `preview` command, ensuring that the handler function receives
 * a well-defined set of parameters.
 */
typedef struct
{
	int pull_request_id;		// --pr <id>
	char ttl[MAX_ARG_LENGTH];   // --ttl <duration>, e.g., "48h"
	char tag[MAX_ARG_LENGTH];   // --tag <tag>
	bool ephemeral;			 // --ephemeral flag
	SubCommandType subcommand;  // The specific action (create, destroy, list)
} PreviewArgs;

/**
 * @struct KubeArgs
 * @brief Holds the parsed arguments for the 'kube' command.
 *
 * Currently a placeholder for future Kubernetes-related command arguments.
 * It will be expanded as `ph kube` functionality is implemented.
 */
typedef struct
{
	// Placeholder for future arguments, e.g., cluster name, namespace, etc.
	char context[MAX_ARG_LENGTH]; // --context <name>
} KubeArgs;

/**
 * @struct ReleaseArgs
 * @brief Holds the parsed arguments for the 'release' command.
 *
 * Encapsulates options for release orchestration, such as the target environment
 * or release version.
 */
typedef struct
{
	char environment[MAX_ARG_LENGTH]; // --env <name>
	char version[MAX_ARG_LENGTH];	 // --version <semver>
} ReleaseArgs;

/**
 * @struct CommandLineArgs
 * @brief A unified structure to hold all parsed command-line arguments.
 *
 * This is the central data structure that represents the fully parsed command from the user.
 * It contains the command and subcommand types, along with a union of the specific
 * argument structures. This design ensures that memory is used efficiently, as only
 * one command's worth of arguments is stored at a time.
 */
typedef struct
{
	CommandType command;
	SubCommandType subcommand;
	union
	{
		PreviewArgs preview_args;
		KubeArgs kube_args;
		ReleaseArgs release_args;
		// Add other command argument structs to this union.
	} args;
} CommandLineArgs;

/**
 * @brief Parses the command-line arguments provided at program start.
 *
 * @param argc The count of command-line arguments, from main().
 * @param argv The array of command-line argument strings, from main().
 * @param parsed_args A pointer to a CommandLineArgs structure that will be populated
 * with the parsed command and its arguments.
 * @return Returns true on successful parsing, false otherwise.
 *
 * This is the main entry point of the CLI parsing module. It orchestrates the
 * identification of the command, its subcommands, and the parsing of all
 * associated options and values. It performs initial validation and populates the
 * `CommandLineArgs` struct.
 */
bool cli_parse_args(int argc, char **argv, CommandLineArgs *parsed_args);

/**
 * @brief Dispatches the parsed command to its corresponding handler.
 *
 * @param args A pointer to the populated CommandLineArgs structure.
 * @return An integer status code, where 0 indicates success and non-zero indicates failure.
 *
 * This function is the heart of the CLI. It takes the parsed arguments, constructs
 * the necessary JSON configuration string, and invokes the appropriate FFI function
 * to call the corresponding Rust module. It is the bridge between the C core and
 * the Rust-based business logic.
 */
int cli_dispatch_command(const CommandLineArgs *args);

/**
 * @brief Frees any memory that was dynamically allocated during argument parsing.
 *
 * @param args A pointer to the CommandLineArgs structure to be cleaned up.
 *
 * While the current argument structures do not use dynamic allocation, this function
 * is provided as a forward-thinking measure for future scalability, ensuring a
 * clean and predictable memory management strategy.
 */
void free_command_line_args(CommandLineArgs *args);

#endif // CLI_PARSER_H