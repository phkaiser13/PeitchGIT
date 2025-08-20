// Copyright (C) 2025 Pedro Henrique / phkaiser13
// File: dependency_manager.hpp
// SPDX-License-Identifier: Apache-2.0
//
// Header for DependencyManager: checks presence and minimum versions of
// external command-line tools used by the installer. Designed to be
// lightweight in compile-time by using forward-declarations where possible.
//
// Usage notes:
//  - The ConfigManager should provide a list of dependencies to check.
//    A compatible return type is: std::vector<phgit_installer::dependencies::DependencyRequirement>.
//  - The implementation (.cpp) performs filesystem/search and process execution.
//  - This header intentionally avoids heavy headers (e.g. <filesystem>) to keep compile times small.

#pragma once

#include <string>
#include <vector>
#include <optional>
#include <memory>
#include <mutex>
#include "platform/platform_detector.hpp"

namespace phgit_installer {
    namespace platform {
        // Forward-declare PlatformInfo. Define this in your platform headers.
        struct PlatformInfo;
    }

    namespace utils {
        // Forward-declare ConfigManager to avoid pulling heavy config headers here.
        class ConfigManager;
    }

    namespace dependencies {

        /**
         * @brief Minimal description of a dependency requirement.
         *
         * This type represents one dependency entry that the ConfigManager
         * may provide. If your ConfigManager returns a custom type, ensure
         * it's convertible or compatible with this struct.
         */
        struct DependencyRequirement {
            std::string name;           ///< Executable name to search (e.g. "git", "python")
            std::string min_version;    ///< Minimum required semantic version (e.g. "2.34.0")
            bool is_required = false;   ///< If true, installer treats this dependency as fatal when missing/outdated
        };

        /**
         * @brief Runtime status for a discovered dependency.
         *
         * All fields are public POD-style for convenient logging and serialization.
         */
        struct DependencyStatus {
            std::string name;             ///< name used to look for executable
            std::string minimum_version;  ///< required minimum version
            std::string found_path;       ///< full path to discovered executable (empty if not found)
            std::string found_version;    ///< parsed version string (empty if not parsed)
            bool is_found = false;        ///< true if executable was found on PATH
            bool is_required = false;     ///< mirrors requirement flag
            bool is_version_ok = false;   ///< true if found_version >= minimum_version
        };

        /**
         * @brief DependencyManager
         *
         * Responsible for:
         *  - Querying the ConfigManager for a list of dependencies to check.
         *  - Locating executables on PATH (cross-platform).
         *  - Running a version command and parsing version output.
         *  - Producing DependencyStatus results consumable by the rest of the installer.
         *
         * Thread-safety:
         *  - This class is safe for concurrent read access to results. Public mutation
         *    methods lock an internal mutex to protect state.
         */
        class DependencyManager {
        public:
            /**
             * @brief Construct a DependencyManager.
             * @param info Platform information (OS family, arch, etc.)
             * @param config Shared pointer to the ConfigManager instance used to retrieve dependency definitions.
             *
             * Note: we take ConfigManager as a shared_ptr so ownership is shared with the caller.
             */
            DependencyManager(platform::PlatformInfo info,
                              std::shared_ptr<utils::ConfigManager> config) noexcept;

            DependencyManager(const DependencyManager&) = delete;
            DependencyManager& operator=(const DependencyManager&) = delete;
            DependencyManager(DependencyManager&&) = delete;
            DependencyManager& operator=(DependencyManager&&) = delete;

            ~DependencyManager() = default;

            /**
             * @brief Perform checks for all dependencies defined by the ConfigManager.
             *
             * This method is the main entry point: it queries the config, checks each
             * dependency and stores the resulting statuses internally.
             */
            void check_all();

            /**
             * @brief Get the status of a dependency by name.
             * @param name Dependency executable name (case-sensitive match).
             * @return Optional containing DependencyStatus if found, std::nullopt otherwise.
             */
            std::optional<DependencyStatus> get_status(const std::string& name) const;

            /**
             * @brief Return a const reference to all collected statuses.
             * @note The returned reference remains valid until the next mutating call (e.g. check_all()).
             */
            const std::vector<DependencyStatus>& all_statuses() const noexcept;

            /**
             * @brief Are all required dependencies found and meet the minimum versions?
             * @return true if all required dependencies are satisfied.
             */
            bool are_core_dependencies_met() const;

        private:
            // Platform information (OS family, architecture, etc.)
            platform::PlatformInfo m_platform_info;

            // Config manager used to retrieve dependency definitions.
            std::shared_ptr<utils::ConfigManager> m_config;

            // Collected dependency statuses from the last run.
            std::vector<DependencyStatus> m_dependency_statuses;

            // Protects m_dependency_statuses and other mutable state.
            mutable std::mutex m_mutex;

            //
            // Internal helpers (implementation details in .cpp)
            //

            /**
             * @brief Search the system PATH for an executable named 'name'.
             * @param name File name without extension (e.g. "git").
             * @return full path to executable if found, std::nullopt otherwise.
             *
             * Implementation notes:
             *  - On Windows, consider PATHEXT and try common extensions (.exe, .cmd, .bat).
             *  - On POSIX, verify execute permission bits.
             */
            std::optional<std::string> find_executable_in_path(const std::string& name) const;

            /**
             * @brief Parse the most likely semantic version from arbitrary program output.
             * @param raw_output stdout/stderr collected from the tool.
             * @return version string like "1.2.3" or empty string on failure.
             *
             * Implementation should be resilient (allow optional 'v' prefix, X.Y or X.Y.Z).
             */
            std::string parse_version_from_output(const std::string& raw_output) const;

            /**
             * @brief Compare two version strings using numeric components.
             * @param v1 left operand (found version)
             * @param v2 right operand (minimum required)
             * @return  1 if v1 > v2, 0 if equal, -1 if v1 < v2
             *
             * Implementation should treat missing components as zero (e.g. "1.2" == "1.2.0")
             * and be safe against malformed segments (prefer non-throwing conversions).
             */
            int compare_versions(const std::string& v1, const std::string& v2) const;
        };

    } // namespace dependencies
} // namespace phgit_installer
