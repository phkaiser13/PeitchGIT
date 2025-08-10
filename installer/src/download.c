/* Copyright (C) 2025 Pedro Henrique / phkaiser13
 * download.c - A robust file downloader using libcurl.
 *
 * This module provides reliable file download functionality for internal
 * application features, such as the self-updater. It uses libcurl's
 * callback mechanism for writing data and displaying a real-time progress bar.
 *
 * SPDX-License-Identifier: Apache-2.0 */

#include <stdio.h>
#include <curl/curl.h>

// Struct de progresso aprimorada para um nome de exibição customizado.
struct ProgressData {
    const char* display_name; // Nome amigável para a UI (ex: "gitph v1.1.0")
};

// A função write_data permanece exatamente a mesma.
static size_t write_data(void* contents, size_t size, size_t nmemb, void* userdata) {
    FILE* fp = (FILE*)userdata;
    return fwrite(contents, size, nmemb, fp);
}

// A função de progresso agora usa o display_name.
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

        // **MUDANÇA CHAVE AQUI**
        printf("\rDownloading %s: [", data->display_name);
        for (int i = 0; i < bar_width; ++i) {
            if (i < pos) printf("=");
            else if (i == pos) printf(">");
            else printf(" ");
        }
        printf("] %.0f %%", percentage);
        fflush(stdout);
    }
    return 0;
}

/**
 * @brief Downloads a file from a given URL to a specified destination path.
 *
 * @param url The URL of the file to download.
 * @param outpath The local file path where the downloaded file will be saved.
 * @param display_name A user-friendly name for the download, shown in the progress bar.
 * @return 0 on success, -1 on failure.
 */
int download_file(const char* url, const char* outpath, const char* display_name) {
    CURL* curl_handle;
    FILE* pagefile;
    CURLcode res;
    int ret_code = 0;

    curl_global_init(CURL_GLOBAL_ALL);
    curl_handle = curl_easy_init();
    if (!curl_handle) {
        fprintf(stderr, "Error: Failed to initialize curl handle.\n");
        return -1;
    }

    pagefile = fopen(outpath, "wb");
    if (!pagefile) {
        fprintf(stderr, "Error: Cannot open output file %s\n", outpath);
        curl_easy_cleanup(curl_handle);
        return -1;
    }

    // Usa o novo display_name na struct de progresso.
    struct ProgressData progress_data = { .display_name = display_name };

    curl_easy_setopt(curl_handle, CURLOPT_URL, url);
    curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, 0L);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_data);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void*)pagefile);
    curl_easy_setopt(curl_handle, CURLOPT_XFERINFOFUNCTION, progress_callback);
    curl_easy_setopt(curl_handle, CURLOPT_XFERINFODATA, (void*)&progress_data);
    curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1L); // Seguir redirecionamentos
    curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "gitph-updater/1.0"); // Bom para APIs

    res = curl_easy_perform(curl_handle);
    printf("\n");

    if (res != CURLE_OK) {
        fprintf(stderr, "Error: curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        ret_code = -1;
    } else {
        printf("Download of %s completed successfully.\n", display_name);
    }

    fclose(pagefile);
    curl_easy_cleanup(curl_handle);
    curl_global_cleanup();

    return ret_code;
}