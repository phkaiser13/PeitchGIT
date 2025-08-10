/* Copyright (C) 2025 Pedro Henrique / phkaiser13
 * download.c - A robust file downloader using libcurl.
 *
 * This file provides a focused and reliable file download functionality,
 * essential for our installer to fetch dependencies like the Git installer.
 * It is built upon the industry-standard libcurl library, abstracting away
 * the complexities of network protocols like HTTPS and redirects.
 *
 * The implementation uses libcurl's callback mechanism for writing data to a
 * file and for displaying a real-time progress bar, providing a professional
 * and user-friendly experience during potentially long operations.
 *
 * SPDX-License-Identifier: Apache-2.0 */

#include <stdio.h>
#include <curl/curl.h> // Assumes libcurl development headers are available

// A struct to hold progress data for the progress callback.
struct ProgressData {
    const char* filename;
};

/**
 * @brief A libcurl callback function to write received data into a file.
 *
 * This function is called by libcurl whenever it receives a new chunk of data
 * from the network. Our implementation simply writes this chunk to the file
 * stream provided via the `userdata` pointer.
 *
 * @param contents Pointer to the received data.
 * @param size Size of each data item (always 1 for us).
 * @param nmemb Number of data items.
 * @param userdata A void pointer to our user-defined data (in this case, a FILE*).
 * @return The number of bytes successfully written. If this differs from the
 *         number of bytes received, libcurl will abort the transfer.
 */
static size_t write_data(void* contents, size_t size, size_t nmemb, void* userdata) {
    FILE* fp = (FILE*)userdata;
    return fwrite(contents, size, nmemb, fp);
}

/**
 * @brief A libcurl callback function to display a progress bar.
 *
 * This function is called by libcurl periodically during the transfer. It
 * provides the total download size and the amount downloaded so far, which we
 * use to calculate and render a simple text-based progress bar.
 *
 * @param clientp A void pointer to our user-defined data (ProgressData struct).
 * @param dltotal Total size of the download in bytes.
 * @param dlnow Number of bytes downloaded so far.
 * @param ultotal Total upload size (not used by us).
 * @param ulnow Bytes uploaded so far (not used by us).
 * @return 0 to continue the transfer, non-zero to abort.
 */
static int progress_callback(void* clientp,
                             curl_off_t dltotal,
                             curl_off_t dlnow,
                             curl_off_t ultotal,
                             curl_off_t ulnow) {
    struct ProgressData* data = (struct ProgressData*)clientp;
    if (dltotal > 0) {
        double percentage = (double)dlnow / (double)dltotal * 100.0;
        int bar_width = 50;
        int pos = bar_width * percentage / 100.0;

        // Print the progress bar to the console.
        // The '\r' character moves the cursor to the beginning of the line,
        // allowing us to overwrite the previous progress bar for an animation.
        printf("\rDownloading %s: [", data->filename);
        for (int i = 0; i < bar_width; ++i) {
            if (i < pos) printf("=");
            else if (i == pos) printf(">");
            else printf(" ");
        }
        printf("] %.0f %%", percentage);
        fflush(stdout); // Ensure the output is displayed immediately.
    }
    return 0;
}

/**
 * @brief Downloads a file from a given URL to a specified destination path.
 *
 * This is the main public function of this module. It initializes libcurl,
 * sets up all necessary options (URL, callbacks, error handling), performs
 * the download, and cleans up resources.
 *
 * @param url The URL of the file to download.
 * @param outpath The local file path where the downloaded file will be saved.
 * @return 0 on success, -1 on failure.
 */
int download_file(const char* url, const char* outpath) {
    CURL* curl_handle;
    FILE* pagefile;
    CURLcode res;
    int ret_code = 0;

    // Initialize libcurl globally.
    curl_global_init(CURL_GLOBAL_ALL);

    // Get a curl handle.
    curl_handle = curl_easy_init();
    if (!curl_handle) {
        fprintf(stderr, "Error: Failed to initialize curl handle.\n");
        curl_global_cleanup();
        return -1;
    }

    // Open the output file for writing in binary mode.
    pagefile = fopen(outpath, "wb");
    if (!pagefile) {
        fprintf(stderr, "Error: Cannot open output file %s\n", outpath);
        curl_easy_cleanup(curl_handle);
        curl_global_cleanup();
        return -1;
    }

    struct ProgressData progress_data = { .filename = outpath };

    // --- Set libcurl options ---
    curl_easy_setopt(curl_handle, CURLOPT_URL, url);
    curl_easy_setopt(curl_handle, CURLOPT_VERBOSE, 0L); // Set to 1L for debug info
    curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, 0L); // Enable progress meter
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_data);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void*)pagefile);
    curl_easy_setopt(curl_handle, CURLOPT_XFERINFOFUNCTION, progress_callback);
    curl_easy_setopt(curl_handle, CURLOPT_XFERINFODATA, (void*)&progress_data);
    curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1L); // Follow redirects

    // Perform the file transfer.
    printf("Preparing to download from %s\n", url);
    res = curl_easy_perform(curl_handle);
    printf("\n"); // Newline after the progress bar is complete.

    if (res != CURLE_OK) {
        fprintf(stderr, "Error: curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        ret_code = -1;
    } else {
        printf("Download of %s completed successfully.\n", outpath);
    }

    // Cleanup.
    fclose(pagefile);
    curl_easy_cleanup(curl_handle);
    curl_global_cleanup();

    return ret_code;
}