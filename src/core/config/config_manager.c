/*
 * config_manager.c - Improved implementation for Peitch GUI core
 *
 * Changes made (summary):
 *  - HASH_TABLE_SIZE set to a prime (131).
 *  - Added safe_strdup() with allocation checks.
 *  - Explicit ownership documented: config_get_value() returns a heap-allocated
 *    string the caller must free; config_get_ref() returns a const pointer into
 *    the table (valid until config_cleanup or until that key is updated/removed).
 *  - Optional thread-safety via -DTHREAD_SAFE (uses pthread mutex).
 *  - Consistent use of phgitStatus return codes.
 *  - Defensive checks for allocation failures and improved logging.
 *  - Minor API helpers: config_has_key(), config_remove().
 *
 * Note: This implementation expects a corresponding header `config_manager.h`
 * that declares phgitStatus enum and prototypes used by the rest of the core.
 * Make sure to document ownership semantics in that header as well.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "config_manager.h"
#include "libs/liblogger/Logger.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifdef THREAD_SAFE
#include <pthread.h>
#endif

// --- Tunables ---------------------------------------------------------------
// Use a prime number for the table size to reduce clustering.
#define HASH_TABLE_SIZE 131
// Maximum line length expected in config file (including newline).
#define MAX_LINE_LENGTH 1024

// --- Internal data structures ----------------------------------------------
typedef struct ConfigNode {
    char* key;           // heap-allocated
    char* value;         // heap-allocated
    struct ConfigNode* next;
} ConfigNode;

static ConfigNode* g_config_table[HASH_TABLE_SIZE] = { NULL };

#ifdef THREAD_SAFE
static pthread_mutex_t g_config_mutex = PTHREAD_MUTEX_INITIALIZER;
# define LOCK()   pthread_mutex_lock(&g_config_mutex)
# define UNLOCK() pthread_mutex_unlock(&g_config_mutex)
#else
# define LOCK()   ((void)0)
# define UNLOCK() ((void)0)
#endif

// --- Helpers ---------------------------------------------------------------
static unsigned long hash_string(const char* str) {
    unsigned long hash = 5381;
    int c;
    while ((c = (unsigned char)*str++)) {
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    }
    return hash;
}

// safe_strdup: portable wrapper that checks for NULL and logs on failure.
static char* safe_strdup(const char* src) {
    if (!src) return NULL;
    size_t len = strlen(src) + 1;
    char* dst = (char*)malloc(len);
    if (!dst) {
        logger_log(LOG_LEVEL_FATAL, "CONFIG", "Memory allocation failed in safe_strdup (size=%zu).", len);
        return NULL;
    }
    memcpy(dst, src, len);
    return dst;
}

// trim_whitespace: returns pointer inside buffer where trimmed string starts,
// and modifies buffer in-place to terminate after the trimmed content.
static char* trim_whitespace(char* str) {
    if (!str) return NULL;
    char* end;
    // Trim leading
    while (isspace((unsigned char)*str)) str++;
    if (*str == '\0') return str;
    // Trim trailing
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    *(end + 1) = '\0';
    return str;
}

// find_node: internal utility to find a node and optionally its previous pointer.
static ConfigNode* find_node(const char* key, ConfigNode** out_prev) {
    unsigned long h = hash_string(key);
    unsigned int idx = (unsigned int)(h % HASH_TABLE_SIZE);
    ConfigNode* prev = NULL;
    ConfigNode* cur = g_config_table[idx];
    while (cur) {
        if (strcmp(cur->key, key) == 0) {
            if (out_prev) *out_prev = prev;
            return cur;
        }
        prev = cur;
        cur = cur->next;
    }
    if (out_prev) *out_prev = NULL;
    return NULL;
}

// --- Public API ------------------------------------------------------------

void config_cleanup(void) {
    LOCK();
    for (int i = 0; i < HASH_TABLE_SIZE; ++i) {
        ConfigNode* cur = g_config_table[i];
        while (cur) {
            ConfigNode* next = cur->next;
            free(cur->key);
            free(cur->value);
            free(cur);
            cur = next;
        }
        g_config_table[i] = NULL;
    }
    UNLOCK();
    logger_log(LOG_LEVEL_INFO, "CONFIG", "Configuration cleaned up.");
}

phgitStatus config_set_value(const char* key, const char* value) {
    if (!key) return phgit_ERROR_INVALID_ARGS;
    if (!value) return phgit_ERROR_INVALID_ARGS;

    LOCK();
    // Attempt to find existing node
    unsigned long h = hash_string(key);
    unsigned int idx = (unsigned int)(h % HASH_TABLE_SIZE);
    ConfigNode* cur = g_config_table[idx];
    while (cur) {
        if (strcmp(cur->key, key) == 0) {
            // Update existing value
            char* new_value = safe_strdup(value);
            if (!new_value) { UNLOCK(); return phgit_ERROR_GENERAL; }
            free(cur->value);
            cur->value = new_value;
            UNLOCK();
            return phgit_SUCCESS;
        }
        cur = cur->next;
    }

    // Create new node
    ConfigNode* node = (ConfigNode*)malloc(sizeof(ConfigNode));
    if (!node) {
        logger_log(LOG_LEVEL_FATAL, "CONFIG", "Allocation failed for new config node.");
        UNLOCK();
        return phgit_ERROR_GENERAL;
    }

    node->key = safe_strdup(key);
    node->value = safe_strdup(value);
    if (!node->key || !node->value) {
        logger_log(LOG_LEVEL_FATAL, "CONFIG", "Allocation failed for key/value when inserting '%s'.", key);
        free(node->key);
        free(node->value);
        free(node);
        UNLOCK();
        return phgit_ERROR_GENERAL;
    }

    // Prepend to bucket
    node->next = g_config_table[idx];
    g_config_table[idx] = node;
    UNLOCK();
    return phgit_SUCCESS;
}

// Returns a heap-allocated string that the caller must free. Returns NULL if
// key not found or on allocation failure.
char* config_get_value(const char* key) {
    if (!key) return NULL;
    LOCK();
    ConfigNode* node = find_node(key, NULL);
    if (!node) { UNLOCK(); return NULL; }
    char* copy = safe_strdup(node->value);
    if (!copy) {
        // safe_strdup logs the failure
        UNLOCK();
        return NULL;
    }
    UNLOCK();
    return copy;
}

// Returns a const pointer to the internal value. The pointer is valid until
// the key is removed or config_cleanup() is called. Caller MUST NOT modify
// or free the returned pointer. Use config_get_value() if you need an owned copy.
const char* config_get_ref(const char* key) {
    if (!key) return NULL;
    LOCK();
    ConfigNode* node = find_node(key, NULL);
    const char* result = node ? (const char*)node->value : NULL;
    UNLOCK();
    return result;
}

// Returns true if key exists (non-zero).
int config_has_key(const char* key) {
    if (!key) return 0;
    LOCK();
    ConfigNode* node = find_node(key, NULL);
    int exists = node != NULL;
    UNLOCK();
    return exists;
}

// Removes a key from the table. Returns phgit_SUCCESS if removed, or
// phgit_ERROR_NOT_FOUND if the key didn't exist.
phgitStatus config_remove(const char* key) {
    if (!key) return phgit_ERROR_INVALID_ARGS;
    LOCK();
    unsigned long h = hash_string(key);
    unsigned int idx = (unsigned int)(h % HASH_TABLE_SIZE);
    ConfigNode* prev = NULL;
    ConfigNode* cur = g_config_table[idx];
    while (cur) {
        if (strcmp(cur->key, key) == 0) {
            if (prev) prev->next = cur->next; else g_config_table[idx] = cur->next;
            free(cur->key);
            free(cur->value);
            free(cur);
            UNLOCK();
            return phgit_SUCCESS;
        }
        prev = cur;
        cur = cur->next;
    }
    UNLOCK();
    return phgit_ERROR_NOT_FOUND;
}

// Loads configuration from a file. Returns phgit_SUCCESS on success or
// an error code on failure (missing file is not fatal: returns phgit_SUCCESS but
// logs info). Lines must be in key=value format. Comments beginning with '#'
// are ignored.
phgitStatus config_load(const char* filename) {
    if (!filename) return phgit_ERROR_INVALID_ARGS;

    // Keep previous config cleared for deterministic behavior
    config_cleanup();

    FILE* f = fopen(filename, "r");
    if (!f) {
        logger_log(LOG_LEVEL_INFO, "CONFIG", "Config file '%s' not found; using defaults.", filename);
        return phgit_SUCCESS; // Not fatal
    }

    char buf[MAX_LINE_LENGTH];
    int line = 0;
    while (fgets(buf, sizeof(buf), f)) {
        line++;
        char* trimmed = trim_whitespace(buf);
        if (!trimmed || trimmed[0] == '\0' || trimmed[0] == '#') continue;
        char* sep = strchr(trimmed, '=');
        if (!sep) {
            logger_log_fmt(LOG_LEVEL_WARN, "CONFIG", "Malformed line %d in %s. Skipping.", line, filename);
            continue;
        }
        *sep = '\0';
        char* key = trim_whitespace(trimmed);
        char* value = trim_whitespace(sep + 1);
        if (key[0] == '\0') {
            logger_log_fmt(LOG_LEVEL_WARN, "CONFIG", "Empty key on line %d in %s. Skipping.", line, filename);
            continue;
        }
        if (config_set_value(key, value) != phgit_SUCCESS) {
            logger_log_fmt(LOG_LEVEL_WARN, "CONFIG", "Failed to insert key '%s' from line %d.", key, line);
        }
    }

    fclose(f);
    logger_log(LOG_LEVEL_INFO, "CONFIG", "Configuration loaded from '%s'.", filename);
    return phgit_SUCCESS;
}
