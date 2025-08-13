/* Copyright (C) 2025 Pedro Henrique / phkaiser13
* File: linux_installer.hpp
* This header defines the LinuxInstaller class, which is responsible for handling the
* entire installation process on Linux systems. It uses the detected platform information
* to delegate tasks to the appropriate package manager (APT, DNF, Pacman, etc.) or
* falls back to a universal tarball installation if the distribution is not
* directly supported. The class is designed to be a comprehensive solution for the
* diverse Linux ecosystem.
* SPDX-License-Identifier: Apache-2.0
*/

#pragma once

#include "platform/iplatform_installer.hpp"
#include "platform/platform_detector.hpp"
#include "dependencies/dependency_manager.hpp" // To check dependency status

#include <string>
#include <memory>

namespace phgit_installer::platform {

    class LinuxInstaller : public IPlatformInstaller {
    public:
        /**
         * @brief Constructs the LinuxInstaller.
         * @param info The detailed platform information for the Linux system.
         * @param dep_manager A reference to the dependency manager to query dependency status.
         */
        explicit LinuxInstaller(platform::PlatformInfo info, const dependencies::DependencyManager& dep_manager);

        /**
         * @brief Overrides the base method to run the Linux-specific installation workflow.
         */
        void run_installation() override;

    private:
        /**
         * @brief The main strategy method that selects the correct installation path.
         * It inspects the os_id from PlatformInfo and calls the relevant method.
         */
        void dispatch_installation_strategy();

        /**
         * @brief Handles installation on Debian-based systems (Ubuntu, Mint, etc.).
         */
        void install_using_apt();

        /**
         * @brief Handles installation on Fedora-based systems (RHEL, CentOS, etc.).
         */
        void install_using_dnf();

        /**
         * @brief Handles installation on Arch-based systems.
         */
        void install_using_pacman();

        /**
         * @brief Handles installation on openSUSE-based systems.
         */
        void install_using_zypper();

        /**
         * @brief The fallback method for unsupported distributions or when native packaging fails.
         * This involves downloading a pre-compiled tarball, extracting it, and setting up the PATH.
         */
        void install_from_tarball();

        /**
         * @brief Installs required dependencies using the system's package manager.
         * @param package_manager_cmd The command for the package manager (e.g., "apt-get", "dnf").
         * @param packages A list of package names to install.
         */
        void install_system_dependencies(const std::string& package_manager_cmd, const std::vector<std::string>& packages);

        /**
         * @brief Installs the phgit package from a local file (.deb, .rpm).
         * @param package_tool_cmd The command for the package tool (e.g., "dpkg -i", "rpm -ivh").
         * @param package_path The path to the local package file.
         */
        void install_local_package(const std::string& package_tool_cmd, const std::string& package_path);

        // Member variables
        platform::PlatformInfo m_platform_info;
        const dependencies::DependencyManager& m_dep_manager;
    };

} // namespace phgit_installer::platform
