/* Copyright (C) 2025 Pedro Henrique / phkaiser13
 * loader.c - Implementation of the dynamic module loader.
 *
 * This file contains the core logic for discovering, loading, validating, and
 * managing external modules. It uses platform-specific APIs to handle shared
 * libraries and directory traversal, abstracting these differences from the
 * rest of the application.
 *
 * The process for loading a module is as follows:
 * 1. Scan the specified directory for files with the correct extension.
 * 2. Attempt to load the file as a shared library.
 * 3. Resolve pointers to the five required functions defined in the API contract.
 * 4. If any function is missing, the module is invalid and rejected.
 * 5. Call the module's `module_get_info` to learn about it.
 * 6. Create a core context and call the module's `module_init` function.
 * 7. If initialization succeeds, the module is added to a global registry.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "loader.h"
#include "platform/platform.h"      // For MODULE_EXTENSION
#include "config/config_manager.h"  // For providing config access to modules
#include "libs/liblogger/Logger.h"  // For logging loading process
#include <stdlib.h>
#include <string.h>

// Platform-specific includes for dynamic loading and directory traversal
#ifdef PLATFORM_WINDOWS
#include <windows.h>
#else
#include <dlfcn.h>
#include <dirent.h>
#endif

// --- Module-level static variables ---

// A dynamic array to store pointers to all successfully loaded modules.
static LoadedModule** g_loaded_modules = NULL;
static int g_module_count = 0;
static int g_module_capacity = 0;

// The context passed to modules, containing pointers to core functions.
static GitphCoreContext g_core_context;


// --- Private Helper Functions ---

/**
 * @brief Adds a successfully loaded module to the internal registry.
 *        Handles dynamic resizing of the module array if necessary.
 */
static GitphStatus add_module_to_registry(LoadedModule* module) {
    if (g_module_count >= g_module_capacity) {
        int new_capacity = (g_module_capacity == 0) ? 8 : g_module_capacity * 2;
        LoadedModule** new_registry = realloc(g_loaded_modules, sizeof(LoadedModule*) * new_capacity);
        if (!new_registry) {
            logger_log(LOG_LEVEL_FATAL, "LOADER", "Failed to allocate memory for module registry.");
            return GITPH_ERROR_GENERAL;
        }
        g_loaded_modules = new_registry;
        g_module_capacity = new_capacity;
    }
    g_loaded_modules[g_module_count++] = module;
    return GITPH_SUCCESS;
}

/**
 * @brief Frees all memory associated with a LoadedModule struct.
 */
static void free_loaded_module(LoadedModule* module) {
    if (!module) return;
    free(module->file_path);
    // The strings inside module->info are owned by the module, not us.
    // We only free the container struct.
    free(module);
}

// --- Public API Implementation ---

/**
 * @see loader.h
 */
GitphStatus modules_load(const char* directory_path) {
    char log_buffer[512];
    snprintf(log_buffer, sizeof(log_buffer), "Scanning for modules in: %s", directory_path);
    logger_log(LOG_LEVEL_INFO, "LOADER", log_buffer);

    // Setup the core context to be passed to modules
    g_core_context.log = logger_log;
    g_core_context.get_config_value = config_get_value;
    // g_core_context.print_ui will be set once the TUI is initialized.

#ifdef PLATFORM_WINDOWS
    // --- Windows Implementation ---
    char search_path[MAX_PATH];
    snprintf(search_path, sizeof(search_path), "%s\\*%s", directory_path, MODULE_EXTENSION);

    WIN32_FIND_DATA fd;
    HANDLE hFind = FindFirstFile(search_path, &fd);
    if (hFind == INVALID_HANDLE_VALUE) {
        logger_log(LOG_LEVEL_WARN, "LOADER", "Could not find any modules or read directory.");
        return GITPH_SUCCESS; // Not a fatal error
    }

    do {
        char full_path[MAX_PATH];
        snprintf(full_path, sizeof(full_path), "%s\\%s", directory_path, fd.cFileName);

        HMODULE handle = LoadLibrary(full_path);
        if (!handle) {
            snprintf(log_buffer, sizeof(log_buffer), "Failed to load library: %s", full_path);
            logger_log(LOG_LEVEL_ERROR, "LOADER", log_buffer);
            continue;
        }

        // Resolve all required functions
        PFN_module_get_info get_info_func = (PFN_module_get_info)GetProcAddress(handle, "module_get_info");
        PFN_module_init init_func = (PFN_module_init)GetProcAddress(handle, "module_init");
        PFN_module_exec exec_func = (PFN_module_exec)GetProcAddress(handle, "module_exec");
        PFN_module_cleanup cleanup_func = (PFN_module_cleanup)GetProcAddress(handle, "module_cleanup");

        if (!get_info_func || !init_func || !exec_func || !cleanup_func) {
            snprintf(log_buffer, sizeof(log_buffer), "Module '%s' does not conform to API contract. Skipping.", full_path);
            logger_log(LOG_LEVEL_ERROR, "LOADER", log_buffer);
            FreeLibrary(handle);
            continue;
        }

        // Initialize and register the module
        const GitphModuleInfo* info = get_info_func();
        if (init_func(&g_core_context) != GITPH_SUCCESS) {
            snprintf(log_buffer, sizeof(log_buffer), "Module '%s' failed to initialize. Skipping.", info->name);
            logger_log(LOG_LEVEL_ERROR, "LOADER", log_buffer);
            FreeLibrary(handle);
            continue;
        }

        LoadedModule* new_module = malloc(sizeof(LoadedModule));
        new_module->handle = handle;
        new_module->file_path = strdup(full_path);
        new_module->info = *info;
        new_module->init_func = init_func;
        new_module->exec_func = exec_func;
        new_module->cleanup_func = cleanup_func;

        add_module_to_registry(new_module);
        snprintf(log_buffer, sizeof(log_buffer), "Successfully loaded module: %s (v%s)", info->name, info->version);
        logger_log(LOG_LEVEL_INFO, "LOADER", log_buffer);

    } while (FindNextFile(hFind, &fd) != 0);
    FindClose(hFind);

#else
    // --- POSIX Implementation ---
    DIR* d = opendir(directory_path);
    if (!d) {
        logger_log(LOG_LEVEL_ERROR, "LOADER", "Cannot open modules directory.");
        return GITPH_ERROR_NOT_FOUND;
    }

    struct dirent* dir;
    while ((dir = readdir(d)) != NULL) {
        if (strstr(dir->d_name, MODULE_EXTENSION) == NULL) {
            continue; // Not a shared library
        }

        char full_path[1024];
        snprintf(full_path, sizeof(full_path), "%s/%s", directory_path, dir->d_name);

        void* handle = dlopen(full_path, RTLD_LAZY);
        if (!handle) {
            snprintf(log_buffer, sizeof(log_buffer), "Failed to load library: %s (Reason: %s)", full_path, dlerror());
            logger_log(LOG_LEVEL_ERROR, "LOADER", log_buffer);
            continue;
        }

        // Clear any existing error
        dlerror();

        // Resolve all required functions
        PFN_module_get_info get_info_func = dlsym(handle, "module_get_info");
        PFN_module_init init_func = dlsym(handle, "module_init");
        PFN_module_exec exec_func = dlsym(handle, "module_exec");
        PFN_module_cleanup cleanup_func = dlsym(handle, "module_cleanup");

        const char* dlsym_error = dlerror();
        if (dlsym_error) {
            snprintf(log_buffer, sizeof(log_buffer), "Module '%s' does not conform to API contract. Skipping. (Error: %s)", full_path, dlsym_error);
            logger_log(LOG_LEVEL_ERROR, "LOADER", log_buffer);
            dlclose(handle);
            continue;
        }

        // Initialize and register the module
        const GitphModuleInfo* info = get_info_func();
        if (init_func(&g_core_context) != GITPH_SUCCESS) {
            snprintf(log_buffer, sizeof(log_buffer), "Module '%s' failed to initialize. Skipping.", info->name);
            logger_log(LOG_LEVEL_ERROR, "LOADER", log_buffer);
            dlclose(handle);
            continue;
        }

        LoadedModule* new_module = malloc(sizeof(LoadedModule));
        new_module->handle = handle;
        new_module->file_path = strdup(full_path);
        new_module->info = *info;
        new_module->init_func = init_func;
        new_module->exec_func = exec_func;
        new_module->cleanup_func = cleanup_func;

        add_module_to_registry(new_module);
        snprintf(log_buffer, sizeof(log_buffer), "Successfully loaded module: %s (v%s)", info->name, info->version);
        logger_log(LOG_LEVEL_INFO, "LOADER", log_buffer);
    }
    closedir(d);
#endif

    return GITPH_SUCCESS;
}

/**
 * @see loader.h
 */
const LoadedModule* modules_find_handler(const char* command) {
    for (int i = 0; i < g_module_count; ++i) {
        const char** cmd_ptr = g_loaded_modules[i]->info.commands;
        while (cmd_ptr && *cmd_ptr) {
            if (strcmp(*cmd_ptr, command) == 0) {
                return g_loaded_modules[i];
            }
            cmd_ptr++;
        }
    }
    return NULL;
}

/**
 * @see loader.h
 */
const LoadedModule** modules_get_all(int* count) {
    if (count) {
        *count = g_module_count;
    }
    // We can return the raw list, but a safer approach would be a copy
    // or ensuring it's NULL-terminated for easier iteration.
    // For now, returning the direct pointer is efficient.
    return (const LoadedModule**)g_loaded_modules;
}

/**
 * @see loader.h
 */
void modules_cleanup(void) {
    logger_log(LOG_LEVEL_INFO, "LOADER", "Cleaning up all loaded modules.");
    for (int i = 0; i < g_module_count; ++i) {
        LoadedModule* module = g_loaded_modules[i];
        if (module) {
            // Call the module's own cleanup function first
            if (module->cleanup_func) {
                module->cleanup_func();
            }
            // Unload the library
#ifdef PLATFORM_WINDOWS
            FreeLibrary(module->handle);
#else
            dlclose(module->handle);
#endif
            // Free the container struct
            free_loaded_module(module);
        }
    }
    free(g_loaded_modules);
    g_loaded_modules = NULL;
    g_module_count = 0;
    g_module_capacity = 0;
}