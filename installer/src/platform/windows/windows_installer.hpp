/* Copyright (C) 2025 Pedro Henrique / phkaiser13
* File: windows_installer.hpp
 * Refactored to remove silent-install and system-integration responsibilities.
 * The class now acts as an interactive post-installation dependency checker.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "platform/iplatform_installer.hpp"
#include "platform/platform_detector.hpp"
#include "dependencies/dependency_manager.hpp"
#include "utils/api_manager.hpp"
#include "utils/config_manager.hpp"

#include <memory>
#include <string>

namespace phgit_installer::platform {

    class WindowsInstaller : public IPlatformInstaller {
    public:
        /**
         * @brief Constructs the WindowsInstaller engine.
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

        // The installer no longer performs downloads or system integration.
        // Instead it provides interactive prompts to the user when dependencies are missing.
        void prompt_user_to_install_git();
        void prompt_user_to_install_optional(const std::string& dependency_name);

        bool install_application_files(const std::string& install_path);

        platform::PlatformInfo m_platform_info;
        const dependencies::DependencyManager& m_dep_manager;
        std::shared_ptr<utils::ApiManager> m_api_manager;
        std::shared_ptr<utils::ConfigManager> m_config;
    };

} // namespace phgit_installer::platform
