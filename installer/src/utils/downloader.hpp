/* Copyright (C) 2025 Pedro Henrique / phkaiser13
* File: downloader.hpp
* This header defines the interface for the Downloader utility class. This class provides
* a high-level, easy-to-use wrapper around a low-level HTTP client library (like libcurl)
* to handle downloads. It supports downloading to both files (with progress tracking) and
* directly to memory (for small payloads like API responses), promoting both efficiency
* and security by avoiding temporary files.
* SPDX-License-Identifier: Apache-2.0
*/

#pragma once

#include <string>
#include <functional>
#include <cstdint>
#include <memory>
#include <optional> // Required for std::optional

namespace phgit_installer::utils {

    /**
     * @brief Defines the signature for the progress callback function.
     * This allows the caller to receive real-time updates on download progress.
     * @param total_bytes The total size of the file being downloaded.
     * @param downloaded_bytes The number of bytes downloaded so far.
     */
    using ProgressCallback = std::function<void(uint64_t total_bytes, uint64_t downloaded_bytes)>;

    /**
     * @class Downloader
     * @brief A robust utility for downloading content over HTTP/S.
     *
     * This class encapsulates the complexity of an HTTP client library (libcurl)
     * behind a simple, modern C++ interface. It uses the PIMPL idiom to hide
     * implementation details, which improves compilation times and isolates dependencies.
     */
    class Downloader {
    public:
        /**
         * @brief Constructs a Downloader instance.
         * Initializes the underlying HTTP client resources.
         */
        Downloader();

        /**
         * @brief Destroys the Downloader instance.
         * Cleans up all underlying HTTP client resources.
         */
        ~Downloader();

        // Rule of Five: Explicitly default or delete special member functions
        // for clarity and to prevent unintended copies.
        Downloader(const Downloader&) = delete;
        Downloader& operator=(const Downloader&) = delete;
        Downloader(Downloader&&) = default;
        Downloader& operator=(Downloader&&) = default;

        /**
         * @brief Downloads a file from a given URL to a specified output path.
         * @param url The HTTP/HTTPS URL of the file to download.
         * @param output_path The local filesystem path to save the file to.
         * @param callback An optional callback function to report progress.
         * @return True if the download was successful, false otherwise.
         */
        bool download_file(const std::string& url, const std::string& output_path, ProgressCallback callback = nullptr);

        /**
         * @brief Downloads content from a URL directly into a string in memory.
         * This is highly efficient for small payloads like JSON API responses, as it
         * avoids disk I/O and the security risks of temporary files.
         * @param url The HTTP/HTTPS URL of the content to download.
         * @return An std::optional<std::string> containing the data on success, or std::nullopt on failure.
         */
        std::optional<std::string> download_to_string(const std::string& url);

        /**
         * @brief Sets the connection and transfer timeout in seconds.
         * @param seconds The timeout duration. A value of 0 means no timeout.
         */
        void set_timeout(long seconds);

        /**
         * @brief Sets the proxy server URL to use for the connection.
         * @param proxy_url The URL of the proxy (e.g., "http://proxy.example.com:8080").
         */
        void set_proxy(const std::string& proxy_url);

        /**
         * @brief Sets a custom User-Agent string for HTTP requests.
         * @param user_agent The User-Agent string.
         */
        void set_user_agent(const std::string& user_agent);

    private:
        // PIMPL (Pointer to Implementation) idiom to hide libcurl details from the header.
        // This avoids including <curl/curl.h> here, which speeds up compilation and
        // keeps the interface clean.
        class DownloaderImpl;
        std::unique_ptr<DownloaderImpl> m_impl;
    };

} // namespace phgit_installer::utils