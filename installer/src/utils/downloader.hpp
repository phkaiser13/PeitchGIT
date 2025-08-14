/* Copyright (C) 2025 Pedro Henrique / phkaiser13
* File: downloader.hpp
* This header defines the interface for the Downloader utility class. This class provides
* a high-level, easy-to-use wrapper around a low-level HTTP client library (like libcurl)
* to handle file downloads. It includes essential features for a modern installer, such as
* progress tracking via callbacks, timeout handling, and support for resuming downloads.
* It also includes a utility function for checksum verification to ensure file integrity.
* SPDX-License-Identifier: Apache-2.0
*/

#pragma once

#include <string>
#include <functional>
#include <cstdint>

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
     * @brief A robust file downloader utility.
     */
    class Downloader {
    public:
        Downloader();
        ~Downloader();

        /**
         * @brief Downloads a file from a given URL to a specified output path.
         * @param url The HTTP/HTTPS URL of the file to download.
         * @param output_path The local filesystem path to save the file to.
         * @param callback An optional callback function to report progress.
         * @return True if the download was successful, false otherwise.
         */
        bool download_file(const std::string& url, const std::string& output_path, ProgressCallback callback = nullptr);

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

    /**
     * @namespace checksum
     * @brief Provides utility functions for file integrity verification.
     */
    namespace checksum {
        enum class Algorithm {
            SHA256
        };

        /**
         * @brief Verifies the checksum of a file against an expected hash.
         * @param file_path The path to the local file.
         * @param expected_hash The expected checksum hash as a hex string.
         * @param algo The hashing algorithm to use (currently only SHA256).
         * @return True if the file's hash matches the expected hash, false otherwise.
         * @note The implementation of this function will require a cryptography library
         *       like OpenSSL, Crypto++, or a smaller, single-header library.
         */
        bool verify_file(const std::string& file_path, const std::string& expected_hash, Algorithm algo = Algorithm::SHA256);
    }

} // namespace phgit_installer::utils
