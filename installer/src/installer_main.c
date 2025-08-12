/* Copyright (C) 2025 Pedro Henrique / phkaiser13
 * installer_main.c - Lógica principal do helper do instalador.
 *
 * Responsabilidades:
 * 1. Servir como um executável chamado pelo script NSIS.
 * 2. Checar em tempo de execução pela existência do Git.
 * 3. Utilizar o módulo de download para baixar o instalador do Git se necessário.
 * 4. Potencialmente, pode ser estendido para outras tarefas do instalador.
 *
 * SPDX-License-Identifier: Apache-2.0 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// Declaração da função que está em download.c
int download_file(const char* url, const char* outpath, const char* display_name);

/**
 * @brief Checa se o 'git' executável está disponível no PATH do sistema.
 * @return true se o git for encontrado, false caso contrário.
 */
bool is_git_in_path() {
#ifdef _WIN32
    return (system("git --version > nul 2>&1") == 0);
#else
    return (system("command -v git > /dev/null 2>&1") == 0);
#endif
}

/**
 * @brief Exibe uma mensagem de ajuda simples para o helper.
 */
void show_help() {
    printf("installer_helper - Utilitário de instalação para gitph.\n");
    printf("Uso: installer_helper <URL> <caminho_saida>\n");
    printf("Este utilitário é projetado para ser chamado pelo script do instalador NSIS.\n");
}

/**
 * @brief Ponto de entrada principal para o helper do instalador.
 *
 * Argumentos esperados da linha de comando (passados pelo NSIS):
 * @param argc - Deve ser 3.
 * @param argv[1] - A URL para baixar o instalador do Git.
 * @param argv[2] - O caminho de destino para salvar o instalador baixado.
 * @return 0 em sucesso, 1 em falha.
 */
int main(int argc, char* argv[]) {
    // Se o Git já estiver instalado, não há nada a fazer.
    // O script NSIS já faz essa checagem, mas uma dupla verificação é segura.
    if (is_git_in_path()) {
        printf("Git já está instalado. Nenhum download necessário.\n");
        return 0;
    }

    if (argc != 3) {
        fprintf(stderr, "Erro: Uso incorreto do installer_helper.\n");
        show_help();
        return 1;
    }

    const char* url = argv[1];
    const char* outpath = argv[2];
    const char* display_name = "Git For Windows"; // Nome amigável para a barra de progresso

    printf("Iniciando o download do Git...\n");
    printf("  De: %s\n", url);
    printf("  Para: %s\n", outpath);

    // Chama a função de download
    if (download_file(url, outpath, display_name) != 0) {
        fprintf(stderr, "Erro: Falha ao baixar o instalador do Git.\n");
        return 1;
    }

    printf("Download do instalador do Git concluído com sucesso.\n");
    return 0;
}
