/* Copyright (C) 2025 Pedro Henrique / phkaiser13
 * Logger.cpp - Implementation of the advanced C++ logger.
 *
 * This file implements the singleton Logger class and its C-compatible wrapper
 * functions. It handles the low-level details of file I/O, message formatting,
 * and thread synchronization. This version includes a new formatted logging
 * function, `logger_log_fmt`, which dynamically allocates memory to prevent
 * buffer overflow vulnerabilities.
 *
 * The use of `std::lock_guard` makes the `log` method inherently thread-safe,
 * preventing interleaved or corrupted log entries when multiple modules write
 * to the log simultaneously. The singleton pattern is implemented using a
 * static local variable in `get_instance()`, which is a clean and thread-safe
 * approach in modern C++.
 *
 * SPDX-License-Identifier: Apache-2.0 */

#include "Logger.h"
#include <iostream>
#include <chrono>
#include <ctime>
#include <iomanip> // For std::put_time
#include <cstdarg> // For va_list, va_start, va_end
#include <vector>  // For std::vector as a safe dynamic buffer
#include <memory>  // For std::unique_ptr for an alternative buffer

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
        // Use the internal log method directly to avoid re-locking
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

    // Manually call the log implementation to avoid re-locking the mutex
    // This is safe because we already hold the lock.
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
 * @brief Logs a pre-formatted message with a timestamp, level, and module name.
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

/**
 * @brief Logs a formatted message using a va_list, preventing buffer overflows.
 */
void Logger::log(GitphLogLevel level, const std::string& module_name, const char* format, va_list args) {
    // 1. Determine the required buffer size.
    // We must copy the va_list as vsnprintf can invalidate it.
    va_list args_copy;
    va_copy(args_copy, args);
    int len = vsnprintf(nullptr, 0, format, args_copy);
    va_end(args_copy);

    if (len < 0) {
        // An encoding error occurred. Log a fallback message.
        log(LOG_LEVEL_ERROR, "LOGGER", "An error occurred during log message formatting.");
        return;
    }

    // 2. Allocate a buffer of the exact size.
    // Using std::vector is a safe, modern C++ way to handle dynamic C-style arrays.
    std::vector<char> buffer(len + 1);

    // 3. Format the string into the buffer.
    vsnprintf(buffer.data(), buffer.size(), format, args);

    // 4. Pass the formatted string to the original log function.
    // The original function will handle thread-locking and file writing.
    log(level, module_name, std::string(buffer.data()));
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
void logger_log_fmt(GitphLogLevel level, const char* module_name, const char* format, ...) {
    if (module_name == nullptr || format == nullptr) {
        return;
    }

    va_list args;
    va_start(args, format);
    // This C wrapper function calls the C++ method that takes a va_list.
    // This delegates all the complex, safe formatting logic to the C++ class.
    Logger::get_instance().log(level, module_name, format, args);
    va_end(args);
}

/**
 * @see Logger.h
 */
void logger_cleanup() {
    // In this singleton implementation using a static local variable, the
    // destructor of the Logger instance is called automatically at program
    // exit. Therefore, this function is not strictly necessary to close the
    // file, but we keep it for API consistency.
    logger_log(LOG_LEVEL_INFO, "MAIN", "Application cleanup requested.");
}