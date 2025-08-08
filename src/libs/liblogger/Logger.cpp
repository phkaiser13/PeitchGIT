/* Copyright (C) 2025 Pedro Henrique
 * Logger.cpp - Implementation of the advanced C++ logger.
 *
 * This file implements the singleton Logger class and its C-compatible wrapper
 * functions. It handles the low-level details of file I/O, message formatting,
 * and thread synchronization.
 *
 * The use of `std::lock_guard` makes the `log` method inherently thread-safe,
 * preventing interleaved or corrupted log entries when multiple modules write
 * to the log simultaneously. The singleton pattern is implemented using a
 * static local variable in `get_instance()`, which is a clean and thread-safe
 * approach in modern C++.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "Logger.h"
#include <iostream>
#include <chrono>
#include <ctime>
#include <iomanip> // For std::put_time

// --- C++ Class Implementation ---

/**
 * @brief Provides the singleton instance.
 *
 * The `static` keyword ensures the instance is created only once, the first
 * time this function is called. This is the Meyers' Singleton pattern.
 */
Logger& Logger::get_instance() {
    static Logger instance;
    return instance;
}

/**
 * @brief Destructor closes the file stream automatically due to RAII.
 */
Logger::~Logger() {
    if (m_log_file.is_open()) {
        log(LOG_LEVEL_INFO, "LOGGER", "Logging system shutting down.");
        m_log_file.close();
    }
}

/**
 * @brief Initializes the logger by opening the specified file.
 */
bool Logger::init(const std::string& filename) {
    // The lock ensures that even initialization is a thread-safe operation.
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_log_file.is_open()) {
        return true; // Already initialized
    }

    m_log_file.open(filename, std::ios::out | std::ios::app);
    if (!m_log_file.is_open()) {
        // Use std::cerr for errors that happen before the logger is ready.
        std::cerr << "FATAL: Could not open log file: " << filename << std::endl;
        return false;
    }

    log(LOG_LEVEL_INFO, "LOGGER", "Logging system initialized.");
    return true;
}

/**
 * @brief Converts GitphLogLevel enum to a human-readable string.
 */
static const char* level_to_string(GitphLogLevel level) {
    switch (level) {
        case LOG_LEVEL_DEBUG: return "DEBUG";
        case LOG_LEVEL_INFO:  return "INFO ";
        case LOG_LEVEL_WARN:  return "WARN ";
        case LOG_LEVEL_ERROR: return "ERROR";
        case LOG_LEVEL_FATAL: return "FATAL";
        default:              return "UNKWN";
    }
}

/**
 * @brief Logs a message with a timestamp, level, and module name.
 */
void Logger::log(GitphLogLevel level, const std::string& module_name, const std::string& message) {
    // Acquire the lock. It will be automatically released when `lock` goes
    // out of scope at the end of the function.
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_log_file.is_open()) {
        // Log to stderr if the file isn't available for some reason.
        std::cerr << "LOGGER NOT INITIALIZED: [" << module_name << "] " << message << std::endl;
        return;
    }

    // Get current time
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);

    // Format the timestamp (e.g., "YYYY-MM-DD HH:MM:SS")
    // Note: std::put_time requires a non-const tm struct.
    struct tm timeinfo;
#ifdef _WIN32
    localtime_s(&timeinfo, &time_t_now); // Windows-specific safe version
#else
    localtime_r(&time_t_now, &timeinfo); // POSIX-specific safe version
#endif

    // Write the complete, formatted log entry to the file.
    m_log_file << "[" << std::put_time(&timeinfo, "%Y-%m-%d %H:%M:%S") << "] "
               << "[" << level_to_string(level) << "] "
               << "[" << module_name << "] "
               << message << std::endl;
}


// --- C Wrapper Implementation ---

/**
 * @see Logger.h
 */
int logger_init(const char* filename) {
    // The C function calls the C++ singleton's method.
    if (Logger::get_instance().init(filename)) {
        return 0; // Success
    }
    return -1; // Failure
}

/**
 * @see Logger.h
 */
void logger_log(GitphLogLevel level, const char* module_name, const char* message) {
    // Check for null pointers to prevent crashes.
    if (module_name == nullptr || message == nullptr) {
        return;
    }
    // The C function calls the C++ singleton's method.
    Logger::get_instance().log(level, module_name, message);
}

/**
 * @see Logger.h
 */
void logger_cleanup() {
    // In this singleton implementation using a static local variable, the
    // destructor of the Logger instance is called automatically at program
    // exit. Therefore, this function is not strictly necessary to close the
    // file, but we keep it for API consistency and to allow for potential
    // future cleanup logic that might be needed.
    // For now, it can be a no-op or log a final message.
    logger_log(LOG_LEVEL_INFO, "MAIN", "Cleanup function called.");
}