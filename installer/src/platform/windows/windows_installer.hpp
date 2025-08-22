/* Copyright (C) 2025 Pedro Henrique / phkaiser13
* File: windows_installer.hpp
* This header defines the WindowsInstaller class, which serves as a post-installation
* dependency checker. It is designed to be run by a primary installer like NSIS after
* file extraction. Its role is to interactively guide the user in installing required
* or optional third-party tools, not to perform system modifications itself.
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

    /**
     * @class WindowsInstaller
     * @brief A post-installation assistant for the Windows platform.
     *
     * This class checks for necessary and optional third-party dependencies after the
     * main application files have been installed by a package manager (e.g., NSIS, WiX).
     * It provides interactive prompts to help the user download and install any
     * missing components, but does not perform these actions automatically.
     */
    class WindowsInstaller : public IPlatformInstaller {
    public:
        /**
         * @brief Constructs the Windows post-installation assistant.
         * @param info The detailed platform information for the Windows system.
         * @param dep_manager A reference to the dependency manager for status checks.
         * @param api_manager A shared pointer to the API manager for fetching download page URLs.
         * @param config A shared pointer to the configuration manager.
         */
        explicit WindowsInstaller(
            platform::PlatformInfo info,
            const dependencies::DependencyManager& dep_manager,
            std::shared_ptr<utils::ApiManager> api_manager,
            std::shared_ptr<utils::ConfigManager> config
        );

        /**
         * @brief Runs the interactive dependency check process.
         * This is the main entry point for the assistant.
         */
        void run_installation() override;

    private:
        /**
         * @brief Informs the user that Git is a required dependency and asks if they
         * want to open the official download page.
         */
        void prompt_user_to_install_git();

        /**
         * @brief Informs the user that a dependency is optional and asks if they
         * want to open its official download page.
         * @param dependency_name The display name of the optional dependency (e.g., "Terraform").
         */
        void prompt_user_to_install_optional(const std::string& dependency_name);

        // Member variables holding the state and tools needed for checks.
        platform::PlatformInfo m_platform_info;
        const dependencies::DependencyManager& m_dep_manager;
        std::shared_ptr<utils::ApiManager> m_api_manager;
        std::shared_ptr<utils::ConfigManager> m_config;
    };

} // namespace phgit_installer::platform