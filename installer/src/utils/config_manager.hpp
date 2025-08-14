/* Copyright (C) 2025 Pedro Henrique / phkaiser13
* File: config_manager.hpp
* This header defines the ConfigManager class, a crucial utility for creating a data-driven
* installer. It is responsible for parsing a central configuration file (e.g., config.json)
* that contains all necessary metadata for the application, package generation, and API
* endpoints. This approach avoids hardcoding values and allows for easy updates to the
* installer's behavior without recompiling the source code.
* SPDX-License-Identifier: Apache-2.0
*/

#pragma once

#include <string>
#include <vector>
#include <optional>
#include <memory>

namespace phgit_installer::utils {

    // --- Data Structures for Configuration ---

    struct PackageMetadata {
        std::string name;
        std::string version;
        std::string maintainer;
        std::string description;
        std::string homepage;
    };

    struct DependencyInfo {
        std::string name;
        std::string min_version;
        bool is_required;
        // This could be expanded to include API endpoints per dependency
    };

    struct ApiEndpoint {
        std::string name; // e.g., "git_for_windows", "terraform"
        std::string type; // e.g., "github", "hashicorp"
        std::string url_template; // e.g., "https://api.github.com/repos/{owner}/{repo}/releases/latest"
        std::string owner;
        std::string repo;
        std::string product_name; // for hashicorp
    };

    /**
     * @class ConfigManager
     * @brief Loads and provides access to installer configuration from a JSON file.
     */
    class ConfigManager {
    public:
        ConfigManager();
        ~ConfigManager();

        /**
         * @brief Loads configuration from a specified JSON file.
         * @param config_path The path to the configuration file.
         * @return True on successful parsing, false otherwise.
         */
        bool load_from_file(const std::string& config_path);

        /**
         * @brief Retrieves the core package metadata.
         * @return An optional containing the PackageMetadata struct.
         */
        std::optional<PackageMetadata> get_package_metadata() const;

        /**
         * @brief Retrieves the list of all defined dependencies.
         * @return A vector of DependencyInfo structs.
         */
        std::vector<DependencyInfo> get_dependencies() const;

        /**
         * @brief Retrieves the list of all defined API endpoints for release lookups.
         * @return A vector of ApiEndpoint structs.
         */
        std::vector<ApiEndpoint> get_api_endpoints() const;

    private:
        // PIMPL idiom to hide nlohmann/json dependency from the header.
        class ConfigManagerImpl;
        std::unique_ptr<ConfigManagerImpl> m_impl;
    };

} // namespace phgit_installer::utils
