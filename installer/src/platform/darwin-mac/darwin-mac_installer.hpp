/* Copyright (C) 2025 Pedro Henrique / phkaiser13
* File: darwin-mac_installer.hpp
* This header defines the MacosInstaller class, responsible for orchestrating the phgit
* installation on Apple's macOS (Darwin). The primary strategy is to leverage the
* Homebrew package manager if it's available, as it provides a seamless experience for
* managing dependencies like Git, Terraform, and Vault. If Homebrew is not present,
* the installer will fall back to using a native .pkg installer or guide the user
* through a manual installation process.
* SPDX-License-Identifier: Apache-2.0
*/

#pragma once

#include "platform/iplatform_installer.hpp"
#include "platform/platform_detector.hpp"
#include "dependencies/dependency_manager.hpp"

#include <string>
#include <memory>
#include <vector>

namespace phgit_installer::platform {

    class MacosInstaller : public IPlatformInstaller {
    public:
        /**
         * @brief Constructs the MacosInstaller.
         * @param info The detailed platform information for the macOS system.
         * @param dep_manager A reference to the dependency manager to query dependency status.
         */
        explicit MacosInstaller(platform::PlatformInfo info, const dependencies::DependencyManager& dep_manager);

        /**
         * @brief Overrides the base method to run the macOS-specific installation workflow.
         */
        void run_installation() override;

    private:
        /**
         * @brief Checks for the presence of the Homebrew package manager.
         * @return True if 'brew' is found in the PATH, false otherwise.
         */
        bool is_homebrew_available();

        /**
         * @brief Selects the best installation strategy based on available tools (e.g., Homebrew).
         */
        void dispatch_installation_strategy();

        /**
         * @brief Handles the entire installation process using Homebrew.
         * This is the preferred method on macOS.
         */
        void install_using_homebrew();

        /**
         * @brief Handles installation from a pre-built .pkg file.
         * This method uses the native `installer` command-line tool.
         */
        void install_from_pkg();

        /**
         * @brief Guides the user through a manual installation.
         * This involves downloading binaries and instructing the user on PATH configuration.
         */
        void perform_manual_installation();

        /**
         * @brief Installs or updates dependencies (git, terraform, vault) using Homebrew.
         */
        void ensure_dependencies_with_brew();

        /**
         * @brief Prompts the user to install Xcode Command Line Tools to get Git.
         * This is the standard fallback if Git is missing and Homebrew is not used.
         */
        void prompt_for_command_line_tools();

        // Member variables
        platform::PlatformInfo m_platform_info;
        const dependencies::DependencyManager& m_dep_manager;
        bool m_homebrew_is_available;
    };

} // namespace phgit_installer::platform
