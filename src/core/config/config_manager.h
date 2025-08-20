/* Copyright (C) 2025 Pedro Henrique / phkaiser13
 * config_manager.h - Interface for the application configuration manager.
 *
 * This header file defines the public API for the configuration manager module.
 * This module is responsible for parsing and providing access to settings
 * stored in a configuration file (e.g., `.phgit.conf`). The file format is a
 * simple key-value store, with one `key=value` pair per line.
 *
 * The manager abstracts the file I/O and parsing logic, providing clean
 * functions for the rest of the application to load settings, retrieve
 * specific values, set new values, and clean up resources upon exit. This
 * centralization prevents configuration logic from being scattered throughout
 * the codebase.
 *
 * SPDX-License-Identifier: Apache-2.0 */

#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include "../../ipc/include/phgit_core_api.h" // For phgitStatus enum

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Loads configuration settings from a specified file into memory.
 *
 * This function reads the given file line by line, parsing `key=value` pairs.
 * It ignores empty lines and lines starting with '#' (comments). Any existing
 * configuration in memory is cleared before loading the new file. If the file
 * cannot be opened, it returns an error, but the application can proceed with
 * default values.
 *
 * @param filename The path to the configuration file.
 * @return phgit_SUCCESS if the file was loaded successfully or if it doesn't
 *         exist (which is not a fatal error). Returns an error code like
 *         phgit_ERROR_GENERAL on file read errors.
 */
phgitStatus config_load(const char* filename);

/**
 * @brief Retrieves a configuration value for a given key.
 *
 * This function performs a lookup in the in-memory configuration store.
 * It returns a NEWLY ALLOCATED string containing the value. The caller
 * OWNS this memory and MUST free it using `free()` when it is no longer needed.
 * This design prevents memory corruption when the value is passed to other
 * modules or scripting engines that might try to manage its lifecycle.
 *
 * @param key The null-terminated string key to look up.
 * @return A pointer to a newly allocated value string, or NULL if the key is
 *         not found or if memory allocation fails.
 */
char* config_get_value(const char* key);

/**
 * @brief Sets or updates a configuration value in memory.
 *
 * This function adds a new key-value pair to the configuration or updates the
 * value of an existing key. The key and value strings are copied internally,
 * so the caller does not need to keep the original strings valid after this
 * function returns. This function does not persist the change to a file.
 *
 * @param key The null-terminated string key to set. Cannot be NULL.
 * @param value The null-terminated string value to associate with the key. Cannot be NULL.
 * @return phgit_SUCCESS on success, or an error code (e.g., phgit_ERROR_GENERAL)
 *         if memory allocation fails.
 */
phgitStatus config_set_value(const char* key, const char* value);

/**
 * @brief Frees all resources used by the configuration manager.
 *
 * This function should be called once at application shutdown to deallocate
 * all memory used for storing the configuration keys and values, preventing
 * memory leaks.
 */
void config_cleanup(void);


#ifdef __cplusplus
} // extern "C"
#endif

#endif // CONFIG_MANAGER_H