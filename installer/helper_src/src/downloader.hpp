/* Copyright (C) 2025 Pedro Henrique / phkaiser13
 * File: downloader.hpp
 * This header file defines the interface for the Downloader class. The class
 * is a modern C++ wrapper around the libcurl C library, designed to provide a
 * clean, safe, and high-performance file downloading utility. It handles all
 * the low-level details of a cURL transfer, including session management,
 * progress reporting, and writing data to a file, exposing a simple public API.
 *
 * SPDX-License-Identifier: Apache-2.0 */

#pragma once // Ensures this header is included only once per compilation unit.

#include <string>
#include <memory> // For std::unique_ptr
#include <cstdio> // For FILE*

// Forward-declare the CURL handle type from libcurl to avoid including the
// full curl.h header in our public interface. This is a good practice to
// reduce compile times and coupling.
using CURL = void;

namespace ph {

/**
 * @class Downloader
 * @brief Manages high-performance file downloads using libcurl.
 *
 * This class encapsulates a cURL "easy" handle, providing a resource-safe
 * (RAII) and easy-to-use interface for downloading a file from a URL to a
 * local path. It includes a visual progress bar.
 */
class Downloader {
public:
    /**
     * @brief Constructor. Initializes the cURL easy handle.
     *
     * Throws a std::runtime_error if the handle cannot be initialized.
     */
    Downloader();

    /**
     * @brief Destructor. Cleans up the cURL easy handle automatically.
     */
    ~Downloader();

    // Disable copy and move semantics to prevent misuse of the cURL handle.
    // The handle is a unique resource managed by this class instance.
    Downloader(const Downloader&) = delete;
    Downloader& operator=(const Downloader&) = delete;
    Downloader(Downloader&&) = delete;
    Downloader& operator=(Downloader&&) = delete;

    /**
     * @brief Downloads a file from a given URL to a specified destination path.
     *
     * @param url The URL of the file to download.
     * @param outputPath The local file path to save the downloaded content.
     * @param displayName A user-friendly name for the download, shown in the progress bar.
     * @return true on success, false on failure.
     */
    bool downloadFile(const std::string& url, const std::string& outputPath, const std::string& displayName);

private:
    // A struct to hold data for the progress callback.
    // Using a nested struct keeps it tightly coupled to the Downloader class.
    struct ProgressData {
        std::string displayName;
    };

    // Static callback functions required by libcurl's C API.
    // They are static because C libraries cannot call non-static C++ member functions.
    // We pass a pointer to 'this' or other data via the 'userdata' parameter.

    /**
     * @brief cURL callback for writing received data to a file.
     */
    static size_t writeCallback(void* contents, size_t size, size_t nmemb, void* userdata);

    /**
     * @brief cURL callback for displaying the progress bar.
     */
    static int progressCallback(void* clientp, double dltotal, double dlnow, double ultotal, double ulnow);

    // The unique cURL easy handle for this downloader instance.
    // Using std::unique_ptr with a custom deleter would be an alternative,
    // but managing it in the constructor/destructor is clear and sufficient here.
    CURL* m_curlHandle;
};

} // namespace ph