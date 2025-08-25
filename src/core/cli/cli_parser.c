/*
 * Copyright (C) 2025 Pedro Henrique / phkaiser13
 *
 * File: src/core/cli/cli_parser.c
 *
 * [
 * This file provides the implementation for the Command Line Interface (CLI) parser
 * of PeitchGIT. It is responsible for the entire lifecycle of a command execution,
 * from parsing the raw command-line arguments to dispatching the command to the
 * appropriate backend logic, which includes the Foreign Function Interface (FFI)
 * calls to the Rust modules.
 *
 * Architecture:
 * The implementation employs a stateful, iterative parsing approach. The `cli_parse_args`
 * function iterates through the `argv` array, identifying commands, subcommands, and
 * their corresponding arguments. It uses `strcmp` for command matching, a highly
 * efficient method for this use case.
 *
 * The core of the architecture is the separation between parsing and dispatching:
 * 1.  `cli_parse_args`: This function is purely mechanical. It translates the string-based
 * `argv` into the structured `CommandLineArgs` format defined in `cli_parser.h`. It
 * includes robust error handling for unrecognized commands or malformed arguments.
 * Helper functions (`parse_preview_args`, etc.) are used to modularize the parsing
 * logic for each primary command, enhancing readability and maintainability.
 *
 * 2.  `cli_dispatch_command`: This function is the logical hub. It takes the structured
 * `CommandLineArgs` and acts as a bridge to the application's core logic. It contains
 * a `switch` statement that routes commands to their respective handlers. This is where
 * the critical task of constructing the JSON configuration string occurs, tailored to
 * the contract expected by each Rust FFI function. It then calls the FFI layer,
 * passing the JSON string, and handles the result.
 *
 * This design ensures that the C core remains decoupled from the Rust business logic,
 * communicating only through a well-defined FFI contract (JSON strings). This makes
 * the system highly modular and allows for independent development and testing of the
 * CLI and the core features.
 * ]
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "cli_parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Forward declarations for internal helper functions.
// This is a good practice to keep the file organized and allows the main public
// functions to be defined at the top.
static void initialize_args(CommandLineArgs *args);
static void print_help(void);
static bool parse_preview_args(int argc, char **argv, int *currentIndex,
			       CommandLineArgs *parsed_args);
static int dispatch_preview_command(const PreviewArgs *args);

/**
 * @brief Initializes the CommandLineArgs structure to a default state.
 *
 * @param args A pointer to the CommandLineArgs structure to initialize.
 *
 * Pre-conditions:
 * - `args` must be a valid, non-null pointer.
 *
 * Post-conditions:
 * - The structure is zero-initialized, ensuring no garbage values are present
 * before parsing begins. This is a crucial step for predictable behavior.
 *
 * Why this function is important:
 * Without explicit initialization, the fields in the struct could contain random
 * data, leading to undefined behavior during parsing and dispatch. This function
 * establishes a known, clean state.
 */
static void initialize_args(CommandLineArgs *args)
{
	if (!args)
	{
		return;
	}
	memset(args, 0, sizeof(CommandLineArgs));
	args->command = CMD_UNKNOWN;
	args->subcommand = SUB_CMD_NONE;
}

/**
 * @brief Prints the main help message for the PeitchGIT CLI.
 *
 * This function provides the user with a summary of available commands and their
 * basic usage. It is designed to be the primary source of user assistance.
 */
static void print_help(void)
{
	printf("PeitchGIT - The next-generation GitOps tool.\n\n");
	printf("Usage: ph <command> [<subcommand>] [options]\n\n");
	printf("Available Commands:\n");
	printf("  preview <create|destroy|list>  Manage preview environments.\n");
	printf("    --pr <id>                    Specify the Pull Request ID.\n");
	printf("    --ttl <duration>             Set a Time-To-Live (e.g., 48h, 30m).\n");
	printf("    --tag <tag>                  Specify an image tag.\n");
	printf("    --ephemeral                  Create an ephemeral preview environment.\n\n");
	printf("  kube <...>                     (Not yet implemented) Interact with Kubernetes clusters.\n");
	printf("  release <...>                  (Not yet implemented) Orchestrate software releases.\n");
	printf("  help                           Show this help message.\n\n");
}

/**
 * @brief Parses the arguments for the 'preview' command and its subcommands.
 *
 * @param argc The total number of arguments.
 * @param argv The array of argument strings.
 * @param currentIndex A pointer to the current index in the argv array.
 * @param parsed_args A pointer to the structure to be populated.
 * @return Returns true on successful parsing, false on error.
 *
 * Pre-conditions:
 * - The command has already been identified as `CMD_PREVIEW`.
 * - `currentIndex` points to the subcommand (e.g., "create").
 *
 * Post-conditions:
 * - `parsed_args` is populated with the specific options for the preview command.
 * - `currentIndex` is advanced past all consumed arguments.
 *
 * This function encapsulates the logic for a single command, making the main
 * `cli_parse_args` function cleaner and more of a dispatcher itself.
 */
static bool parse_preview_args(int argc, char **argv, int *currentIndex,
			       CommandLineArgs *parsed_args)
{
	// A subcommand is mandatory for `preview`.
	if (*currentIndex >= argc)
	{
		fprintf(stderr, "Error: 'preview' command requires a subcommand (create, destroy, or list).\n");
		return false;
	}

	char *subcommand = argv[*currentIndex];
	if (strcmp(subcommand, "create") == 0)
	{
		parsed_args->subcommand = SUB_CMD_PREVIEW_CREATE;
	}
	else if (strcmp(subcommand, "destroy") == 0)
	{
		parsed_args->subcommand = SUB_CMD_PREVIEW_DESTROY;
	}
	else if (strcmp(subcommand, "list") == 0)
	{
		parsed_args->subcommand = SUB_CMD_PREVIEW_LIST;
	}
	else
	{
		fprintf(stderr, "Error: Unknown subcommand '%s' for 'preview'.\n",
			subcommand);
		return false;
	}

	(*currentIndex)++; // Move past the subcommand

	// Now, parse the options for the preview command.
	while (*currentIndex < argc)
	{
		char *arg = argv[*currentIndex];
		if (strcmp(arg, "--pr") == 0)
		{
			if (*currentIndex + 1 >= argc)
			{
				fprintf(stderr, "Error: --pr requires a value.\n");
				return false;
			}
			parsed_args->args.preview_args.pull_request_id =
				atoi(argv[*currentIndex + 1]);
			(*currentIndex) += 2;
		}
		else if (strcmp(arg, "--ttl") == 0)
		{
			if (*currentIndex + 1 >= argc)
			{
				fprintf(stderr, "Error: --ttl requires a value.\n");
				return false;
			}
			strncpy(parsed_args->args.preview_args.ttl,
				argv[*currentIndex + 1], MAX_ARG_LENGTH - 1);
			parsed_args->args.preview_args
				.ttl[MAX_ARG_LENGTH - 1] = '\0'; // Ensure null-termination
			(*currentIndex) += 2;
		}
		else if (strcmp(arg, "--tag") == 0)
		{
			if (*currentIndex + 1 >= argc)
			{
				fprintf(stderr, "Error: --tag requires a value.\n");
				return false;
			}
			strncpy(parsed_args->args.preview_args.tag,
				argv[*currentIndex + 1], MAX_ARG_LENGTH - 1);
			parsed_args->args.preview_args
				.tag[MAX_ARG_LENGTH - 1] = '\0'; // Ensure null-termination
			(*currentIndex) += 2;
		}
		else if (strcmp(arg, "--ephemeral") == 0)
		{
			parsed_args->args.preview_args.ephemeral = true;
			(*currentIndex)++;
		}
		else
		{
			// If it's not a recognized option, we assume we're done parsing
			// for this command and break the loop.
			break;
		}
	}
	return true;
}

/*
================================================================================
 Public API Functions
================================================================================
*/

/**
 * See header file `cli_parser.h` for function documentation.
 */
bool cli_parse_args(int argc, char **argv, CommandLineArgs *parsed_args)
{
	initialize_args(parsed_args);

	// The program name is at argv[0]. The first command starts at argv[1].
	if (argc < 2)
	{
		print_help();
		return false; // Not an error, but no command was issued.
	}

	int i = 1; // Start parsing from the first argument after the program name.
	char *command = argv[i];

	if (strcmp(command, "help") == 0 || strcmp(command, "--help") == 0)
	{
		parsed_args->command = CMD_HELP;
		return true;
	}
	else if (strcmp(command, "preview") == 0)
	{
		parsed_args->command = CMD_PREVIEW;
		i++; // Move to the next token, which should be the subcommand.
		return parse_preview_args(argc, argv, &i, parsed_args);
	}
	else if (strcmp(command, "kube") == 0)
	{
		// Placeholder for 'kube' command parsing logic.
		fprintf(stderr, "Notice: 'kube' command is not yet implemented.\n");
		parsed_args->command = CMD_KUBE;
		return true;
	}
	else if (strcmp(command, "release") == 0)
	{
		// Placeholder for 'release' command parsing logic.
		fprintf(stderr,
			"Notice: 'release' command is not yet implemented.\n");
		parsed_args->command = CMD_RELEASE;
		return true;
	}

	fprintf(stderr, "Error: Unknown command '%s'. Use 'ph help' for a list of commands.\n",
		command);
	return false;
}

/**
 * @brief Dispatches the 'preview' command, constructing the JSON and calling the FFI.
 *
 * @param args A pointer to the populated PreviewArgs structure.
 * @return An integer status code from the FFI call.
 *
 * This helper function demonstrates the core responsibility of the dispatch layer:
 * converting the C struct into a JSON string that the Rust module understands.
 * The JSON format is the contract between the C and Rust layers.
 * It uses `snprintf` for safe string construction to prevent buffer overflows.
 */
static int dispatch_preview_command(const PreviewArgs *args)
{
	char json_config[1024]; // A sufficiently large buffer for the JSON string.
	const char *subcommand_str = "unknown";

	switch (args->subcommand)
	{
	case SUB_CMD_PREVIEW_CREATE:
		subcommand_str = "create";
		break;
	case SUB_CMD_PREVIEW_DESTROY:
		subcommand_str = "destroy";
		break;
	case SUB_CMD_PREVIEW_LIST:
		subcommand_str = "list";
		break;
	default:
		break; // Should not happen if parsing is correct.
	}

	// Safely construct the JSON string.
	// This format is the API contract with the Rust k8s_preview module.
	snprintf(json_config, sizeof(json_config),
		 "{\"subcommand\":\"%s\",\"pull_request_id\":%d,\"ttl\":\"%s\",\"tag\":\"%s\",\"ephemeral\":%s}",
		 subcommand_str, args->pull_request_id, args->ttl, args->tag,
		 args->ephemeral ? "true" : "false");

	printf("Dispatching 'preview' command...\n");
	printf("Generated JSON: %s\n", json_config);

	// TODO: Replace this with the actual FFI call.
	// This is the "missing link" that needs to be implemented in the FFI layer.
	// For example:
	// int status = ffi_call_preview_module(json_config);
	// return status;

	fprintf(stderr, "FFI call to Rust 'preview' module is not yet implemented.\n");

	return 0; // Return success for now.
}

/**
 * See header file `cli_parser.h` for function documentation.
 */
int cli_dispatch_command(const CommandLineArgs *args)
{
	if (!args)
	{
		return -1; // Invalid argument
	}

	switch (args->command)
	{
	case CMD_HELP:
		print_help();
		return 0;

	case CMD_PREVIEW:
		return dispatch_preview_command(&args->args.preview_args);

	case CMD_KUBE:
		// Placeholder for dispatching 'kube' command.
		// Example: return ffi_call_kube_module(...);
		printf("Dispatching 'kube' (not implemented).\n");
		return 0;

	case CMD_RELEASE:
		// Placeholder for dispatching 'release' command.
		// Example: return ffi_call_release_module(...);
		printf("Dispatching 'release' (not implemented).\n");
		return 0;

	case CMD_UNKNOWN:
	default:
		fprintf(stderr,
			"Error: Cannot dispatch an unknown or unparsed command.\n");
		return 1;
	}
}

/**
 * See header file `cli_parser.h` for function documentation.
 */
void free_command_line_args(CommandLineArgs *args)
{
	// Currently, no dynamic memory is allocated within CommandLineArgs, so this
	// function is a placeholder. If, for example, a command argument needed to
	// allocate a list of strings, the memory would be freed here. This provides
	// a robust pattern for future expansion.
	(void)args; // Suppress unused parameter warning.
}