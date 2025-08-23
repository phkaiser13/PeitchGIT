/* Copyright (C) 2025 Pedro Henrique / phkaiser13
 * config_manager.c - Manages application settings from a configuration file.
 *
 * This module is responsible for parsing, storing, and providing access to
 * application-wide configuration settings. It reads a simple key-value file
 * where keys and values are separated by an equals sign '='.
 *
 * Core responsibilities include:
 * 1. Loading settings from a specified file (e.g., "phgit.conf").
 * 2. Storing these settings in an efficient, queryable data structure.
 * 3. Providing getter functions for other modules to retrieve configuration
 * values. It supports retrieving values as strings or integers with
 * default fallbacks.
 * 4. Cleaning up all allocated resources upon application shutdown to prevent
 * memory leaks.
 *
 * The current implementation uses a fixed-size array of key-value pairs,
 * which is simple and avoids dynamic allocation complexity for a small number
 * of keys. Lines starting with '#' or ';' are treated as comments and ignored.
 *
 * SPDX-License-Identifier: Apache-2.0 */

#include "config_manager.h"
#include "libs/liblogger/Logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Constants for configuration parsing
#define MAX_CONFIG_ENTRIES 64 // Maximum number of key-value pairs
#define MAX_LINE_LENGTH 256	  // Maximum characters per line in the config file

// Internal structure for a single configuration entry
typedef struct
{
	char key[MAX_LINE_LENGTH];
	char value[MAX_LINE_LENGTH];
} ConfigEntry;

// Static (module-private) storage for configuration data
static ConfigEntry g_config_entries[MAX_CONFIG_ENTRIES];
static int g_config_count = 0; // Number of entries currently loaded

/**
 * @brief Trims leading and trailing whitespace from a string in-place.
 *
 * Modifies the input string by removing any space or tab characters from the
 * beginning and end.
 *
 * @param str The string to trim.
 */
static void trim_whitespace(char *str)
{
	if (!str)
		return;

	// Trim trailing whitespace
	char *end = str + strlen(str) - 1;
	while (end > str && (*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r'))
	{
		*end = '\0';
		end--;
	}

	// Trim leading whitespace
	char *start = str;
	while (*start == ' ' || *start == '\t' || *start == '\n' || *start == '\r')
	{
		start++;
	}

	// Shift the string to the left if leading whitespace was found
	if (start != str)
	{
		memmove(str, start, strlen(start) + 1);
	}
}

/**
 * @brief Parses a single line from the config file and stores it if valid.
 *
 * @param line The line to parse.
 * @return 0 on success, -1 on failure (e.g., malformed line or storage full).
 */
static int parse_and_store_line(const char *line)
{
	char key[MAX_LINE_LENGTH];
	char value[MAX_LINE_LENGTH];
	char mutable_line[MAX_LINE_LENGTH];

	// Make a mutable copy to use with strtok
	strncpy(mutable_line, line, MAX_LINE_LENGTH - 1);
	mutable_line[MAX_LINE_LENGTH - 1] = '\0'; // Ensure null-termination

	// Find the separator
	char *separator = strchr(mutable_line, '=');
	if (separator == NULL)
	{
		logger_log(LOG_LEVEL_WARN, "CONFIG", "Malformed line (no '=' separator): %s", line);
		return -1; // Not a valid key-value pair
	}

	// Split the line into key and value
	*separator = '\0';
	strncpy(key, mutable_line, MAX_LINE_LENGTH - 1);
	strncpy(value, separator + 1, MAX_LINE_LENGTH - 1);
	key[MAX_LINE_LENGTH - 1] = '\0';
	value[MAX_LINE_LENGTH - 1] = '\0';

	// Clean up key and value by trimming whitespace
	trim_whitespace(key);
	trim_whitespace(value);

	// Basic validation: ensure key is not empty
	if (strlen(key) == 0)
	{
		logger_log(LOG_LEVEL_WARN, "CONFIG", "Ignoring entry with empty key.");
		return -1;
	}

	// Check if storage is full
	if (g_config_count >= MAX_CONFIG_ENTRIES)
	{
		logger_log(LOG_LEVEL_ERROR, "CONFIG", "Configuration storage is full. Cannot add more entries.");
		return -1;
	}

	// Store the new entry
	strncpy(g_config_entries[g_config_count].key, key, MAX_LINE_LENGTH - 1);
	strncpy(g_config_entries[g_config_count].value, value, MAX_LINE_LENGTH - 1);
	g_config_count++;

	logger_log(LOG_LEVEL_DEBUG, "CONFIG", "Loaded config: %s = %s", key, value);

	return 0;
}

int config_load(const char *file_path)
{
	FILE *file = fopen(file_path, "r");
	if (!file)
	{
		logger_log(LOG_LEVEL_INFO, "CONFIG", "Configuration file not found at '%s'. Using default settings.", file_path);
		return -1; // Not a fatal error, but indicates no file was loaded
	}

	logger_log(LOG_LEVEL_INFO, "CONFIG", "Loading configuration from '%s'.", file_path);

	char line_buffer[MAX_LINE_LENGTH];
	int line_num = 0;
	g_config_count = 0; // Reset count before loading

	while (fgets(line_buffer, sizeof(line_buffer), file))
	{
		line_num++;
		trim_whitespace(line_buffer);

		// Skip empty lines or comments
		if (line_buffer[0] == '\0' || line_buffer[0] == '#' || line_buffer[0] == ';')
		{
			continue;
		}

		if (parse_and_store_line(line_buffer) != 0)
		{
			logger_log(LOG_LEVEL_WARN, "CONFIG", "Skipping invalid configuration entry at line %d.", line_num);
		}
	}

	fclose(file);
	return 0;
}

const char *config_get_string(const char *key, const char *default_value)
{
	for (int i = 0; i < g_config_count; ++i)
	{
		if (strcmp(g_config_entries[i].key, key) == 0)
		{
			return g_config_entries[i].value;
		}
	}
	return default_value;
}

int config_get_int(const char *key, int default_value)
{
	const char *value_str = config_get_string(key, NULL);
	if (value_str)
	{
		// Use strtol for robust integer conversion
		char *endptr;
		long val = strtol(value_str, &endptr, 10);

		// Check if conversion was successful
		if (*endptr == '\0' || *endptr == ' ' || *endptr == '\t')
		{
			return (int)val;
		}
	}
	return default_value;
}

void config_cleanup(void)
{
	// With the current static array implementation, no dynamic memory is allocated.
	// This function serves as a placeholder for future enhancements where
	// dynamic memory (e.g., linked list, hash map) might be used.
	// We reset the count to ensure a clean state if re-initialized.
	g_config_count = 0;
	logger_log(LOG_LEVEL_INFO, "CONFIG", "Configuration manager cleaned up.");
}
