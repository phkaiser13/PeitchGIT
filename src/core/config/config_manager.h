/* Copyright (C) 2025 Pedro Henrique
 * config_manager.h - Interface for the application configuration manager.
 *
 * This header file defines the public API for the configuration manager module.
 * This module is responsible for parsing and providing access to settings
 * stored in a configuration file (e.g., `.gitph.conf`). The file format is a
 * simple key-value store, with one `key=value` pair per line.
 *
 * The manager abstracts the file I/O and parsing logic, providing clean
 * functions for the rest of the application to load settings, retrieve
 * specific values, and clean up resources upon exit. This centralization
 * prevents configuration logic from being scattered throughout the codebase.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later */

#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include "../../ipc/include/gitph_core_api.h" // For GitphStatus enum

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
 * @return GITPH_SUCCESS if the file was loaded successfully or if it doesn't
 *         exist (which is not a fatal error). Returns an error code like
 *         GITPH_ERROR_GENERAL on file read errors.
 */
GitphStatus config_load(const char* filename);

/**
 * @brief Retrieves a configuration value for a given key.
 *
 * This function performs a lookup in the in-memory configuration store.
 * The returned string is a pointer to the internally stored value and MUST NOT
 * be modified or freed by the caller. It remains valid until `config_cleanup`
 * is called.
 *
 * @param key The null-terminated string key to look up.
 * @return A read-only pointer to the value string, or NULL if the key is not
 *         found.
 */
const char* config_get_value(const char* key);

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