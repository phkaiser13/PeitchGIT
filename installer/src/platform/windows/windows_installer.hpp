/* Copyright (C) 2025 Pedro Henrique / phkaiser13
* File: windows_installer.hpp
* This header defines the fully integrated WindowsInstaller class. It now depends on the
* ConfigManager for metadata and the ApiManager for dynamic release fetching, making it
* a truly data-driven installation engine.
* SPDX-License-Identifier: Apache-2.0
*/

#pragma once

#include "platform/iplatform_installer.hpp"
#include "platform/platform_detector.hpp"
#include "dependencies/dependency_manager.hpp"
#include "utils/api_manager.hpp" // Added ApiManager dependency
#include "utils/config_manager.hpp" // Added ConfigManager dependency

#include <memory>
#include <string>
#include <vector>


namespace phgit_installer::platform {

    class WindowsInstaller : public IPlatformInstaller {
    public:
        /**
         * @brief Constructs the WindowsInstaller engine.
         * @param info The detailed platform information for the Windows system.
         * @param dep_manager A reference to the dependency manager.
         * @param api_manager A shared pointer to the API manager for dynamic downloads.
         * @param config A shared pointer to the configuration manager for metadata.
         */
        explicit WindowsInstaller(
            platform::PlatformInfo info,
            const dependencies::DependencyManager& dep_manager,
            std::shared_ptr<utils::ApiManager> api_manager,
            std::shared_ptr<utils::ConfigManager> config
        );

        void run_installation() override;

    private:
        void perform_installation();
        bool ensure_git_is_installed();
        bool ensure_optional_dependencies(const std::string& target_dir);
        bool install_application_files(const std::string& install_path);
        void perform_system_integration(const std::string& install_path);
        void update_path_environment_variable(const std::string& directory_to_add, bool for_all_users);
        void create_registry_entries(const std::string& install_path, bool for_all_users);
        void create_start_menu_shortcuts(const std::string& install_path);

        /**
         * @brief Unzips a zip archive to a specified destination directory.
         * @param zip_path The full path to the .zip file.
         * @param dest_dir The directory to extract the contents into.
         * @return True on success, false on failure.
         */
        bool unzip_archive(const std::string& zip_path, const std::string& dest_dir);

        platform::PlatformInfo m_platform_info;
        const dependencies::DependencyManager& m_dep_manager;
        std::shared_ptr<utils::ApiManager> m_api_manager;
        std::shared_ptr<utils::ConfigManager> m_config;
    };

} // namespace phgit_installer::platform
