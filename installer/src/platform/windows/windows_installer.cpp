/* Copyright (C) 2025 Pedro Henrique / phkaiser13
* File: windows_installer.cpp
* This file implements the WindowsInstaller class. It contains the low-level logic for
* installing phgit on Windows using the native WinAPI. It handles downloading and silently
* installing dependencies like Git for Windows, manipulating the system registry to update
* the PATH environment variable and to register an uninstaller, and creating shortcuts.
* This code is designed to be a robust engine for a user-friendly installer package.
* SPDX-License-Identifier: Apache-2.0
*/

#include "platform/windows/windows_installer.hpp"
#include "spdlog/spdlog.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winreg.h>
#include <shlobj.h> // For CSIDL constants
#include <filesystem>
#include <vector>
#include <sstream>

#pragma comment(lib, "advapi32.lib") // Link against the registry library

namespace phgit_installer::platform {

    namespace fs = std::filesystem;

    // Helper function to simulate command execution.
    static bool execute_silent_command(const std::string& command) {
        spdlog::info("Executing silent command: {}", command);
        // A real implementation would use CreateProcess with DETACHED_PROCESS or CREATE_NO_WINDOW
        // flags and wait for the process to complete.
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
            spdlog::warn("System-wide installation (for all users) may fail.");
        }
        perform_installation();
    }

    void WindowsInstaller::perform_installation() {
        // For simplicity, we assume a per-machine (all users) installation if running as admin.
        bool for_all_users = m_platform_info.is_privileged;
        std::string install_dir;
        char path[MAX_PATH];

        if (for_all_users) {
            // C:\Program Files\phgit
            if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_PROGRAM_FILES, NULL, 0, path))) {
                install_dir = std::string(path) + "\\phgit";
            }
        } else {
            // C:\Users\<user>\AppData\Local\Programs\phgit
            if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, path))) {
                install_dir = std::string(path) + "\\Programs\\phgit";
            }
        }

        if (install_dir.empty()) {
            spdlog::critical("Could not determine a valid installation directory.");
            return;
        }
        spdlog::info("Installation target directory: {}", install_dir);

        // Create the directory if it doesn't exist
        fs::create_directories(install_dir);

        if (!ensure_git_is_installed()) {
            spdlog::critical("Failed to install Git. Aborting installation.");
            return;
        }

        if (!install_application_files(install_dir)) {
            spdlog::critical("Failed to install application files. Aborting.");
            return;
        }

        // Optional dependencies will be placed in a subdirectory
        std::string tools_dir = install_dir + "\\bin";
        fs::create_directories(tools_dir);
        ensure_optional_dependencies(tools_dir);

        perform_system_integration(install_dir);

        spdlog::info("Windows installation process completed successfully (simulated).");
    }

    bool WindowsInstaller::ensure_git_is_installed() {
        auto git_status = m_dep_manager.get_status("git");
        if (git_status && git_status->is_found && git_status->is_version_ok) {
            spdlog::info("Git is already installed and up-to-date.");
            return true;
        }
        spdlog::info("Git not found or outdated. Attempting to download and install Git for Windows.");
        
        // STUB: This would be handled by a Downloader component.
        std::string git_installer_path = "C:\\Users\\Public\\Downloads\\Git-2.45.1-64-bit.exe";
        spdlog::info("Simulating download of Git installer to: {}", git_installer_path);

        // These arguments are crucial for a non-interactive, silent installation.
        std::string command = "\"" + git_installer_path + "\" /VERYSILENT /NORESTART /NOCANCEL /SP- /CLOSEAPPLICATIONS /RESTARTAPPLICATIONS";
        return execute_silent_command(command);
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
