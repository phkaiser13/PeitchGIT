/* Copyright (C) 2025 Pedro Henrique / phkaiser13
* File: downloader.cpp
* This file implements the Downloader utility class using libcurl for robust HTTP/S
* operations. It employs the PIMPL idiom to hide all libcurl implementation details,
* ensuring a clean separation of concerns. It provides static callback functions required
* by libcurl's C API, which act as bridges to modern C++ features like std::function for
* progress reporting and writing data directly to a std::string.
* SPDX-License-Identifier: Apache-2.0
*/

#include "utils/downloader.hpp"
#include "spdlog/spdlog.h"

#include <curl/curl.h>
#include <memory>
#include <cstdio>
#include <utility> // For std::move

namespace phgit_installer::utils {

    // --- PIMPL Implementation ---
    // This class holds all the libcurl-specific implementation details.
    // This pattern decouples the interface from the implementation, reducing compile times
    // and hiding third-party library includes from the public header.
    class Downloader::DownloaderImpl {
    public:
        CURL* curl_handle;

        DownloaderImpl() {
            // curl_global_init should be called once per program.
            // We wrap this in a static struct to ensure it's called only once
            // in a thread-safe manner (since C++11, static local variable
            // initialization is thread-safe).
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

    /**
     * @brief libcurl callback to write received data into a FILE stream.
     * This is a C-style function required by the libcurl API. It acts as a bridge
     * between the C library and C++ file handling.
     * @param ptr Pointer to the received data chunk.
     * @param size Size of each data item.
     * @param nmemb Number of data items.
     * @param stream A FILE pointer where data will be written.
     * @return The total number of bytes successfully written.
     */
    static size_t write_to_file_callback(void* ptr, size_t size, size_t nmemb, FILE* stream) {
        return fwrite(ptr, size, nmemb, stream);
    }

    /**
     * @brief libcurl callback to write received data directly into a std::string.
     * This is a C-style function required by the libcurl API.
     * @param contents Pointer to the received data chunk.
     * @param size Size of each data item.
     * @param nmemb Number of data items.
     * @param userp A void pointer to a std::string object where data will be appended.
     * @return The total number of bytes processed (size * nmemb).
     */
    static size_t write_to_string_callback(void* contents, size_t size, size_t nmemb, void* userp) {
        // Calculate the real size of the incoming data chunk.
        size_t realsize = size * nmemb;
        // Cast the user pointer back to a std::string pointer and append the data.
        static_cast<std::string*>(userp)->append(static_cast<char*>(contents), realsize);
        // Return the number of bytes processed to signal success to libcurl.
        return realsize;
    }

    /**
     * @brief libcurl callback to report download progress.
     * This is a C-style function required by the libcurl API. It bridges the C callback
     * mechanism to a modern C++ std::function.
     * @param clientp A void pointer to our ProgressCallback std::function.
     * @param dltotal Total expected download size.
     * @param dlnow Bytes downloaded so far.
     * @param ultotal Total expected upload size (unused).
     * @param ulnow Bytes uploaded so far (unused).
     * @return Must return 0 to continue the transfer. A non-zero value would abort it.
     */
    static int progress_callback(void* clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow) {
        auto* callback_fn = static_cast<ProgressCallback*>(clientp);
        if (callback_fn && *callback_fn) {
            // Invoke the std::function with the progress data.
            (*callback_fn)(static_cast<uint64_t>(dltotal), static_cast<uint64_t>(dlnow));
        }
        return 0; // Return 0 to continue the transfer.
    }

    // --- Downloader Class Implementation ---
    Downloader::Downloader() : m_impl(std::make_unique<DownloaderImpl>()) {}

    Downloader::~Downloader() = default; // Default destructor is sufficient due to unique_ptr managing the PIMPL resource.

    bool Downloader::download_file(const std::string& url, const std::string& output_path, ProgressCallback callback) {
        if (!m_impl || !m_impl->curl_handle) {
            spdlog::error("Downloader not initialized correctly.");
            return false;
        }

        CURL* curl = m_impl->curl_handle;
        
        // Use a std::unique_ptr for the FILE pointer to ensure it's closed automatically
        // via RAII, even if exceptions occur or the function returns early.
        std::unique_ptr<FILE, decltype(&fclose)> fp(fopen(output_path.c_str(), "wb"), &fclose);
        if (!fp) {
            spdlog::error("Failed to open output file: {}", output_path);
            return false;
        }

        // Set libcurl options for file download
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L); // Follow HTTP redirects
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_to_file_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp.get());

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

        // fp is closed automatically by unique_ptr's destructor here.

        if (res != CURLE_OK) {
            spdlog::error("Download failed for URL '{}': {}", url, curl_easy_strerror(res));
            remove(output_path.c_str()); // Clean up the partial file on failure
            return false;
        }

        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        if (http_code >= 400) {
            spdlog::error("Download failed for URL '{}': Server responded with HTTP code {}", url, http_code);
            remove(output_path.c_str()); // Clean up the partial file on server error
            return false;
        }

        spdlog::info("Successfully downloaded file from '{}' to '{}'", url, output_path);
        return true;
    }

    std::optional<std::string> Downloader::download_to_string(const std::string& url) {
        if (!m_impl || !m_impl->curl_handle) {
            spdlog::error("Downloader not initialized correctly.");
            return std::nullopt;
        }

        CURL* curl = m_impl->curl_handle;
        std::string read_buffer;

        // Set libcurl options for in-memory download
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_to_string_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &read_buffer);
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L); // Disable progress meter for string downloads

        // Execute the download
        CURLcode res = curl_easy_perform(curl);

        if (res != CURLE_OK) {
            spdlog::error("In-memory download failed for URL '{}': {}", url, curl_easy_strerror(res));
            return std::nullopt;
        }

        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        if (http_code >= 400) {
            spdlog::error("In-memory download failed for URL '{}': Server responded with HTTP code {}", url, http_code);
            return std::nullopt;
        }

        spdlog::debug("Successfully downloaded content from '{}' to memory ({} bytes)", url, read_buffer.length());
        return read_buffer;
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

} // namespace phgit_installer::utils