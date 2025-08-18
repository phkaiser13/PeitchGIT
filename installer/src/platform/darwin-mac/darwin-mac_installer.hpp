/* Copyright (C) 2025 Pedro Henrique / phkaiser13
* File: darwin-mac_installer.hpp
* This header defines the fully integrated MacosInstaller class. It leverages Homebrew
* as its primary strategy and includes a fully functional fallback to a native .pkg
* installer, using the ApiManager and Downloader to create a complete, end-to-end
* installation experience on macOS.
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
#include <vector>

namespace phgit_installer::platform {

    class MacosInstaller : public IPlatformInstaller {
    public:
        /**
         * @brief Constructs the MacosInstaller engine.
         * @param info The detailed platform information for the macOS system.
         * @param dep_manager A reference to the dependency manager.
         * @param api_manager A shared pointer to the API manager for dynamic downloads.
         * @param config A shared pointer to the configuration manager for metadata.
         */
        explicit MacosInstaller(
            platform::PlatformInfo info,
            const dependencies::DependencyManager& dep_manager,
            std::shared_ptr<utils::ApiManager> api_manager,
            std::shared_ptr<utils::ConfigManager> config
        );

        void run_installation() override;

    private:
        bool is_homebrew_available();
        void dispatch_installation_strategy();
        void install_using_homebrew();
        void install_from_pkg();
        void perform_manual_installation();
        void ensure_dependencies_with_brew();
        void prompt_for_command_line_tools();

        platform::PlatformInfo m_platform_info;
        const dependencies::DependencyManager& m_dep_manager;
        std::shared_ptr<utils::ApiManager> m_api_manager;
        std::shared_ptr<utils::ConfigManager> m_config;
        bool m_homebrew_is_available;
    };

} // namespace phgit_installer::platform
