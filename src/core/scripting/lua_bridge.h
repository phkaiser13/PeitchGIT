/* Copyright (C) 2025 Pedro Henrique / phkaiser13
 * config_manager.h - Interface for the Core Configuration Manager.
 *
 * This header defines the public API for the configuration manager, which is
 * responsible for loading, parsing, accessing, and persisting key-value
 * configuration settings for the application.
 *
 * The manager now supports both reading and writing operations. It loads
 * settings from a specified file on initialization and can save any changes
 * back to the same file, ensuring persistence across sessions.
 *
 * Key changes in this version:
 * - `config_get_value` now returns a dynamically allocated copy of the value,
 *   which the caller is responsible for freeing. This prevents memory corruption
 *   when the configuration is modified by other parts of the application.
 * - `config_set_value` has been added to allow for creating new settings or
 *   updating existing ones programmatically.
 *
 * SPDX-License-Identifier: Apache-2.0 */

#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include "../../ipc/include/phgit_core_api.h" // For phgitStatus
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initializes the configuration manager by loading settings from a file.
 *
 * This function reads a specified configuration file, parses its key-value pairs,
 * and stores them in memory for fast access. It must be called before any other
 * function in this module.
 *
 * @param filepath The path to the configuration file to load.
 * @return phgit_SUCCESS on successful loading and parsing, or an error code
 *         (e.g., phgit_ERROR_IO, phgit_ERROR_INIT_FAILED) on failure.
 */
phgitStatus config_init(const char* filepath);

/**
 * @brief Retrieves the value for a given configuration key.
 *
 * This function searches for a configuration key in the in-memory store and
 * returns a copy of its associated value.
 *
 * CRITICAL: The returned string is dynamically allocated (`strdup`) and ownership
 * is transferred to the caller. The caller MUST free the returned pointer after
 * use to prevent memory leaks.
 *
 * @param key The configuration key to look up (e.g., "user.name").
 * @return A pointer to a newly allocated string containing the value, or NULL
 *         if the key is not found. The caller must free this pointer.
 */
char* config_get_value(const char* key);

/**
 * @brief Sets or updates a configuration key with a new value.
 *
 * This function sets a key to a specified value. If the key already exists,
 * its value is updated. If the key does not exist, a new key-value pair is
 * created. After successfully setting the value, the changes are automatically
 * persisted to the configuration file.
 *
 * @param key The configuration key to set (e.g., "user.email").
 * @param value The value to associate with the key.
 * @return phgit_SUCCESS on success, or phgit_ERROR_ALLOC_FAILED if memory
 *         allocation fails, or phgit_ERROR_IO if saving to the file fails.
 */
phgitStatus config_set_value(const char* key, const char* value);

/**
 * @brief Frees all resources used by the configuration manager.
 *
 * This function deallocates all memory associated with the loaded configuration
 * entries, including keys, values, and the main entry array. It should be called
 * at application shutdown to ensure a clean exit.
 */
void config_cleanup(void);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // CONFIG_MANAGER_H