/* Copyright (C) 2025 Pedro Henrique / phkaiser13
* File: linux_installer.hpp
* This header defines the fully integrated LinuxInstaller class. It is designed to run as
* a post-installation script, using the system's native package manager to satisfy
* dependencies. It also includes a robust fallback mechanism to install from a tarball,
* using the ApiManager and Downloader for a complete, end-to-end solution.
* SPDX-License-Identifier: Apache-2.0
*/

#pragma once

#include "platform/iplatform_installer.hpp"
#include "platform/platform_detector.hpp"
#include "dependencies/dependency_manager.hpp"
#include "utils/api_manager.hpp"
#include "utils/config_manager.hpp"

#include <string>
#include <memory>
#include <vector>

namespace phgit_installer::platform {

    class LinuxInstaller : public IPlatformInstaller {
    public:
        /**
         * @brief Constructs the LinuxInstaller engine.
         * @param info The detailed platform information for the Linux system.
         * @param dep_manager A reference to the dependency manager.
         * @param api_manager A shared pointer to the API manager for dynamic downloads.
         * @param config A shared pointer to the configuration manager for metadata.
         */
        explicit LinuxInstaller(
            platform::PlatformInfo info,
            const dependencies::DependencyManager& dep_manager,
            std::shared_ptr<utils::ApiManager> api_manager,
            std::shared_ptr<utils::ConfigManager> config
        );

        void run_installation() override;

    private:
        void dispatch_installation_strategy();
        void install_using_apt();
        void install_using_dnf();
        void install_using_pacman();
        void install_using_zypper();
        void install_from_tarball();

        /**
         * @brief Installs required system packages using the specified package manager.
         * @param package_manager_cmd The full command for the package manager (e.g., "sudo apt-get install -y").
         * @param packages A list of package names to install.
         * @return True if all installations were successful, false otherwise.
         */
        bool install_system_dependencies(const std::string& package_manager_cmd, const std::vector<std::string>& packages);

        /**
         * @brief Extracts a .tar.gz archive to a destination directory.
         * @param archive_path Path to the .tar.gz file.
         * @param dest_dir Directory to extract into.
         * @return True on success, false otherwise.
         */
        bool untar_archive(const std::string& archive_path, const std::string& dest_dir);

        platform::PlatformInfo m_platform_info;
        const dependencies::DependencyManager& m_dep_manager;
        std::shared_ptr<utils::ApiManager> m_api_manager;
        std::shared_ptr<utils::ConfigManager> m_config;
    };

} // namespace phgit_installer::platform
