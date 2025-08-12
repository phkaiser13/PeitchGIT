/* Copyright (C) 2025 Pedro Henrique / phkaiser13
 * download.c - Um downloader de arquivos robusto usando libcurl.
 *
 * Este módulo fornece funcionalidade de download de arquivos confiável para
 * o helper de instalação. Ele usa o mecanismo de callback do libcurl
 * para escrever dados e exibir uma barra de progresso em tempo real.
 *
 * SPDX-License-Identifier: Apache-2.0 */

#include <stdio.h>
#include <curl/curl.h>

// Struct de progresso para um nome de exibição customizado.
struct ProgressData {
    const char* display_name; // Nome amigável para a UI (ex: "Git for Windows")
};

// Função de callback para escrever os dados recebidos em um arquivo.
static size_t write_data(void* contents, size_t size, size_t nmemb, void* userdata) {
    FILE* fp = (FILE*)userdata;
    return fwrite(contents, size, nmemb, fp);
}

// Função de callback para desenhar a barra de progresso.
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

        printf("\rBaixando %s: [", data->display_name);
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
 * @brief Baixa um arquivo de uma dada URL para um caminho de destino especificado.
 *
 * @param url A URL do arquivo para baixar.
 * @param outpath O caminho do arquivo local onde o arquivo baixado será salvo.
 * @param display_name Um nome amigável para o download, mostrado na barra de progresso.
 * @return 0 em sucesso, -1 em falha.
 */
int download_file(const char* url, const char* outpath, const char* display_name) {
    CURL* curl_handle;
    FILE* pagefile;
    CURLcode res;
    int ret_code = 0;

    curl_global_init(CURL_GLOBAL_ALL);
    curl_handle = curl_easy_init();
    if (!curl_handle) {
        fprintf(stderr, "Erro: Falha ao inicializar o handle do curl.\n");
        return -1;
    }

    pagefile = fopen(outpath, "wb");
    if (!pagefile) {
        fprintf(stderr, "Erro: Nao foi possivel abrir o arquivo de saida %s\n", outpath);
        curl_easy_cleanup(curl_handle);
        return -1;
    }

    struct ProgressData progress_data = { .display_name = display_name };

    curl_easy_setopt(curl_handle, CURLOPT_URL, url);
    curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, 0L);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_data);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void*)pagefile);
    curl_easy_setopt(curl_handle, CURLOPT_XFERINFOFUNCTION, progress_callback);
    curl_easy_setopt(curl_handle, CURLOPT_XFERINFODATA, (void*)&progress_data);
    curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1L); // Seguir redirecionamentos
    curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "gitph-installer/1.0"); // Boa prática para APIs

    res = curl_easy_perform(curl_handle);
    printf("\n"); // Nova linha após a barra de progresso

    if (res != CURLE_OK) {
        fprintf(stderr, "Erro: curl_easy_perform() falhou: %s\n", curl_easy_strerror(res));
        ret_code = -1;
    } else {
        printf("Download de %s concluido com sucesso.\n", display_name);
    }

    fclose(pagefile);
    curl_easy_cleanup(curl_handle);
    curl_global_cleanup();

    return ret_code;
}
