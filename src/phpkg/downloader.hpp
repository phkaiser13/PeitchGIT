/* Copyright (C) 2025 Pedro Henrique / phkaiser13
* downloader.hpp
* This header file defines the C-compatible interface for the C++ download
* subsystem. It is designed to be a clean boundary between the C module logic
* and the C++ implementation details of handling HTTP requests. The interface
* provides a robust, callback-based mechanism for downloading files, allowing
* the calling C code to receive progress updates and detailed error messages
* without being exposed to C++ complexities like exceptions or STL containers.
* SPDX-License-Identifier: Apache-2.0 */

#ifndef PHPKG_DOWNLOADER_HPP
#define PHPKG_DOWNLOADER_HPP

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h> // For size_t

/**
 * @enum DownloadStatusCode
 * @brief Defines status codes for the download operation.
 */
typedef enum {
    DOWNLOAD_SUCCESS = 0,
    DOWNLOAD_ERROR_GENERIC = 1,
    DOWNLOAD_ERROR_HTTP = 2,        // Indicates an HTTP error (e.g., 404, 500)
    DOWNLOAD_ERROR_NETWORK = 3,     // Indicates a network-level error (e.g., DNS failure)
    DOWNLOAD_ERROR_FILESYSTEM = 4,  // Indicates an error writing to the destination file
    DOWNLOAD_ERROR_INVALID_URL = 5
} DownloadStatusCode;

/**
 * @struct DownloadResult
 * @brief Holds the result of a download operation.
 *
 * This struct provides both a status code for programmatic checks and a
 * human-readable error message for logging or user feedback.
 */
typedef struct {
    DownloadStatusCode code;
    // A dynamically allocated string with details on the error.
    // It is the responsibility of the caller to free this memory.
    // Will be NULL on success.
    char* error_message;
} DownloadResult;

/**
 * @typedef download_progress_callback_t
 * @brief A function pointer type for download progress callbacks.
 *
 * @param total_bytes The total size of the file being downloaded. May be -1 if unknown.
 * @param downloaded_bytes The number of bytes downloaded so far.
 * @param user_data A pointer to user-defined data, passed through from the download_file call.
 */
typedef void (*download_progress_callback_t)(long long total_bytes, long long downloaded_bytes, void* user_data);

/**
 * @struct DownloadCallbacks
 * @brief A structure to hold optional callback functions for the downloader.
 */
typedef struct {
    download_progress_callback_t on_progress;
    void* user_data; // Opaque pointer passed to the callback
} DownloadCallbacks;

/**
 * @brief Downloads a file from a given URL to a specified destination path.
 *
 * This function orchestrates the entire download process, handling the HTTP
 * request, writing the data to a file, and providing progress updates via
 * optional callbacks.
 *
 * @param url The fully-qualified URL of the file to download.
 * @param destination_path The local filesystem path where the file will be saved.
 * @param callbacks An optional pointer to a DownloadCallbacks struct. If NULL, no progress is reported.
 * @return A DownloadResult struct. The caller MUST check the status code and
 *         free the 'error_message' field if it is not NULL.
 */
DownloadResult download_file(const char* url, const char* destination_path, const DownloadCallbacks* callbacks);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // PHPKG_DOWNLOADER_HPP