/* Copyright (C) 2025 Pedro Henrique / phkaiser13
* downloader.cpp
* This file provides the C++ implementation for the file download functionality,
* exposed to the C core via a C-compatible interface. It leverages the 'cpr'
* library, a modern C++ wrapper around libcurl, to handle HTTP requests in a
* robust and efficient manner. The implementation focuses on performance by
* streaming downloads directly to disk, provides detailed error reporting, and
* supports progress callbacks for an interactive user experience.
* SPDX-License-Identifier: Apache-2.0 */

#include "downloader.hpp"

// We will use 'cpr' (C++ Requests) for handling HTTP.
// It's a modern, clean wrapper around libcurl.
// The Makefile must be updated to link against cpr and curl.
// Example LDFLAGS: -lcpr -lcurl
#include <cpr/cpr.h>

#include <fstream>
#include <string>
#include <memory> // For std::unique_ptr

// Helper function to create a DownloadResult with an error message.
// This ensures consistent memory allocation and message formatting.
static DownloadResult make_error_result(DownloadStatusCode code, const std::string& message) {
    DownloadResult result;
    result.code = code;
    // Allocate memory for the C string and copy the message.
    // The C-side caller is responsible for freeing this.
    result.error_message = new char[message.length() + 1];
    std::strcpy(result.error_message, message.c_str());
    return result;
}

// Helper function to create a success result.
static DownloadResult make_success_result() {
    DownloadResult result;
    result.code = DOWNLOAD_SUCCESS;
    result.error_message = nullptr;
    return result;
}

// The C-linkage implementation of the download_file function.
extern "C" DownloadResult download_file(const char* url, const char* destination_path, const DownloadCallbacks* callbacks) {
    if (!url || !destination_path) {
        return make_error_result(DOWNLOAD_ERROR_INVALID_URL, "URL or destination path is null.");
    }

    // Open the output file stream in binary mode.
    // Using RAII (std::ofstream) ensures the file is closed on scope exit.
    std::ofstream ofs(destination_path, std::ios::binary);
    if (!ofs) {
        return make_error_result(DOWNLOAD_ERROR_FILESYSTEM, "Failed to open destination file for writing: " + std::string(destination_path));
    }

    cpr::Url cpr_url{url};

    // Prepare the progress callback if provided.
    // We use a lambda to capture the C-style callback and user_data.
    cpr::ProgressCallback progress_cb;
    if (callbacks && callbacks->on_progress) {
        progress_cb = cpr::ProgressCallback([callbacks](cpr::cpr_off_t total, cpr::cpr_off_t downloaded, cpr::cpr_off_t, cpr::cpr_off_t, intptr_t) -> bool {
            callbacks->on_progress(total, downloaded, callbacks->user_data);
            return true; // Continue download
        });
    }

    // Use a streaming download (WriteCallback) to avoid loading the entire file into memory.
    // This is highly efficient for large files.
    auto write_cb = cpr::WriteCallback([&ofs](std::string data, intptr_t) -> bool {
        ofs.write(data.c_str(), data.length());
        // Return true to continue the download, false to abort.
        // We check the stream state to ensure writes are succeeding.
        return ofs.good();
    });

    // Configure the session
    cpr::Session session;
    session.SetUrl(cpr_url);
    session.SetWriteCallback(write_cb);
    if (progress_cb) {
        session.SetProgressCallback(progress_cb);
    }
    // Follow redirects, a common case for GitHub releases.
    session.SetRedirect(true);
    // Set a reasonable timeout.
    session.SetTimeout(cpr::Timeout{300000}); // 300 seconds

    // Execute the GET request
    cpr::Response r = session.Get();

    // After the request, check the file stream for any write errors.
    if (!ofs.good()) {
        return make_error_result(DOWNLOAD_ERROR_FILESYSTEM, "An error occurred while writing to the destination file.");
    }

    // Check for network-level errors (e.g., DNS, connection refused).
    if (r.error.code != cpr::ErrorCode::OK) {
        return make_error_result(DOWNLOAD_ERROR_NETWORK, "Network error: " + r.error.message);
    }

    // Check for HTTP-level errors (e.g., 404 Not Found, 503 Service Unavailable).
    // cpr considers 4xx and 5xx status codes as successful requests at the transport layer,
    // so we must check the status code explicitly.
    if (r.status_code >= 400) {
        std::string error_msg = "HTTP error: " + std::to_string(r.status_code) + " " + r.reason;
        return make_error_result(DOWNLOAD_ERROR_HTTP, error_msg);
    }

    // If we've reached here, the download was successful.
    return make_success_result();
}