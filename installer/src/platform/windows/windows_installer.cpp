/* Copyright (C) 2025 Pedro Henrique / phkaiser13
* File: windows_installer.cpp
* This file implements the WindowsInstaller class, which acts as a post-installation
* assistant. Its primary role is to check for external dependencies like Git, Terraform,
* and Vault, and interactively guide the user to install them if they are missing.
* It does *not* perform any file installation or system modification itself, as that
* is the responsibility of the primary installer (e.g., NSIS).
* SPDX-License-Identifier: Apache-2.0
*/

#include "platform/windows/windows_installer.hpp"
#include "spdlog/spdlog.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h> // Required for ShellExecuteA to open URLs
#include <iostream>
#include <string>
#include <algorithm>
#include <cctype>

namespace phgit_installer::platform {

    // Anonymous namespace for internal helper functions
    namespace {
        /**
         * @brief Converts a string to lowercase.
         * @param s The string to convert.
         * @return The lowercased string.
         */
        std::string to_lower(std::string s) {
            std::transform(s.begin(), s.end(), s.begin(),
                           [](unsigned char c){ return std::tolower(c); });
            return s;
        }
    }

    WindowsInstaller::WindowsInstaller(
        platform::PlatformInfo info,
        const dependencies::DependencyManager& dep_manager,
        std::shared_ptr<utils::ApiManager> api_manager,
        std::shared_ptr<utils::ConfigManager> config)
        : m_platform_info(std::move(info)),
          m_dep_manager(dep_manager),
          m_api_manager(std::move(api_manager)),
          m_config(std::move(config)) {
        spdlog::debug("Windows Post-Installation Assistant initialized.");
    }

    void WindowsInstaller::run_installation() {
        spdlog::info("Starting post-installation dependency check for Windows.");
        std::cout << "--- Post-Installation Dependency Assistant ---\n" << std::endl;

        // Check for Git (required dependency)
        if (!m_dep_manager.get_status("git").value_or(dependencies::DependencyStatus{}).is_version_ok) {
            prompt_user_to_install_git();
        } else {
            spdlog::info("Git dependency is satisfied.");
            std::cout << "[OK] Git is installed and meets the version requirements." << std::endl;
        }

        // Check for Terraform (optional dependency)
        if (!m_dep_manager.get_status("terraform").value_or(dependencies::DependencyStatus{}).is_version_ok) {
            prompt_user_to_install_optional("Terraform");
        } else {
            spdlog::info("Terraform dependency is satisfied.");
            std::cout << "[OK] Terraform is installed and meets the version requirements." << std::endl;
        }

        // Check for Vault (optional dependency)
        if (!m_dep_manager.get_status("vault").value_or(dependencies::DependencyStatus{}).is_version_ok) {
            prompt_user_to_install_optional("Vault");
        } else {
            spdlog::info("Vault dependency is satisfied.");
            std::cout << "[OK] Vault is installed and meets the version requirements." << std::endl;
        }

        spdlog::info("Dependency check completed.");
        std::cout << "\nDependency check finished. The application is now ready." << std::endl;
        std::cout << "Press Enter to exit." << std::endl;
        std::cin.get(); // Wait for user confirmation before closing the console
    }

    void WindowsInstaller::prompt_user_to_install_git() {
        spdlog::warn("Required dependency 'Git' is missing or outdated.");
        std::cout << "\n[REQUIRED] Git was not found on your system or the version is too old." << std::endl;
        std::cout << "Git is essential for the core functionality of this application." << std::endl;

        char user_choice;
        while (true) {
            std::cout << "Would you like to open the official Git for Windows download page in your browser? (y/n): ";
            std::cin >> user_choice;
            user_choice = std::tolower(user_choice);
            if (user_choice == 'y' || user_choice == 'n') {
                // Clear the input buffer for the final 'cin.get()' in run_installation
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                break;
            }
            std::cout << "Invalid input. Please enter 'y' for yes or 'n' for no." << std::endl;
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        }

        if (user_choice == 'y') {
            // Fetch the URL from a reliable source (config or API)
            std::string url = m_api_manager->get_download_page_url("git_for_windows").value_or("https://git-scm.com/download/win");
            spdlog::info("User chose to open the Git download page: {}", url);
            std::cout << "Opening " << url << " in your default browser..." << std::endl;

            // Use ShellExecuteA for maximum compatibility to open the URL
            ShellExecuteA(NULL, "open", url.c_str(), NULL, NULL, SW_SHOWNORMAL);

            std::cout << "Please download and run the installer. After installation, you may need to restart this application." << std::endl;
        } else {
            spdlog::info("User declined to install Git at this time.");
            std::cout << "You can install Git later, but some features may not work correctly." << std::endl;
        }
    }

    void WindowsInstaller::prompt_user_to_install_optional(const std::string& dependency_name) {
        spdlog::warn("Optional dependency '{}' is missing or outdated.", dependency_name);
        std::cout << "\n[OPTIONAL] " << dependency_name << " was not found on your system." << std::endl;
        std::cout << "This tool is recommended for extended features but is not required for basic operation." << std::endl;

        char user_choice;
        while (true) {
            std::cout << "Would you like to open the official " << dependency_name << " download page? (y/n): ";
            std::cin >> user_choice;
            user_choice = std::tolower(user_choice);
            if (user_choice == 'y' || user_choice == 'n') {
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                break;
            }
            std::cout << "Invalid input. Please enter 'y' for yes or 'n' for no." << std::endl;
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        }

        if (user_choice == 'y') {
            std::string api_key = to_lower(dependency_name);
            std::string default_url = "https://www.google.com/search?q=download+" + dependency_name;
            std::string url = m_api_manager->get_download_page_url(api_key).value_or(default_url);

            spdlog::info("User chose to open the {} download page: {}", dependency_name, url);
            std::cout << "Opening " << url << " in your default browser..." << std::endl;

            ShellExecuteA(NULL, "open", url.c_str(), NULL, NULL, SW_SHOWNORMAL);

            std::cout << "Please follow the instructions on the website to install " << dependency_name << "." << std::endl;
        } else {
            spdlog::info("User declined to install optional dependency '{}'.", dependency_name);
            std::cout << "You can install " << dependency_name << " at any time to enable its related features." << std::endl;
        }
    }

} // namespace phgit_installer::platform