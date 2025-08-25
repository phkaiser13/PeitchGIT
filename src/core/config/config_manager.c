/* Copyright (C) 2025 Pedro Henrique / phkaiser13
 * config_manager.c - Implementation of the application configuration manager.
 *
 * This file implements the configuration management functionality using a hash
 * table for efficient key-value storage and retrieval. This approach ensures
 * fast lookups (average O(1) time complexity), which is essential for a
 * responsive application.
 *
 * The implementation handles:
 * - Parsing of `key=value` files, ignoring comments and whitespace.
 * - Dynamic allocation and safe copying of keys and values.
 * - In-memory creation and modification of configuration pairs.
 * - A string hashing function (djb2) to map keys to table indices.
 * - Collision resolution using separate chaining (linked lists).
 * - A thorough cleanup mechanism to prevent memory leaks.
 *
 * SPDX-License-Identifier: Apache-2.0 */

#include "config_manager.h"
#include "libs/liblogger/Logger.hpp" // For logging parsing warnings
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// --- Internal Data Structures ---

#define HASH_TABLE_SIZE 128 // A prime number is often a good choice

/**
 * @struct ConfigNode
 * @brief A node in the hash table's linked list (for collision handling).
 */
typedef struct ConfigNode {
    char* key;
    char* value;
    struct ConfigNode* next;
} ConfigNode;

// The global hash table. It's an array of pointers to ConfigNodes.
// Declared `static` to be private to this file.
static ConfigNode* g_config_table[HASH_TABLE_SIZE] = {NULL};


// --- Private Helper Functions ---

/**
 * @brief The djb2 hash function for strings.
 *
 * A simple, fast, and effective hashing algorithm for strings.
 * See: http://www.cse.yorku.ca/~oz/hash.html
 *
 * @param str The string to hash.
 * @return The calculated hash value.
 */
static unsigned long hash_string(const char* str) {
    unsigned long hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    }
    return hash;
}

/**
 * @brief Trims leading and trailing whitespace from a string in-place.
 * @param str The string to trim.
 * @return A pointer to the beginning of the trimmed string.
 */
static char* trim_whitespace(char* str) {
    if (!str) return NULL;
    char* end;

    // Trim leading space
    while (isspace((unsigned char)*str)) str++;

    if (*str == 0) // All spaces?
        return str;

    // Trim trailing space
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;

    // Write new null terminator
    *(end + 1) = '\0';

    return str;
}

// --- Public API Implementation ---

/**
 * @see config_manager.h
 */
void config_cleanup(void) {
    for (int i = 0; i < HASH_TABLE_SIZE; ++i) {
        ConfigNode* current = g_config_table[i];
        while (current != NULL) {
            ConfigNode* next = current->next;
            free(current->key);
            free(current->value);
            free(current);
            current = next;
        }
        g_config_table[i] = NULL;
    }
}

/**
 * @see config_manager.h
 */
phStatus config_load(const char* filename) {
    // Ensure the previous configuration is cleared before loading a new one.
    config_cleanup();

    FILE* file = fopen(filename, "r");
    if (!file) {
        // It's not an error if the config file doesn't exist.
        // The application will just use default values.
        logger_log(LOG_LEVEL_INFO, "CONFIG", "Configuration file not found. Using defaults.");
        return ph_SUCCESS;
    }

    char line[1024];
    int line_number = 0;
    while (fgets(line, sizeof(line), file)) {
        line_number++;
        char* trimmed_line = trim_whitespace(line);

        if (strlen(trimmed_line) == 0 || trimmed_line[0] == '#') {
            continue; // Skip empty or commented lines
        }

        char* separator = strchr(trimmed_line, '=');
        if (!separator) {
            logger_log_fmt(LOG_LEVEL_WARN, "CONFIG", "Malformed line %d in config file. Skipping.", line_number);
            continue;
        }

        *separator = '\0'; // Split the line into key and value
        char* key = trim_whitespace(trimmed_line);
        char* value = trim_whitespace(separator + 1);

        if (strlen(key) == 0) {
            logger_log_fmt(LOG_LEVEL_WARN, "CONFIG", "Empty key on line %d in config file. Skipping.", line_number);
            continue;
        }

        // Use the new set function to add the value, ensuring consistency.
        config_set_value(key, value);
    }

    fclose(file);
    logger_log(LOG_LEVEL_INFO, "CONFIG", "Configuration loaded successfully.");
    return ph_SUCCESS;
}

/**
 * @see config_manager.h
 */
char* config_get_value(const char* key) {
    if (!key) {
        return NULL;
    }

    unsigned long hash = hash_string(key);
    unsigned int index = hash % HASH_TABLE_SIZE;

    ConfigNode* current = g_config_table[index];
    while (current != NULL) {
        if (strcmp(current->key, key) == 0) {
            // Return a copy that the caller is responsible for freeing.
            // This is critical for memory safety with external modules.
            return strdup(current->value);
        }
        current = current->next;
    }

    return NULL; // Key not found
}

/**
 * @see config_manager.h
 */
phStatus config_set_value(const char* key, const char* value) {
    if (!key || !value) {
        return ph_ERROR_INVALID_ARGS;
    }

    unsigned long hash = hash_string(key);
    unsigned int index = hash % HASH_TABLE_SIZE;

    // First, check if the key already exists to update it.
    ConfigNode* current = g_config_table[index];
    while (current != NULL) {
        if (strcmp(current->key, key) == 0) {
            // Key found, update the value.
            char* new_value = strdup(value);
            if (!new_value) {
                logger_log(LOG_LEVEL_FATAL, "CONFIG", "Memory allocation failed for config value update.");
                return ph_ERROR_GENERAL;
            }
            free(current->value); // Free the old value
            current->value = new_value; // Assign the new one
            return ph_SUCCESS;
        }
        current = current->next;
    }

    // If we reach here, the key does not exist. Create a new node.
    ConfigNode* new_node = (ConfigNode*)malloc(sizeof(ConfigNode));
    if (!new_node) {
        logger_log(LOG_LEVEL_FATAL, "CONFIG", "Memory allocation failed for new config node.");
        return ph_ERROR_GENERAL;
    }

    new_node->key = strdup(key);
    new_node->value = strdup(value);

    // Check for allocation failures during strdup and clean up if necessary
    if (!new_node->key || !new_node->value) {
        logger_log(LOG_LEVEL_FATAL, "CONFIG", "Memory allocation failed for new config key/value.");
        free(new_node->key);   // free() on NULL is safe
        free(new_node->value); // free() on NULL is safe
        free(new_node);
        return ph_ERROR_GENERAL;
    }

    // Prepend the new node to the list at the calculated index.
    new_node->next = g_config_table[index];
    g_config_table[index] = new_node;

    return ph_SUCCESS;
}