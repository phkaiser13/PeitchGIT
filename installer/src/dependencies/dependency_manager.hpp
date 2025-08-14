/* Copyright (C) 2025 Pedro Henrique / phkaiser13
* File: dependency_manager.hpp
* This header defines the interface for the DependencyManager class. This class is now
* fully data-driven, reading its dependency requirements from a ConfigManager instance.
* It uses the ProcessExecutor utility to robustly check for the existence and version
* of external tools, providing a reliable foundation for the installer's logic.
* SPDX-License-Identifier: Apache-2.0
*/

#pragma once

#include "platform/platform_detector.hpp"
#include "utils/config_manager.hpp" // Added config manager dependency
#include <string>
#include <vector>
#include <optional>
#include <memory> // For std::shared_ptr

namespace phgit_installer::dependencies {

    struct DependencyStatus {
        std::string name;
        bool is_required = false;
        std::string minimum_version;
        bool is_found = false;
        bool is_version_ok = false;
        std::string found_path;
        std::string found_version;
    };

    class DependencyManager {
    public:
        /**
         * @brief Constructs a DependencyManager.
         * @param info The platform information, used to tailor dependency checks.
         * @param config A shared pointer to the application's configuration manager.
         */
        explicit DependencyManager(platform::PlatformInfo info, std::shared_ptr<utils::ConfigManager> config);

        void check_all();
        std::optional<DependencyStatus> get_status(const std::string& name) const;
        bool are_core_dependencies_met() const;

    private:
        void check_dependency(const std::string& name, const std::string& min_version, bool is_required);
        std::optional<std::string> find_executable_in_path(const std::string& name);
        std::string parse_version_from_output(const std::string& raw_output);
        int compare_versions(const std::string& version1, const std::string& version2);

        platform::PlatformInfo m_platform_info;
        std::shared_ptr<utils::ConfigManager> m_config; // Added config manager member
        std::vector<DependencyStatus> m_dependency_statuses;
    };

} // namespace phgit_installer::dependencies
