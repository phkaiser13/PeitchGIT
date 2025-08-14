/* Copyright (C) 2025 Pedro Henrique / phkaiser13
* File: windows_installer.hpp
* This header defines the WindowsInstaller class, which manages the installation process
* on Microsoft Windows. Unlike POSIX systems, Windows lacks a standard, built-in package
* manager. Therefore, this class is designed to be a self-contained engine that can be
* embedded within a native installer package (like NSIS or MSI). It handles dependency
* detection (Git for Windows), silent installation of dependencies, downloading and
* extracting optional tools (Terraform, Vault), and critical system integration tasks
* such as modifying the system PATH and creating registry entries.
* SPDX-License-Identifier: Apache-2.0
*/

#pragma once

#include "platform/iplatform_installer.hpp"
#include "platform/platform_detector.hpp"
#include "dependencies/dependency_manager.hpp"

#include <string>
#include <vector>

namespace phgit_installer::platform {

    class WindowsInstaller : public IPlatformInstaller {
    public:
        /**
         * @brief Constructs the WindowsInstaller.
         * @param info The detailed platform information for the Windows system.
         * @param dep_manager A reference to the dependency manager to query dependency status.
         */
        explicit WindowsInstaller(platform::PlatformInfo info, const dependencies::DependencyManager& dep_manager);

        /**
         * @brief Overrides the base method to run the Windows-specific installation workflow.
         */
        void run_installation() override;

    private:
        /**
         * @brief The main method that orchestrates the installation steps on Windows.
         */
        void perform_installation();

        /**
         * @brief Ensures Git for Windows is installed. If not, downloads and runs its
         *        silent installer.
         * @return True on success, false on failure.
         */
        bool ensure_git_is_installed();

        /**
         * @brief Ensures optional dependencies (Terraform, Vault) are installed if requested.
         * @param target_dir The directory where the tools should be placed.
         * @return True on success, false on failure.
         */
        bool ensure_optional_dependencies(const std::string& target_dir);

        /**
         * @brief Installs the main phgit application files.
         * In a real installer, this would involve copying files from the installation media.
         * @param install_path The root installation directory for phgit.
         * @return True on success, false on failure.
         */
        bool install_application_files(const std::string& install_path);

        /**
         * @brief Handles all system integration tasks.
         * @param install_path The root installation directory for phgit.
         */
        void perform_system_integration(const std::string& install_path);

        /**
         * @brief Adds a directory to the user or system PATH environment variable.
         * This is a critical and sensitive operation on Windows.
         * @param directory_to_add The directory to add to the PATH.
         * @param for_all_users If true, modifies the system PATH; otherwise, the user PATH.
         */
        void update_path_environment_variable(const std::string& directory_to_add, bool for_all_users);

        /**
         * @brief Creates registry keys for Add/Remove Programs and application settings.
         * @param install_path The root installation directory for phgit.
         * @param for_all_users If true, writes to HKLM; otherwise, HKCU.
         */
        void create_registry_entries(const std::string& install_path, bool for_all_users);

        /**
         * @brief Creates Start Menu shortcuts.
         * @param install_path The root installation directory for phgit.
         */
        void create_start_menu_shortcuts(const std::string& install_path);

        // Member variables
        platform::PlatformInfo m_platform_info;
        const dependencies::DependencyManager& m_dep_manager;
        // TODO: Add a Downloader component member variable
    };

} // namespace phgit_installer::platform
