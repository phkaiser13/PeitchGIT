/* Copyright (C) 2025 Pedro Henrique / phkaiser13
* File: dependency_manager.cpp
* This file implements the DependencyManager class. It has been refactored to be fully
* data-driven and robust. It retrieves the list of dependencies to check from the
* ConfigManager and uses the powerful ProcessExecutor utility to run version checks,
* capturing all output for accurate diagnostics. This removes all hardcoded values
* and brittle process logic from this component.
* SPDX-License-Identifier: Apache-2.0
*/


/*
* Correct includes for dependency_manager.hpp
*/

#pragma once

// For platform::PlatformInfo struct
#include "platform/platform_detector.hpp"
// For utils::ConfigManager
#include "utils/config_manager.hpp"

// For std::string and std::vector
#include <string>
#include <vector>
// For std::optional
#include <optional>
// For std::shared_ptr
#include <memory>

// NOTE: All platform-specific includes for process creation are now gone!
// This is a sign of successful abstraction.

namespace phgit_installer::dependencies {

    namespace fs = std::filesystem;

    // Constructor now accepts the ConfigManager
    DependencyManager::DependencyManager(platform::PlatformInfo info, std::shared_ptr<utils::ConfigManager> config)
        : m_platform_info(std::move(info)), m_config(std::move(config)) {
        spdlog::debug("DependencyManager initialized for OS: {}", m_platform_info.os_family);
    }

    // check_all is now data-driven
    void DependencyManager::check_all() {
        spdlog::info("Starting check for all external dependencies defined in configuration.");
        m_dependency_statuses.clear();

        auto dependencies_to_check = m_config->get_dependencies();
        if (dependencies_to_check.empty()) {
            spdlog::warn("No dependencies are defined in the configuration file.");
            return;
        }

        for (const auto& dep_info : dependencies_to_check) {
            check_dependency(dep_info.name, dep_info.min_version, dep_info.is_required);
        }
    }

    std::optional<DependencyStatus> DependencyManager::get_status(const std::string& name) const {
        for (const auto& status : m_dependency_statuses) {
            if (status.name == name) {
                return status;
            }
        }
        return std::nullopt;
    }

    bool DependencyManager::are_core_dependencies_met() const {
        for (const auto& status : m_dependency_statuses) {
            if (status.is_required && (!status.is_found || !status.is_version_ok)) {
                return false;
            }
        }
        return true;
    }

    void DependencyManager::check_dependency(const std::string& name, const std::string& min_version, bool is_required) {
        DependencyStatus status;
        status.name = name;
        status.minimum_version = min_version;
        status.is_required = is_required;

        spdlog::debug("Checking dependency: {}", name);

        auto executable_path = find_executable_in_path(name);
        if (!executable_path) {
            spdlog::warn("Dependency '{}' not found in PATH.", name);
            status.is_found = false;
            m_dependency_statuses.push_back(status);
            return;
        }

        status.is_found = true;
        status.found_path = *executable_path;
        spdlog::debug("Found '{}' at: {}", name, status.found_path);

        // --- REFACTORED SECTION ---
        // Use the robust ProcessExecutor instead of the old internal method.
        std::string command = "\"" + status.found_path + "\" --version";
        auto proc_result = ProcessExecutor::execute(command);

        if (proc_result.exit_code != 0) {
            spdlog::warn("Command '{}' failed with exit code {}. Stderr: {}", command, proc_result.exit_code, proc_result.std_err);
            m_dependency_statuses.push_back(status);
            return;
        }

        // Some tools (like git) print version to stdout, others to stderr. Check both.
        std::string raw_output = !proc_result.std_out.empty() ? proc_result.std_out : proc_result.std_err;
        // --- END REFACTORED SECTION ---

        status.found_version = parse_version_from_output(raw_output);
        if (status.found_version.empty()) {
            spdlog::warn("Could not parse version for '{}' from output: {}", name, raw_output);
            m_dependency_statuses.push_back(status);
            return;
        }

        spdlog::debug("Found version for '{}': {}", name, status.found_version);
        status.is_version_ok = (compare_versions(status.found_version, status.minimum_version) >= 0);

        if (status.is_version_ok) {
            spdlog::info("Dependency '{}' is OK (version {}, required >= {}).", name, status.found_version, status.minimum_version);
        } else {
            spdlog::warn("Dependency '{}' is outdated (version {}, required >= {}).", name, status.found_version, status.minimum_version);
        }

        m_dependency_statuses.push_back(status);
    }

    std::optional<std::string> DependencyManager::find_executable_in_path(const std::string& name) {
        // This function was already well-written and remains unchanged.
        const char* path_env = std::getenv("PATH");
        if (!path_env) return std::nullopt;
        std::string path_str(path_env);
        #if defined(_WIN32) || defined(_WIN64)
            const char delimiter = ';';
            const std::vector<std::string> extensions = { ".exe", ".cmd", ".bat" };
        #else
            const char delimiter = ':';
            const std::vector<std::string> extensions = { "" };
        #endif
        std::stringstream ss(path_str);
        std::string path_item;
        while (std::getline(ss, path_item, delimiter)) {
            if (path_item.empty()) continue;
            for (const auto& ext : extensions) {
                fs::path full_path = fs::path(path_item) / (name + ext);
                std::error_code ec;
                if (fs::exists(full_path, ec) && !fs::is_directory(full_path, ec) && !ec) {
                    // On POSIX, we should also check for execute permissions
                    #if !defined(_WIN32) && !defined(_WIN64)
                        if ((fs::status(full_path, ec).permissions() & (fs::perms::owner_exec | fs::perms::group_exec | fs::perms::others_exec)) == fs::perms::none) {
                            continue;
                        }
                    #endif
                    return full_path.string();
                }
            }
        }
        return std::nullopt;
    }

    // The get_output_from_command method has been completely removed.

    std::string DependencyManager::parse_version_from_output(const std::string& raw_output) {
        // This function remains unchanged.
        std::regex version_regex("(\\d+\\.\\d+(\\.\\d+)?)"); // Improved regex for X.Y or X.Y.Z
        std::smatch match;
        if (std::regex_search(raw_output, match, version_regex) && match.size() > 1) {
            return match[1].str();
        }
        return "";
    }

    int DependencyManager::compare_versions(const std::string& v1, const std::string& v2) {
        // This function remains unchanged.
        std::stringstream ss1(v1);
        std::stringstream ss2(v2);
        std::string segment;
        std::vector<int> v1_parts, v2_parts;
        while(std::getline(ss1, segment, '.')) v1_parts.push_back(std::stoi(segment));
        while(std::getline(ss2, segment, '.')) v2_parts.push_back(std::stoi(segment));
        size_t max_size = std::max(v1_parts.size(), v2_parts.size());
        v1_parts.resize(max_size, 0);
        v2_parts.resize(max_size, 0);
        for (size_t i = 0; i < max_size; ++i) {
            if (v1_parts[i] < v2_parts[i]) return -1;
            if (v1_parts[i] > v2_parts[i]) return 1;
        }
        return 0;
    }

} // namespace phgit_installer::dependencies
