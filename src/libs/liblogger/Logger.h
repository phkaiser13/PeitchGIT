/* Copyright (C) 2025 Pedro Henrique
 * Logger.h - Advanced C++ logger with a C-compatible interface.
 *
 * This header defines a singleton Logger class designed for robust, thread-safe
 * logging across the entire application. It is implemented in C++ to leverage
 * features like object-oriented design, RAII for file management, and the
 * Standard Template Library (STL).
 *
 * Key Features:
 * - Singleton Pattern: Ensures a single, globally accessible logging instance.
 * - Thread Safety: Uses std::mutex to protect file writes from race
 *   conditions, which is critical in a polyglot application where different
 *   modules may run concurrently.
 * - C-style Wrapper: Exposes a simple C API (`logger_init`, `logger_log`,
 *   `logger_cleanup`) using `extern "C"`. This is the bridge that allows the
 *   C core and other non-C++ modules to use the logger seamlessly.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later */

#ifndef LOGGER_H
#define LOGGER_H

// We need the core API header to use the consistent GitphLogLevel enum.
// This demonstrates how even C++ modules adhere to the core contracts.
#include "../../ipc/include/gitph_core_api.h"

#ifdef __cplusplus

#include <string>
#include <fstream>
#include <mutex>
#include <memory> // For std::unique_ptr

// The C++ Logger class. This is not directly visible to C code.
class Logger {
public:
    /**
     * @brief Gets the single instance of the Logger.
     * @return A reference to the Logger instance.
     */
    static Logger& get_instance();

    /**
     * @brief Configures the logger, primarily by opening the log file.
     * @param filename The path to the log file.
     * @return true on success, false if the file could not be opened.
     */
    bool init(const std::string& filename);

    /**
     * @brief Writes a formatted message to the log file.
     * @param level The severity level of the message.
     * @param module_name The name of the module originating the log entry.
     * @param message The log message content.
     */
    void log(GitphLogLevel level, const std::string& module_name, const std::string& message);

    // Delete copy constructor and assignment operator to enforce singleton property.
    Logger(const Logger&) = delete;
    void operator=(const Logger&) = delete;

private:
    // Private constructor to prevent direct instantiation.
    Logger() = default;
    // Private destructor to be managed internally.
    ~Logger();

    std::ofstream m_log_file; // The output file stream.
    std::mutex m_mutex;       // Mutex to ensure thread-safe writes.
};

#endif // __cplusplus


/*
 * C-style wrapper interface.
 * This is the public-facing API for all C code (and other languages that
 * link against C symbols). These functions will internally call the methods
 * of the C++ Logger singleton.
 */
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initializes the global logging system.
 *
 * Must be called once at application startup. It sets up the log file that
 * will be used for the entire session.
 *
 * @param filename The path to the log file.
 * @return 0 on success, -1 on failure (e.g., cannot open file).
 */
int logger_init(const char* filename);

/**
 * @brief Logs a message through the global logger.
 *
 * This function is thread-safe.
 *
 * @param level The severity level of the message.
 * @param module_name The name of the calling module (e.g., "MAIN", "GIT_OPS").
 * @param message The message to be logged.
 */
void logger_log(GitphLogLevel level, const char* module_name, const char* message);

/**
 * @brief Cleans up the logging system.
 *
 * Must be called once at application shutdown to ensure the log file is
 * properly closed.
 */
void logger_cleanup();

#ifdef __cplusplus
} // extern "C"
#endif

#endif // LOGGER_H