/* Copyright (C) 2025 Pedro Henrique / phkaiser13
* File: dependency_manager.h
* This header defines the interface for the DependencyManager class. This class is
* responsible for detecting required and optional external dependencies for phgit, such as
* Git, Terraform, and Vault. It provides functionality to check for the existence of
* these tools, verify that their versions meet the minimum requirements, and report
* their status. This abstraction is key to ensuring the host environment is correctly
* prepared before the main application is installed.
* SPDX-License-Identifier: Apache-2.0
*/

#pragma once

#include "platform/platform_detector.h" // Needs PlatformInfo for context
#include <string>
#include <vector>
#include <optional>

namespace phgit_installer::dependencies {

    /**
     * @struct DependencyStatus
     * @brief Holds the results of a check for a single external dependency.
     */
    struct DependencyStatus {
        std::string name;               // The name of the dependency, e.g., "git"
        bool is_required = false;       // Is this dependency mandatory for phgit to run?
        std::string minimum_version;    // The minimum required version string, e.g., "2.20.0"
        
        // --- Status Fields (populated by DependencyManager) ---
        bool is_found = false;          // Was the executable found in the system's PATH?
        bool is_version_ok = false;     // Does the found version meet the minimum requirement?
        std::string found_path;         // The full path to the found executable.
        std::string found_version;      // The version string reported by the executable.
    };

    /**
     * @class DependencyManager
     * @brief Manages the detection and validation of all external dependencies.
     */
    class DependencyManager {
    public:
        /**
         * @brief Constructs a DependencyManager.
         * @param info The platform information, used to tailor dependency checks.
         */
        explicit DependencyManager(platform::PlatformInfo info);

        /**
         * @brief Runs checks for all configured dependencies (git, terraform, vault).
         */
        void check_all();

        /**
         * @brief Retrieves the status of a specific dependency by name.
         * @param name The name of the dependency (e.g., "git").
         * @return An optional containing the DependencyStatus if found, otherwise std::nullopt.
         */
        std::optional<DependencyStatus> get_status(const std::string& name) const;

        /**
         * @brief Checks if all *required* dependencies have been found and are valid.
         * @return True if all core requirements are met, false otherwise.
         */
        bool are_core_dependencies_met() const;

    private:
        /**
         * @brief A generic function to check a single dependency.
         * @param name The dependency's name.
         * @param min_version The minimum required version.
         * @param is_required Whether the dependency is mandatory.
         */
        void check_dependency(const std::string& name, const std::string& min_version, bool is_required);

        /**
         * @brief Searches for an executable in the system's PATH environment variable.
         * @param name The name of the executable (e.g., "git" or "git.exe").
         * @return An optional containing the full path if found, otherwise std::nullopt.
         */
        std::optional<std::string> find_executable_in_path(const std::string& name);

        /**
         * @brief Executes a command (like `git --version`) and captures its standard output.
         * @param command The full command to execute.
         * @return An optional containing the first line of output, otherwise std::nullopt.
         */
        std::optional<std::string> get_output_from_command(const std::string& command);

        /**
         * @brief Parses a version string (e.g., "git version 2.34.1") to extract the version number.
         * @param raw_output The raw string from the command's output.
         * @return The extracted version number (e.g., "2.34.1").
         */
        std::string parse_version_from_output(const std::string& raw_output);

        /**
         * @brief Compares two semantic version strings.
         * @param version1 A version string (e.g., "2.34.1").
         * @param version2 A version string (e.g., "2.20.0").
         * @return -1 if version1 < version2, 0 if version1 == version2, 1 if version1 > version2.
         */
        int compare_versions(const std::string& version1, const std::string& version2);

        platform::PlatformInfo m_platform_info;
        std::vector<DependencyStatus> m_dependency_statuses;
    };

} // namespace phgit_installer::dependencies
