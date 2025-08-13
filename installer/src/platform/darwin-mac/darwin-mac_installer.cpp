/* Copyright (C) 2025 Pedro Henrique / phkaiser13
* File: darwin-mac_installer.cpp
* This file implements the MacosInstaller class. It contains the logic for installing
* phgit on macOS, with a primary strategy of using Homebrew for a seamless experience.
* It checks for Homebrew's existence and uses it to manage both the application and its
* dependencies. If Homebrew is not found, it gracefully falls back to using a standard
* .pkg installer, guiding the user if manual intervention (like installing Xcode
* Command Line Tools) is required.
* SPDX-License-Identifier: Apache-2.0
*/

#include "platform/darwin-mac/darwin-mac_installer.hpp"
#include "spdlog/spdlog.h"

#include <cstdlib> // For system()
#include <iostream>

namespace phgit_installer::platform {

    // Helper function to simulate command execution.
    // In a real implementation, this would use a robust process utility.
    static bool execute_system_command(const std::string& command) {
        spdlog::info("Executing command: {}", command);
        // int return_code = std::system(command.c_str());
        // if (return_code != 0) {
        //     spdlog::error("Command failed with exit code: {}", return_code);
        //     return false;
        // }
        spdlog::warn("Command execution is currently simulated.");
        return true;
    }

    MacosInstaller::MacosInstaller(platform::PlatformInfo info, const dependencies::DependencyManager& dep_manager)
        : m_platform_info(std::move(info)),
          m_dep_manager(dep_manager),
          m_homebrew_is_available(is_homebrew_available()) {
        spdlog::debug("MacosInstaller instance created. Homebrew available: {}", m_homebrew_is_available);
    }

    void MacosInstaller::run_installation() {
        spdlog::info("Starting macOS installation process.");
        dispatch_installation_strategy();
    }

    bool MacosInstaller::is_homebrew_available() {
        spdlog::debug("Checking for Homebrew...");
        // Using `command -v` is a standard POSIX way to check if a command exists in PATH.
        // It's more reliable than `which`. Redirecting output to /dev/null keeps it silent.
        if (std::system("command -v brew > /dev/null 2>&1") == 0) {
            spdlog::info("Homebrew detected on the system.");
            return true;
        }
        spdlog::info("Homebrew not found in PATH.");
        return false;
    }

    void MacosInstaller::dispatch_installation_strategy() {
        if (m_homebrew_is_available) {
            install_using_homebrew();
        } else {
            spdlog::warn("Homebrew not found. Falling back to .pkg installer strategy.");
            install_from_pkg();
        }
    }

    void MacosInstaller::install_using_homebrew() {
        spdlog::info("Using Homebrew installation strategy.");
        
        ensure_dependencies_with_brew();

        spdlog::info("Installing phgit via Homebrew...");
        // This assumes a custom tap `phkaiser13/tap` has been created.
        execute_system_command("brew tap phkaiser13/tap");
        execute_system_command("brew install phkaiser13/tap/phgit");

        spdlog::info("Homebrew installation process completed (simulated).");
    }

    void MacosInstaller::ensure_dependencies_with_brew() {
        bool needs_hashicorp_tap = false;

        auto git_status = m_dep_manager.get_status("git");
        if (!git_status || !git_status->is_version_ok) {
            spdlog::info("Git is missing or outdated. Installing/upgrading with Homebrew.");
            execute_system_command("brew install git");
        }

        auto terraform_status = m_dep_manager.get_status("terraform");
        if (!terraform_status || !terraform_status->is_version_ok) {
            spdlog::info("Terraform is missing or outdated. Will install with Homebrew.");
            needs_hashicorp_tap = true;
        }

        auto vault_status = m_dep_manager.get_status("vault");
        if (!vault_status || !vault_status->is_version_ok) {
            spdlog::info("Vault is missing or outdated. Will install with Homebrew.");
            needs_hashicorp_tap = true;
        }

        if (needs_hashicorp_tap) {
            spdlog::info("Tapping HashiCorp repository...");
            execute_system_command("brew tap hashicorp/tap");
            if (!terraform_status || !terraform_status->is_version_ok) {
                execute_system_command("brew install hashicorp/tap/terraform");
            }
            if (!vault_status || !vault_status->is_version_ok) {
                execute_system_command("brew install hashicorp/tap/vault");
            }
        }
    }

    void MacosInstaller::install_from_pkg() {
        spdlog::info("Using .pkg installation strategy.");

        auto git_status = m_dep_manager.get_status("git");
        if (!git_status || !git_status->is_found) {
            prompt_for_command_line_tools();
            // In a real installer, we would stop here and wait for the user.
            spdlog::error("Cannot proceed without Git. Please install Xcode Command Line Tools and run the installer again.");
            return; // Stop the installation flow.
        }

        // Placeholder for the .pkg file path. This would be downloaded or bundled.
        // A universal package would contain both x86_64 and arm64 binaries.
        std::string package_path = "phgit-1.0.0-universal.pkg";
        spdlog::info("Attempting to install from package: {}", package_path);

        // The native `installer` tool requires root privileges.
        std::string command = "sudo installer -pkg \"" + package_path + "\" -target /";
        if (execute_system_command(command)) {
            spdlog::info(".pkg installation completed successfully (simulated).");
        } else {
            spdlog::error(".pkg installation failed. Falling back to manual installation guidance.");
            perform_manual_installation();
        }
    }

    void MacosInstaller::perform_manual_installation() {
        spdlog::critical("Automatic installation failed.");
        spdlog::info("--------------------------------------------------");
        spdlog::info("MANUAL INSTALLATION REQUIRED:");
        spdlog::info("1. Ensure Git is installed. If not, run 'xcode-select --install' in your terminal.");
        spdlog::info("2. Download the latest phgit binary for your architecture ({}) from the GitHub releases page.", m_platform_info.architecture);
        spdlog::info("3. Unzip the download.");
        spdlog::info("4. Move the 'phgit' executable to a directory in your system's PATH, for example:");
        spdlog::info("   sudo mv phgit /usr/local/bin/");
        spdlog::info("--------------------------------------------------");
    }

    void MacosInstaller::prompt_for_command_line_tools() {
        spdlog::warn("--------------------------------------------------");
        spdlog::warn("REQUIRED ACTION: Git is not installed.");
        spdlog::warn("phgit requires Git to function.");
        spdlog::warn("To install it, please run the following command in your terminal:");
        spdlog::warn("  xcode-select --install");
        spdlog::warn("After the installation is complete, please run this installer again.");
        spdlog::warn("--------------------------------------------------");
    }

} // namespace phgit_installer::platform
