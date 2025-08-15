/* Copyright (C) 2025 Pedro Henrique / phkaiser13
 * tui.c - Implementation of the Text-based User Interface.
 *
 * This file contains the logic for the interactive menu of the application.
 * It dynamically generates a list of options based on the commands exposed
 * by all successfully loaded modules.
 *
 * The main loop performs the following steps:
 * 1. Clears the screen for a clean presentation.
 * 2. Prints a static header.
 * 3. Fetches all loaded modules via the module loader.
 * 4. Iterates through the modules and their commands to build and display a
 *    numbered list of options. A temporary mapping is created to link the
 *    menu number to the corresponding command and module.
 * 5. Prompts the user for input.
 * 6. On valid input, it simulates a command-line call by passing the chosen
 *    command to the `cli_dispatch_command` function. This promotes code reuse
 *    and keeps the TUI's responsibility focused on presentation.
 *
 * SPDX-License-Identifier: Apache-2.0 */

#include "tui.h"
#include "platform/platform.h"
#include "module_loader/loader.h"
#include "cli/cli_parser.h" // To dispatch commands
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// --- Private Helper Structures and Functions ---

/**
 * @struct MenuItem
 * @brief A temporary structure to map a menu index to a command.
 */
typedef struct {
    const char* command_alias;
    const LoadedModule* module;
} MenuItem;

/**
 * @brief A simple helper to wait for the user to press Enter.
 */
static void wait_for_enter(void) {
    printf("\nPress Enter to continue...");
    // Clear the input buffer and wait for a newline
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
    if (c == EOF) { // If stdin closes, we should not loop forever
        return;
    }
    // If the previous getchar didn't consume the newline, this one will
    if (c != '\n') {
        getchar();
    }
}

// --- Public API Implementation ---

/**
 * @see tui.h
 */
void tui_show_main_menu(void) {
    MenuItem* menu_items = NULL;
    int menu_item_count = 0;

    while (1) {
        platform_clear_screen();

        // --- Header ---
        printf("========================================\n");
        printf("  phgit - The Polyglot Git Helper\n");
        printf("========================================\n\n");
        printf("Please select an option:\n\n");

        // --- Dynamic Menu Generation ---
        int module_count = 0;
        const LoadedModule** modules = modules_get_all(&module_count);

        // Free previous menu items if any
        free(menu_items);
        menu_items = NULL;
        menu_item_count = 0;

        // Allocate space for all possible commands
        int total_commands = 0;
        for (int i = 0; i < module_count; i++) {
            const char** cmd_ptr = modules[i]->info.commands;
            while (cmd_ptr && *cmd_ptr) {
                total_commands++;
                cmd_ptr++;
            }
        }
        menu_items = malloc(sizeof(MenuItem) * total_commands);
        if (!menu_items) {
            tui_print_error("Failed to allocate memory for the menu.");
            return;
        }

        // Populate and print the menu
        for (int i = 0; i < module_count; i++) {
            const char** cmd_ptr = modules[i]->info.commands;
            while (cmd_ptr && *cmd_ptr) {
                printf("  [%2d] %s\n", menu_item_count + 1, *cmd_ptr);
                menu_items[menu_item_count].command_alias = *cmd_ptr;
                menu_items[menu_item_count].module = modules[i];
                menu_item_count++;
                cmd_ptr++;
            }
        }

        printf("\n  [%2d] Exit\n", menu_item_count + 1);
        printf("\n----------------------------------------\n");

        // --- User Input and Dispatch ---
        char input_buffer[16];
        if (!tui_prompt_user("Your choice: ", input_buffer, sizeof(input_buffer))) {
            break; // Exit on EOF or read error
        }

        long choice = strtol(input_buffer, NULL, 10);

        if (choice > 0 && choice <= menu_item_count) {
            // A valid command was chosen
            const MenuItem* selected_item = &menu_items[choice - 1];

            // We simulate a command-line call: `phgit <command>`
            // This reuses the entire CLI dispatch logic.
            const char* argv[] = { "phgit", selected_item->command_alias, NULL };
            int argc = 2;

            printf("\nExecuting '%s'...\n", selected_item->command_alias);
            printf("----------------------------------------\n");

            cli_dispatch_command(argc, argv);

            printf("----------------------------------------\n");
            wait_for_enter();

        } else if (choice == menu_item_count + 1) {
            // Exit was chosen
            break;
        } else {
            // Invalid input
            tui_print_error("Invalid choice. Please try again.");
            wait_for_enter();
        }
    }

    free(menu_items);
    printf("\nExiting phgit. Goodbye!\n");
}

/**
 * @see tui.h
 */
void tui_print_error(const char* message) {
    // In a more advanced TUI, this would use colors.
    fprintf(stderr, "\n[ERROR] %s\n", message);
}

/**
 * @see tui.h
 */
void tui_print_success(const char* message) {
    printf("\n[SUCCESS] %s\n", message);
}

/**
 * @see tui.h
 */
bool tui_prompt_user(const char* prompt, char* buffer, size_t buffer_size) {
    printf("%s", prompt);
    fflush(stdout); // Ensure prompt is displayed before waiting for input

    if (fgets(buffer, buffer_size, stdin) == NULL) {
        // Handle EOF (Ctrl+D) or read error
        return false;
    }

    // Remove trailing newline character, if present
    buffer[strcspn(buffer, "\r\n")] = '\0';

    return true;
}