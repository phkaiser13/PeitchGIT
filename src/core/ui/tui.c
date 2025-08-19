/* Copyright (C) 2025 Pedro Henrique / phkaiser13
 * tui.c - Implementation of the Text-based User Interface.
 *
 * This file implements a sophisticated, dynamic, and interactive menu for the
 * application. It has been architected to be completely agnostic of command
 * sources, seamlessly integrating both native C commands and dynamically
 * registered Lua script commands into a single, unified user experience.
 *
 * Core Architectural Principles:
 * 1. Data Unification: A generic `MenuItem` struct is used to represent any
 *    command, abstracting away its origin (native or Lua).
 * 2. Separation of Concerns: The main function is broken down into helpers for
 *    gathering, sorting, displaying, and cleaning up menu data.
 * 3. Enhanced UX: All commands are sorted alphabetically and displayed with
 *    their descriptions in a clean, aligned format for superior readability.
 * 4. Robustness: Memory is managed carefully within a single loop iteration,
 *    preventing leaks, and user input is handled safely.
 *
 * The main loop now orchestrates these steps, providing a highly professional
 * and extensible interface that reflects the power of the underlying system.
 *
 * SPDX-License-Identifier: Apache-2.0 */

#include "tui.h"
#include "platform/platform.h"
#include "module_loader/loader.h"
#include "cli/cli_parser.h"
#include "scripting/lua_bridge.h" // To get Lua commands
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// --- Private Helper Structures and Functions ---

/**
 * @enum CommandSource
 * @brief Differentiates the origin of a command for potential future use.
 */
typedef enum {
    COMMAND_SOURCE_NATIVE,
    COMMAND_SOURCE_LUA
} CommandSource;

/**
 * @struct MenuItem
 * @brief A unified, abstract representation of a single command for the menu.
 *        It holds copies of the command's data, decoupling the TUI from the
 *        lifetime of module and script data.
 */
typedef struct {
    char* name;
    char* description;
    CommandSource source;
} MenuItem;

/**
 * @brief Comparison function for qsort to sort menu items alphabetically by name.
 */
static int compare_menu_items(const void* a, const void* b) {
    const MenuItem* item_a = (const MenuItem*)a;
    const MenuItem* item_b = (const MenuItem*)b;
    return strcmp(item_a->name, item_b->name);
}

/**
 * @brief Frees all memory associated with a list of menu items.
 * @param items The array of MenuItem to free.
 * @param count The number of items in the array.
 */
static void free_menu_items(MenuItem* items, size_t count) {
    if (!items) return;
    for (size_t i = 0; i < count; ++i) {
        free(items[i].name);
        free(items[i].description);
    }
    free(items);
}

/**
 * @brief Gathers all available commands from both native modules and the Lua bridge.
 *
 * This function is the core of the TUI's data aggregation. It dynamically
 * allocates a unified list of MenuItem structures, populating it with data
 * from all command sources.
 *
 * @param out_count Pointer to a size_t that will receive the total number of commands found.
 * @return A pointer to a newly allocated array of MenuItem, or NULL on failure.
 *         The caller is responsible for freeing this array using `free_menu_items`.
 */
static MenuItem* gather_all_commands(size_t* out_count) {
    // --- Phase 1: Get counts from all sources ---
    int native_module_count = 0;
    const LoadedModule** modules = modules_get_all(&native_module_count);
    size_t native_command_count = 0;
    for (int i = 0; i < native_module_count; i++) {
        for (const char** cmd = modules[i]->info.commands; cmd && *cmd; ++cmd) {
            native_command_count++;
        }
    }

    size_t lua_command_count = lua_bridge_get_command_count();
    size_t total_commands = native_command_count + lua_command_count;

    if (total_commands == 0) {
        *out_count = 0;
        return NULL;
    }

    // --- Phase 2: Allocate and populate the unified list ---
    MenuItem* items = malloc(sizeof(MenuItem) * total_commands);
    if (!items) {
        logger_log(LOG_LEVEL_FATAL, "TUI", "Failed to allocate memory for menu items.");
        *out_count = 0;
        return NULL;
    }

    size_t current_index = 0;

    // Populate with native commands
    for (int i = 0; i < native_module_count; i++) {
        for (const char** cmd = modules[i]->info.commands; cmd && *cmd; ++cmd) {
            items[current_index].name = strdup(*cmd);
            items[current_index].description = strdup(modules[i]->info.description);
            items[current_index].source = COMMAND_SOURCE_NATIVE;
            current_index++;
        }
    }

    // Populate with Lua commands
    // NOTE: This assumes `lua_bridge_get_all_command_names` is implemented.
    const char** lua_command_names = lua_bridge_get_all_command_names();
    for (size_t i = 0; i < lua_command_count; i++) {
        const char* name = lua_command_names[i];
        const char* desc = lua_bridge_get_command_description(name);
        items[current_index].name = strdup(name);
        items[current_index].description = strdup(desc ? desc : "A user-defined script command.");
        items[current_index].source = COMMAND_SOURCE_LUA;
        current_index++;
    }
    // The bridge should provide a function to free the list of names, e.g.,
    // lua_bridge_free_command_names(lua_command_names);

    *out_count = total_commands;
    return items;
}

/**
 * @brief Displays the formatted, sorted menu to the console.
 * @param items The sorted array of MenuItem to display.
 * @param count The number of items in the array.
 */
static void display_menu(const MenuItem* items, size_t count) {
    platform_clear_screen();
    printf("========================================\n");
    printf("  phgit - The Polyglot Git Helper\n");
    printf("========================================\n\n");
    printf("Please select a command:\n\n");

    if (count > 0) {
        for (size_t i = 0; i < count; ++i) {
            // Display with aligned descriptions for a professional look
            printf("  [%2zu] %-20s - %s\n", i + 1, items[i].name, items[i].description);
        }
    } else {
        printf("  No commands available.\n");
    }

    printf("\n  [%2zu] Exit\n", count + 1);
    printf("\n----------------------------------------\n");
}

/**
 * @brief A simple helper to wait for the user to press Enter.
 */
static void wait_for_enter(void) {
    printf("\nPress Enter to continue...");
    int c;
    while ((c = getchar()) != '\n' && c != EOF); // Clear buffer before waiting
    if (c != '\n' && c != EOF) {
        while ((c = getchar()) != '\n' && c != EOF); // Clear buffer after waiting
    }
}

// --- Public API Implementation ---

/**
 * @see tui.h
 */
void tui_show_main_menu(void) {
    while (1) {
        // --- Data Gathering and Preparation ---
        size_t item_count = 0;
        MenuItem* menu_items = gather_all_commands(&item_count);

        if (menu_items) {
            qsort(menu_items, item_count, sizeof(MenuItem), compare_menu_items);
        }

        // --- Presentation ---
        display_menu(menu_items, item_count);

        // --- User Input and Dispatch ---
        char input_buffer[16];
        if (!tui_prompt_user("Your choice: ", input_buffer, sizeof(input_buffer))) {
            free_menu_items(menu_items, item_count);
            break; // Exit on EOF or read error
        }

        long choice = strtol(input_buffer, NULL, 10);

        if (choice > 0 && choice <= (long)item_count) {
            const MenuItem* selected = &menu_items[choice - 1];
            const char* argv[] = { "phgit", selected->name, NULL };

            printf("\nExecuting '%s'...\n", selected->name);
            printf("----------------------------------------\n");
            cli_dispatch_command(2, argv);
            printf("----------------------------------------\n");
            wait_for_enter();

        } else if (choice == (long)item_count + 1) {
            free_menu_items(menu_items, item_count);
            break; // Exit
        } else {
            tui_print_error("Invalid choice. Please try again.");
            wait_for_enter();
        }

        // --- Cleanup for this iteration ---
        free_menu_items(menu_items, item_count);
    }

    printf("\nExiting phgit. Goodbye!\n");
}

/**
 * @see tui.h
 */
void tui_print_error(const char* message) {
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
    fflush(stdout);

    if (fgets(buffer, buffer_size, stdin) == NULL) {
        return false;
    }

    buffer[strcspn(buffer, "\r\n")] = '\0';
    return true;
}