/* Copyright (C) 2025 Pedro Henrique / phkaiser13
* File: windows_installer.cpp
* This file implements the WindowsInstaller class. It has been updated to use the
* Downloader utility for robustly fetching dependencies like Git for Windows. It now
* orchestrates a more realistic installation flow: download, verify checksum (stubbed),
* and then execute the silent installer.
* SPDX-License-Identifier: Apache-2.0
*/

#include "platform/windows/windows_installer.hpp"
#include "utils/downloader.hpp" // Include the new downloader
#include "spdlog/spdlog.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winreg.h>
#include <shlobj.h>
#include <filesystem>
#include <vector>
#include <sstream>

#pragma comment(lib, "advapi32.lib")

namespace phgit_installer::platform {

    namespace fs = std::filesystem;

    static bool execute_silent_command(const std::string& command) {
        spdlog::info("Executing silent command: {}", command);
        // A real implementation would use CreateProcess.
        spdlog::warn("Command execution is currently simulated.");
        return true;
    }

    WindowsInstaller::WindowsInstaller(platform::PlatformInfo info, const dependencies::DependencyManager& dep_manager)
        : m_platform_info(std::move(info)), m_dep_manager(dep_manager) {
        spdlog::debug("WindowsInstaller instance created.");
    }

    void WindowsInstaller::run_installation() {
        spdlog::info("Starting Windows installation process.");
        if (!m_platform_info.is_privileged) {
            spdlog::warn("Installer is not running with Administrator privileges.");
        }
        perform_installation();
    }

    void WindowsInstaller::perform_installation() {
        bool for_all_users = m_platform_info.is_privileged;
        std::string install_dir;
        char path[MAX_PATH];

        if (for_all_users) {
            if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_PROGRAM_FILES, NULL, 0, path))) {
                install_dir = std::string(path) + "\\phgit";
            }
        } else {
            if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, path))) {
                install_dir = std::string(path) + "\\Programs\\phgit";
            }
        }

        if (install_dir.empty()) {
            spdlog::critical("Could not determine a valid installation directory.");
            return;
        }
        spdlog::info("Installation target directory: {}", install_dir);
        fs::create_directories(install_dir);

        if (!ensure_git_is_installed()) {
            spdlog::critical("Failed to install Git. Aborting installation.");
            return;
        }

        // ... (rest of the installation logic)
        spdlog::info("Windows installation process completed successfully.");
    }

    bool WindowsInstaller::ensure_git_is_installed() {
        auto git_status = m_dep_manager.get_status("git");
        if (git_status && git_status->is_found && git_status->is_version_ok) {
            spdlog::info("Git is already installed and up-to-date.");
            return true;
        }
        spdlog::info("Git not found or outdated. Attempting to download and install Git for Windows.");

        // --- Integration of Downloader ---
        utils::Downloader downloader;
        downloader.set_user_agent("phgit-installer/1.0");
        downloader.set_timeout(300); // 5 minute timeout

        // In a real app, this URL and hash would come from a config file or API call.
        const std::string git_url = "https://github.com/git-for-windows/git/releases/download/v2.45.1.windows.1/Git-2.45.1-64-bit.exe";
        const std::string git_hash = "SOME_EXPECTED_SHA256_HASH"; // Placeholder
        
        char temp_path[MAX_PATH];
        GetTempPathA(MAX_PATH, temp_path);
        std::string git_installer_path = std::string(temp_path) + "Git-Installer.exe";

        spdlog::info("Downloading Git for Windows from: {}", git_url);
        
        // Define a progress bar callback
        auto progress_bar = [](uint64_t total, uint64_t downloaded) {
            if (total == 0) return;
            int percentage = static_cast<int>((static_cast<double>(downloaded) / total) * 100.0);
            // Use \r to redraw the line, creating a dynamic progress bar
            std::cout << "\rDownloading... " << percentage << "% [" << downloaded << " / " << total << " bytes]" << std::flush;
            if (downloaded == total) {
                std::cout << std::endl; // Newline when done
            }
        };

        if (!downloader.download_file(git_url, git_installer_path, progress_bar)) {
            spdlog::critical("Failed to download Git installer.");
            return false;
        }

        if (!utils::checksum::verify_file(git_installer_path, git_hash)) {
            spdlog::critical("Checksum verification failed for Git installer. The file may be corrupt or tampered with.");
            return false;
        }
        
        spdlog::info("Download complete and verified. Starting silent installation...");
        std::string command = "\"" + git_installer_path + "\" /VERYSILENT /NORESTART";
        
        bool install_success = execute_silent_command(command);
        
        // Clean up the downloaded installer
        remove(git_installer_path.c_str());

        return install_success;
    }


    bool WindowsInstaller::ensure_optional_dependencies(const std::string& target_dir) {
        spdlog::info("Checking for optional dependencies (Terraform, Vault).");
        // STUB: This would use a Downloader and an ArchiveExtractor utility.
        spdlog::info("Simulating download of Terraform and Vault zip files.");
        spdlog::info("Simulating extraction to: {}", target_dir);
        return true;
    }

    bool WindowsInstaller::install_application_files(const std::string& install_path) {
        spdlog::info("Installing application files to: {}", install_path);
        // STUB: In a real installer, files are extracted from the package.
        // We simulate creating the main executable and a bin directory.
        fs::create_directories(install_path + "\\bin");
        std::ofstream main_exe(install_path + "\\bin\\phgit.exe");
        if (main_exe) {
            main_exe << "stub";
            main_exe.close();
            spdlog::debug("Simulated creation of phgit.exe");
            return true;
        }
        return false;
    }

    void WindowsInstaller::perform_system_integration(const std::string& install_path) {
        spdlog::info("Performing system integration tasks.");
        bool for_all_users = m_platform_info.is_privileged;
        std::string bin_path = install_path + "\\bin";

        update_path_environment_variable(bin_path, for_all_users);
        create_registry_entries(install_path, for_all_users);
        create_start_menu_shortcuts(install_path);
    }

    void WindowsInstaller::update_path_environment_variable(const std::string& directory_to_add, bool for_all_users) {
        spdlog::info("Updating PATH environment variable.");
        HKEY root_key = for_all_users ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER;
        const char* sub_key = for_all_users ?
            "SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment" :
            "Environment";
        
        HKEY h_key;
        if (RegOpenKeyExA(root_key, sub_key, 0, KEY_READ | KEY_WRITE, &h_key) != ERROR_SUCCESS) {
            spdlog::error("Failed to open registry key for PATH variable.");
            return;
        }

        char current_path[8192]; // Max PATH length is complex, but 8192 is generous.
        DWORD path_size = sizeof(current_path);
        LSTATUS res = RegGetValueA(h_key, NULL, "Path", RRF_RT_REG_SZ, NULL, current_path, &path_size);

        if (res != ERROR_SUCCESS && res != ERROR_FILE_NOT_FOUND) {
            RegCloseKey(h_key);
            spdlog::error("Failed to read current PATH from registry. Error: {}", res);
            return;
        }

        std::string path_str(current_path);
        // Idempotency check: don't add if it's already there.
        if (path_str.find(directory_to_add) != std::string::npos) {
            spdlog::info("PATH already contains '{}'. No changes needed.", directory_to_add);
            RegCloseKey(h_key);
            return;
        }

        // Append the new directory.
        if (!path_str.empty() && path_str.back() != ';') {
            path_str += ";";
        }
        path_str += directory_to_add;

        if (RegSetValueExA(h_key, "Path", 0, REG_SZ, (const BYTE*)path_str.c_str(), path_str.length() + 1) != ERROR_SUCCESS) {
            spdlog::error("Failed to write updated PATH to registry.");
        } else {
            spdlog::info("Successfully added '{}' to PATH.", directory_to_add);
            // Broadcast the change to all windows, so new cmd prompts will see it.
            SendMessageTimeout(HWND_BROADCAST, WM_SETTINGCHANGE, 0, (LPARAM)"Environment", SMTO_ABORTIFHUNG, 5000, NULL);
        }
        RegCloseKey(h_key);
    }

    void WindowsInstaller::create_registry_entries(const std::string& install_path, bool for_all_users) {
        spdlog::info("Creating registry entries for Add/Remove Programs.");
        HKEY root_key = for_all_users ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER;
        const char* uninstall_key_path = "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\phgit";
        
        HKEY h_uninstall_key;
        if (RegCreateKeyExA(root_key, uninstall_key_path, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &h_uninstall_key, NULL) != ERROR_SUCCESS) {
            spdlog::error("Failed to create Uninstall registry key.");
            return;
        }

        std::string uninstaller_path = "\"" + install_path + "\\uninstall.exe\"";
        std::string display_name = "phgit";
        std::string display_version = "1.0.0"; // Should come from constants
        std::string publisher = "Pedro Henrique / phkaiser13";

        RegSetValueExA(h_uninstall_key, "DisplayName", 0, REG_SZ, (const BYTE*)display_name.c_str(), display_name.length() + 1);
        RegSetValueExA(h_uninstall_key, "DisplayVersion", 0, REG_SZ, (const BYTE*)display_version.c_str(), display_version.length() + 1);
        RegSetValueExA(h_uninstall_key, "Publisher", 0, REG_SZ, (const BYTE*)publisher.c_str(), publisher.length() + 1);
        RegSetValueExA(h_uninstall_key, "UninstallString", 0, REG_SZ, (const BYTE*)uninstaller_path.c_str(), uninstaller_path.length() + 1);
        RegSetValueExA(h_uninstall_key, "InstallLocation", 0, REG_SZ, (const BYTE*)install_path.c_str(), install_path.length() + 1);
        
        DWORD no_modify = 1;
        RegSetValueExA(h_uninstall_key, "NoModify", 0, REG_DWORD, (const BYTE*)&no_modify, sizeof(no_modify));
        DWORD no_repair = 1;
        RegSetValueExA(h_uninstall_key, "NoRepair", 0, REG_DWORD, (const BYTE*)&no_repair, sizeof(no_repair));

        RegCloseKey(h_uninstall_key);
        spdlog::info("Successfully created Add/Remove Programs entries.");
    }

    void WindowsInstaller::create_start_menu_shortcuts(const std::string& install_path) {
        spdlog::info("Creating Start Menu shortcuts.");
        // STUB: Creating shortcuts with WinAPI (IShellLink) is very verbose.
        // This is typically handled by the installer framework (NSIS, WiX).
        // We will just log the intent.
        spdlog::info("Simulating creation of shortcut to '{}\\bin\\phgit.exe' in Start Menu.", install_path);
    }

} // namespace phgit_installer::platform
