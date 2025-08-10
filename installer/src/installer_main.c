/* Copyright (C) 2025 Pedro Henrique / phkaiser13
 * installer_main.c - Main logic for the gitph installer.
 *
 * This program is the user's first interaction with gitph. It orchestrates
 * the entire installation process as a robust state machine:
 * 1. Detects the critical 'git' dependency by inspecting the system's PATH.
 * 2. Offers to download the official Git installer if it's missing.
 * 3. Copies the application files to a standard user-level location.
 * 4. Modifies the user's PATH environment variable persistently.
 * 5. Provides clear feedback and instructions to the user at every step.
 *
 * It is written in pure C with platform-specific code to ensure maximum
 * compatibility and low-level control over the system environment.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// Forward declaration for our download utility
int download_file(const char* url, const char* outpath);

// Platform-specific includes and definitions
#ifdef _WIN32
#include <windows.h>
#include <ShlObj.h> // For SHGetFolderPath
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "shell32.lib")
#define PATH_DELIMITER ";"
#else
#include <sys/stat.h>
#include <unistd.h>
#define PATH_DELIMITER ":"
#endif

/**
 * @brief Checks if the 'git' executable is available in the system's PATH.
 * @return true if git is found, false otherwise.
 */
bool is_git_in_path() {
    const char* path_env = getenv("PATH");
    if (!path_env) return false;

    char* path_copy = strdup(path_env);
    char* path_token = strtok(path_copy, PATH_DELIMITER);

    while (path_token != NULL) {
        char git_path[1024];
#ifdef _WIN32
        snprintf(git_path, sizeof(git_path), "%s\\git.exe", path_token);
#else
        snprintf(git_path, sizeof(git_path), "%s/git", path_token);
#endif

        // Check if file exists and is executable
#ifdef _WIN32
        if (GetFileAttributesA(git_path) != INVALID_FILE_ATTRIBUTES) {
            free(path_copy);
            return true;
        }
#else
        struct stat st;
        if (stat(git_path, &st) == 0 && (st.st_mode & S_IXUSR)) {
            free(path_copy);
            return true;
        }
#endif
        path_token = strtok(NULL, PATH_DELIMITER);
    }

    free(path_copy);
    return false;
}

/**
 * @brief Handles the process of downloading and guiding the user to install Git.
 */
void handle_git_download() {
    printf("Git not found. It is a required dependency.\n");
    printf("Would you like to download the official Git installer? (y/n): ");
    int choice = getchar();

    if (choice == 'y' || choice == 'Y') {
#ifdef _WIN32
        const char* git_url = "https://github.com/git-for-windows/git/releases/download/v2.37.1.windows.1/Git-2.37.1-64-bit.exe";
        const char* out_filename = "Git-Installer.exe";
#else
        // On Linux/macOS, it's better to guide the user to their package manager.
        printf("\nPlease install Git using your system's package manager.\n");
        printf("  - On Debian/Ubuntu: sudo apt-get install git\n");
        printf("  - On Fedora/CentOS: sudo dnf install git\n");
        printf("  - On macOS (with Homebrew): brew install git\n");
        return;
#endif
        if (download_file(git_url, out_filename) == 0) {
            printf("\nSuccessfully downloaded '%s'.\n", out_filename);
            printf("Please run this installer, then run the gitph installer again.\n");
        } else {
            printf("\nFailed to download the Git installer.\n");
        }
    } else {
        printf("Installation cancelled. Please install Git manually and try again.\n");
    }
}

/**
 * @brief Installs gitph files and configures the PATH.
 */
void install_gitph() {
    printf("Installing gitph...\n");
    // TODO: Implement file copying logic.
    // 1. Get user's home/appdata directory.
    // 2. Create a 'gitph/bin' and 'gitph/modules' directory.
    // 3. Copy 'gitph.exe' to 'gitph/bin'.
    // 4. Copy the entire 'modules' folder to 'gitph/modules'.

    // TODO: Implement PATH modification logic.
    // This is a highly platform-specific and sensitive operation.
    // On Windows, it involves modifying the Registry.
    // On POSIX, it involves appending to shell profile files (~/.bashrc, etc.).

    printf("Placeholder: Files would be copied and PATH would be set here.\n");
}

int main() {
    printf("========================================\n");
    printf("  gitph - Installer\n");
    printf("========================================\n\n");

    // State 1: Check for Git dependency.
    printf("Checking for dependencies...\n");
    if (is_git_in_path()) {
        printf("-> Git found on your system.\n\n");

        // State 2: Install the application.
        install_gitph();

        // State 3: Conclude.
        printf("\nInstallation complete!\n");
        printf("Please restart your terminal to use the 'gitph' command.\n");

    } else {
        // State 2 (Alternate Path): Handle missing dependency.
        handle_git_download();
    }

    printf("\n========================================\n");
    return 0;
}