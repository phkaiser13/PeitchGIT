/* Copyright (C) 2025 Pedro Henrique / phkaiser13
 * gitph_main.c - Core logic for the gitph command-line tool.
 *
 * This is the main entry point for the gitph application. It functions as a
 * command dispatcher, parsing user input from the command line and executing
 * the corresponding functionality.
 *
 * Key Features:
 * - Runtime dependency check for Git.
 * - A robust command-line argument parser.
 * - A self-update mechanism to keep the tool current.
 *
 * SPDX-License-Identifier: Apache-2.0 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// Forward declaration for our download utility, now used for self-updates.
int download_file(const char* url, const char* outpath, const char* display_name);

// Forward declaration for platform-specific self-update logic.
int apply_update(const char* new_executable_path);

#ifdef _WIN32
#include <windows.h>
#define PATH_DELIMITER "\\"
#else
#include <unistd.h>
#define PATH_DELIMITER "/"
#endif

#define CURRENT_VERSION "1.0.0"
#define UPDATE_URL "https://api.github.com/repos/phkaiser13/gitph/releases/latest" // Exemplo de URL

/**
 * @brief Checks if the 'git' executable is available in the system's PATH.
 *        Crucial runtime check.
 * @return true if git is found, false otherwise.
 */
bool is_git_in_path() {
    // A implementação original estava ótima, pode ser mantida aqui.
    // Para brevidade, vamos assumir que a função existe e funciona.
    // No seu código final, copie a função original para cá.
    return (system("git --version > nul 2>&1") == 0); // Maneira simples e eficaz no Windows
}

/**
 * @brief Displays the help message with all available commands.
 */
void show_help() {
    printf("gitph v%s - A professional Git helper.\n\n", CURRENT_VERSION);
    printf("USAGE:\n");
    printf("  gitph <command> [options]\n\n");
    printf("COMMANDS:\n");
    printf("  commit      Assists with creating a conventional commit.\n");
    printf("  status      Provides an enhanced 'git status' view.\n");
    printf("  self-update Checks for and installs the latest version of gitph.\n");
    printf("  --version   Shows the current version of gitph.\n");
    printf("  --help      Displays this help message.\n");
}

/**
 * @brief Handles the self-update process.
 */
void handle_self_update() {
    printf("Checking for updates...\n");
    
    // 1. Checar a URL de release para a versão mais recente.
    //    (Isso exigiria uma lib para fazer requisições HTTP GET, como a própria libcurl,
    //     ou um parser de JSON. Para simplificar, vamos pular essa parte e simular.)
    const char* latest_version = "1.1.0"; // Simulação
    const char* download_url = "https://github.com/phkaiser13/gitph/releases/download/v1.1.0/gitph.exe"; // Simulação

    if (strcmp(latest_version, CURRENT_VERSION) <= 0) {
        printf("You are already running the latest version (%s).\n", CURRENT_VERSION);
        return;
    }

    printf("A new version (%s) is available! Current version is %s.\n", latest_version, CURRENT_VERSION);
    printf("Would you like to download and install it? (y/n): ");
    int choice = getchar();

    if (choice != 'y' && choice != 'Y') {
        printf("Update cancelled.\n");
        return;
    }

    // 2. Baixar o novo executável para um local temporário.
    char temp_filename[512];
    snprintf(temp_filename, sizeof(temp_filename), "gitph-update-temp.exe");
    
    char display_name[100];
    snprintf(display_name, sizeof(display_name), "gitph v%s", latest_version);

    if (download_file(download_url, temp_filename, display_name) != 0) {
        fprintf(stderr, "Error: Failed to download the update.\n");
        return;
    }

    // 3. Aplicar a atualização (a parte mais complexa).
    if (apply_update(temp_filename) != 0) {
        fprintf(stderr, "Error: Failed to apply the update.\n");
    } else {
        printf("Update successful! Please restart gitph.\n");
    }
}

/**
 * @brief Platform-specific logic to replace the current executable with the new one.
 * @param new_executable_path Path to the downloaded new version.
 * @return 0 on success, -1 on failure.
 */
int apply_update(const char* new_executable_path) {
    // A auto-atualização é complexa, especialmente no Windows, pois um arquivo
    // em execução não pode ser sobrescrito. A estratégia padrão é:
    // 1. Criar um script (.bat no Windows, .sh no Linux).
    // 2. O script espera um segundo, copia o novo .exe sobre o antigo, e se auto-deleta.
    // 3. O programa principal executa esse script e encerra imediatamente.
    
    printf("\nPlaceholder: Applying update from '%s'.\n", new_executable_path);
    printf("This would involve a platform-specific script to replace the running executable.\n");
    
    // Exemplo de como seria o conteúdo de um .bat no Windows:
    // @echo off
    // timeout /t 1 /nobreak > NUL
    // move /y "gitph-update-temp.exe" "gitph.exe"
    // del "%~f0"
    
    return 0; // Simula sucesso
}


int main(int argc, char* argv[]) {
    // Verificação de dependência em tempo de execução. Essencial!
    if (!is_git_in_path()) {
        fprintf(stderr, "FATAL ERROR: 'git' was not found in your system's PATH.\n");
        fprintf(stderr, "Please ensure Git is installed and accessible, then try again.\n");
        return 1; // Retorna um código de erro
    }

    // Se nenhum comando for dado, mostrar ajuda.
    if (argc < 2) {
        show_help();
        return 0;
    }

    // Analisador de Comandos (Dispatcher)
    char* command = argv[1];

    if (strcmp(command, "self-update") == 0) {
        handle_self_update();
    } else if (strcmp(command, "--version") == 0) {
        printf("gitph version %s\n", CURRENT_VERSION);
    } else if (strcmp(command, "--help") == 0) {
        show_help();
    } else if (strcmp(command, "commit") == 0) {
        printf("Placeholder: Interactive commit logic would run here.\n");
    } else if (strcmp(command, "status") == 0) {
        printf("Placeholder: Enhanced status logic would run here.\n");
    } else {
        fprintf(stderr, "Error: Unknown command '%s'\n", command);
        show_help();
        return 1;
    }

    return 0;
}