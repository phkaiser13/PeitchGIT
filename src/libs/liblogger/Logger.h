/* Copyright (C) 2025 Pedro Henrique / phkaiser13
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
 *   `logger_cleanup`, and the new `logger_log_fmt`) using `extern "C"`. This
 *   is the bridge that allows the C core and other non-C++ modules to use
 *   the logger seamlessly.
 *
 * SPDX-License-Identifier: Apache-2.0 */

#ifndef LOGGER_H
#define LOGGER_H

// We need the core API header to use the consistent phgitLogLevel enum.
// This demonstrates how even C++ modules adhere to the core contracts.
#include "../../ipc/include/phgit_core_api.h"

#ifdef __cplusplus

#include <string>
#include <fstream>
#include <mutex>
#include <memory> // For std::unique_ptr
#include <cstdarg> // For va_list in the C++ implementation

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
     * @brief Writes a pre-formatted message to the log file.
     * @param level The severity level of the message.
     * @param module_name The name of the module originating the log entry.
     * @param message The log message content.
     */
    void log(phgitLogLevel level, const std::string& module_name, const std::string& message);

    /**
     * @brief Writes a message to the log file using a va_list.
     *        This is the core implementation for formatted logging.
     * @param level The severity level of the message.
     * @param module_name The name of the module originating the log entry.
     * @param format The printf-style format string.
     * @param args The va_list of arguments.
     */
    void log(phgitLogLevel level, const std::string& module_name, const char* format, va_list args);

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
 * @brief Logs a simple, pre-formatted message through the global logger.
 *
 * This function is thread-safe.
 *
 * @param level The severity level of the message.
 * @param module_name The name of the calling module (e.g., "MAIN", "GIT_OPS").
 * @param message The message to be logged.
 */
void logger_log(phgitLogLevel level, const char* module_name, const char* message);

/**
 * @brief Logs a formatted message safely, preventing buffer overflows.
 *
 * This function accepts a printf-style format string and a variable number of
 * arguments. It dynamically allocates the necessary memory for the final
 * log message, making it safe to use with inputs of unpredictable size, such
 * as file paths or network error messages.
 *
 * This function is thread-safe.
 *
 * @param level The severity level of the message.
 * @param module_name The name of the calling module.
 * @param format The printf-style format string.
 * @param ... The variable arguments corresponding to the format string.
 */
void logger_log_fmt(phgitLogLevel level, const char* module_name, const char* format, ...);

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