/* Copyright (C) 2025 Pedro Henrique / phkaiser13
 * File: downloader.cpp
 * This file provides the implementation for the Downloader class. It contains
 * the logic for interacting with the libcurl library to perform file downloads.
 * The implementation handles setting up cURL options, managing file I/O,
 * and rendering a real-time progress bar to the console.
 *
 * SPDX-License-Identifier: Apache-2.0 */

#include "downloader.hpp"

#include <iostream>
#include <stdexcept> // For std::runtime_error
#include <curl/curl.h>

namespace ph {

// Static initialization for libcurl. Should be called once per program.
// We can use a simple RAII struct to manage this globally and safely.
struct CurlGlobalInitializer {
    CurlGlobalInitializer() {
        // Initialize libcurl globally for all features.
        CURLcode res = curl_global_init(CURL_GLOBAL_ALL);
        if (res != CURLE_OK) {
            throw std::runtime_error("Failed to initialize libcurl globally.");
        }
    }
    ~CurlGlobalInitializer() {
        // Cleanup libcurl resources.
        curl_global_cleanup();
    }
};

// A global instance of this struct ensures that curl_global_init/cleanup
// are called at the start and end of the program's lifetime.
static CurlGlobalInitializer curl_initializer;


Downloader::Downloader() {
    m_curlHandle = curl_easy_init();
    if (!m_curlHandle) {
        // If initialization fails, we cannot proceed.
        throw std::runtime_error("Failed to initialize cURL easy handle.");
    }
}

Downloader::~Downloader() {
    // The handle is always cleaned up when the Downloader object is destroyed.
    if (m_curlHandle) {
        curl_easy_cleanup(m_curlHandle);
    }
}

bool Downloader::downloadFile(const std::string& url, const std::string& outputPath, const std::string& displayName) {
    // Use a smart pointer for the file handle to ensure it's always closed.
    // The custom deleter `&fclose` is provided to `std::unique_ptr`.
    std::unique_ptr<FILE, decltype(&fclose)> file(fopen(outputPath.c_str(), "wb"), &fclose);

    if (!file) {
        std::cerr << "Error: Could not open output file for writing: " << outputPath << std::endl;
        return false;
    }

    ProgressData progressData = { displayName };

    // Configure the cURL handle for the transfer.
    curl_easy_setopt(m_curlHandle, CURLOPT_URL, url.c_str());
    curl_easy_setopt(m_curlHandle, CURLOPT_USERAGENT, "phgit-installer/1.0");
    curl_easy_setopt(m_curlHandle, CURLOPT_FOLLOWLOCATION, 1L); // Follow redirects.
    curl_easy_setopt(m_curlHandle, CURLOPT_FAILONERROR, 1L);    // Fail on HTTP status codes >= 400.

    // Configure callbacks.
    curl_easy_setopt(m_curlHandle, CURLOPT_WRITEFUNCTION, &Downloader::writeCallback);
    curl_easy_setopt(m_curlHandle, CURLOPT_WRITEDATA, file.get());

    curl_easy_setopt(m_curlHandle, CURLOPT_NOPROGRESS, 0L); // Enable progress meter.
    curl_easy_setopt(m_curlHandle, CURLOPT_XFERINFOFUNCTION, &Downloader::progressCallback);
    curl_easy_setopt(m_curlHandle, CURLOPT_XFERINFODATA, &progressData);

    // Execute the download.
    CURLcode res = curl_easy_perform(m_curlHandle);

    // Add a newline to move past the progress bar line.
    std::cout << std::endl;

    if (res != CURLE_OK) {
        std::cerr << "Error: Download failed: " << curl_easy_strerror(res) << std::endl;
        return false;
    }

    return true;
}

size_t Downloader::writeCallback(void* contents, size_t size, size_t nmemb, void* userdata) {
    // The userdata pointer is the FILE* handle we passed to CURLOPT_WRITEDATA.
    FILE* file = static_cast<FILE*>(userdata);
    // Write the received data chunk to the file.
    return fwrite(contents, size, nmemb, file);
}

int Downloader::progressCallback(void* clientp, double dltotal, double dlnow, double ultotal, double ulnow) {
    // The clientp pointer is the ProgressData struct we passed.
    ProgressData* data = static_cast<ProgressData*>(clientp);

    // Avoid division by zero if the total size is unknown.
    if (dltotal <= 0.0) {
        return 0;
    }

    double percentage = dlnow / dltotal * 100.0;
    int barWidth = 50;
    int pos = static_cast<int>(barWidth * percentage / 100.0);

    // Print the progress bar to the console.
    std::cout << "\rDownloading " << data->displayName << ": [";
    for (int i = 0; i < barWidth; ++i) {
        if (i < pos) std::cout << "=";
        else if (i == pos) std::cout << ">";
        else std::cout << " ";
    }
    std::cout << "] " << static_cast<int>(percentage) << " %" << std::flush;

    return 0; // A non-zero return would abort the transfer.
}

} // namespace ph