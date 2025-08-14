/* Copyright (C) 2025 Pedro Henrique / phkaiser13
* File: downloader.cpp
* This file implements the Downloader utility class using libcurl for robust HTTP/S
* operations. It employs the PIMPL idiom to hide all libcurl implementation details,
* ensuring a clean separation of concerns. It provides static callback functions required
* by libcurl's C API, which act as bridges to modern C++ features like std::function for
* progress reporting. The checksum verification function is provided as a stub, pending
* the integration of a dedicated cryptography library.
* SPDX-License-Identifier: Apache-2.0
*/

#include "utils/downloader.hpp"
#include "spdlog/spdlog.h"

#include <curl/curl.h>
#include <memory>
#include <cstdio>
#include <fstream>

namespace phgit_installer::utils {

    // --- PIMPL Implementation ---
    // This class holds all the libcurl-specific implementation details.
    class Downloader::DownloaderImpl {
    public:
        CURL* curl_handle;

        DownloaderImpl() {
            // curl_global_init should be called once per program.
            // We can wrap this in a static struct to ensure it.
            struct CurlGlobalInitializer {
                CurlGlobalInitializer() { curl_global_init(CURL_GLOBAL_ALL); }
                ~CurlGlobalInitializer() { curl_global_cleanup(); }
            };
            static CurlGlobalInitializer initializer;

            curl_handle = curl_easy_init();
        }

        ~DownloaderImpl() {
            if (curl_handle) {
                curl_easy_cleanup(curl_handle);
            }
        }
    };

    // --- libcurl C-style Callbacks ---
    // This function is called by libcurl whenever it receives data.
    static size_t write_data_callback(void* ptr, size_t size, size_t nmemb, FILE* stream) {
        return fwrite(ptr, size, nmemb, stream);
    }

    // This function is called by libcurl to report download progress.
    static int progress_callback(void* clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow) {
        auto* callback_fn = static_cast<ProgressCallback*>(clientp);
        if (callback_fn && *callback_fn) {
            (*callback_fn)(static_cast<uint64_t>(dltotal), static_cast<uint64_t>(dlnow));
        }
        return 0; // Return 0 to continue the transfer.
    }

    // --- Downloader Class Implementation ---
    Downloader::Downloader() : m_impl(std::make_unique<DownloaderImpl>()) {}

    Downloader::~Downloader() = default; // Default destructor is fine due to unique_ptr.

    bool Downloader::download_file(const std::string& url, const std::string& output_path, ProgressCallback callback) {
        if (!m_impl || !m_impl->curl_handle) {
            spdlog::error("Downloader not initialized correctly.");
            return false;
        }

        CURL* curl = m_impl->curl_handle;
        
        // Open the output file for writing in binary mode.
        FILE* fp = fopen(output_path.c_str(), "wb");
        if (!fp) {
            spdlog::error("Failed to open output file: {}", output_path);
            return false;
        }

        // Set libcurl options
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L); // Follow redirects
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);

        // Setup progress callback if one was provided
        if (callback) {
            curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
            curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, progress_callback);
            curl_easy_setopt(curl, CURLOPT_XFERINFODATA, &callback);
        } else {
            curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
        }

        // Execute the download
        CURLcode res = curl_easy_perform(curl);

        // Cleanup
        fclose(fp);

        if (res != CURLE_OK) {
            spdlog::error("Download failed for URL '{}': {}", url, curl_easy_strerror(res));
            // Remove the partial file on failure
            remove(output_path.c_str());
            return false;
        }

        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        if (http_code >= 400) {
            spdlog::error("Download failed for URL '{}': Server responded with HTTP code {}", url, http_code);
            remove(output_path.c_str());
            return false;
        }

        spdlog::info("Successfully downloaded file from '{}' to '{}'", url, output_path);
        return true;
    }

    void Downloader::set_timeout(long seconds) {
        if (m_impl && m_impl->curl_handle) {
            curl_easy_setopt(m_impl->curl_handle, CURLOPT_TIMEOUT, seconds);
        }
    }

    void Downloader::set_proxy(const std::string& proxy_url) {
        if (m_impl && m_impl->curl_handle) {
            curl_easy_setopt(m_impl->curl_handle, CURLOPT_PROXY, proxy_url.c_str());
        }
    }

    void Downloader::set_user_agent(const std::string& user_agent) {
        if (m_impl && m_impl->curl_handle) {
            curl_easy_setopt(m_impl->curl_handle, CURLOPT_USERAGENT, user_agent.c_str());
        }
    }

    // --- Checksum Namespace Implementation ---
    namespace checksum {
        bool verify_file(const std::string& file_path, const std::string& expected_hash, Algorithm algo) {
            // STUB IMPLEMENTATION
            // A real implementation requires a cryptography library (e.g., OpenSSL, Crypto++, etc.)
            // The logic would be:
            // 1. Initialize a SHA256 context.
            // 2. Open the file in binary mode.
            // 3. Read the file in chunks.
            // 4. For each chunk, update the SHA256 context.
            // 5. Finalize the hash to get the digest.
            // 6. Convert the binary digest to a hex string.
            // 7. Compare the hex string with `expected_hash`.

            spdlog::warn("Checksum verification for '{}' is currently a STUB. It will always return TRUE.", file_path);
            spdlog::warn("A cryptography library must be integrated for this feature to work.");
            
            if (!std::filesystem::exists(file_path)) {
                spdlog::error("File not found for checksum verification: {}", file_path);
                return false;
            }

            // To make the stub slightly more useful, we check against an empty hash.
            if (expected_hash.empty()) {
                spdlog::info("No expected hash provided for '{}'. Skipping verification.", file_path);
                return true;
            }

            return true; // Always succeed for now.
        }
    }

} // namespace phgit_installer::utils
