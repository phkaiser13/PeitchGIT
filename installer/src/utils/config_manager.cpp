/* Copyright (C) 2025 Pedro Henrique / phkaiser13
* File: config_manager.cpp
* This file implements the ConfigManager class. It uses the nlohmann/json library to
* parse the configuration file. The implementation is carefully designed to be robust,
* handling potential parsing errors and missing keys gracefully. It populates the C++
* data structures defined in the header, providing a type-safe way for the rest of the
* application to access configuration data.
* SPDX-License-Identifier: Apache-2.0
*/

#include "utils/config_manager.hpp"
#include "spdlog/spdlog.h"
#include "nlohmann/json.hpp" // This will be fetched by CMake

#include <fstream>

namespace phgit_installer::utils {

    using json = nlohmann::json;

    // --- PIMPL Implementation ---
    class ConfigManager::ConfigManagerImpl {
    public:
        json config_data;
    };

    // --- ConfigManager Class Implementation ---
    ConfigManager::ConfigManager() : m_impl(std::make_unique<ConfigManagerImpl>()) {}

    ConfigManager::~ConfigManager() = default;

    bool ConfigManager::load_from_file(const std::string& config_path) {
        std::ifstream config_stream(config_path);
        if (!config_stream.is_open()) {
            spdlog::critical("Failed to open configuration file: {}", config_path);
            return false;
        }

        try {
            m_impl->config_data = json::parse(config_stream);
            spdlog::info("Successfully loaded configuration from {}", config_path);
            return true;
        } catch (const json::parse_error& e) {
            spdlog::critical("Failed to parse configuration file: {}. Error: {}", config_path, e.what());
            return false;
        }
    }

    std::optional<PackageMetadata> ConfigManager::get_package_metadata() const {
        if (!m_impl->config_data.contains("package_metadata")) {
            return std::nullopt;
        }

        try {
            const auto& meta = m_impl->config_data["package_metadata"];
            PackageMetadata data;
            data.name = meta.value("name", "phgit");
            data.version = meta.value("version", "0.0.0");
            data.maintainer = meta.value("maintainer", "");
            data.description = meta.value("description", "");
            data.homepage = meta.value("homepage", "");
            return data;
        } catch (const json::exception& e) {
            spdlog::error("Error accessing package_metadata: {}", e.what());
            return std::nullopt;
        }
    }

    std::vector<DependencyInfo> ConfigManager::get_dependencies() const {
        std::vector<DependencyInfo> deps;
        if (!m_impl->config_data.contains("dependencies")) {
            return deps;
        }

        try {
            for (const auto& dep_json : m_impl->config_data["dependencies"]) {
                DependencyInfo info;
                info.name = dep_json.value("name", "");
                info.min_version = dep_json.value("min_version", "0.0.0");
                info.is_required = dep_json.value("is_required", false);
                if (!info.name.empty()) {
                    deps.push_back(info);
                }
            }
        } catch (const json::exception& e) {
            spdlog::error("Error accessing dependencies: {}", e.what());
        }
        return deps;
    }

    std::vector<ApiEndpoint> ConfigManager::get_api_endpoints() const {
        std::vector<ApiEndpoint> endpoints;
        if (!m_impl->config_data.contains("api_endpoints")) {
            return endpoints;
        }

        try {
            for (const auto& api_json : m_impl->config_data["api_endpoints"]) {
                ApiEndpoint endpoint;
                endpoint.name = api_json.value("name", "");
                endpoint.type = api_json.value("type", "");
                endpoint.url_template = api_json.value("url_template", "");
                endpoint.owner = api_json.value("owner", "");
                endpoint.repo = api_json.value("repo", "");
                endpoint.product_name = api_json.value("product_name", "");
                if (!endpoint.name.empty()) {
                    endpoints.push_back(endpoint);
                }
            }
        } catch (const json::exception& e) {
            spdlog::error("Error accessing api_endpoints: {}", e.what());
        }
        return endpoints;
    }

} // namespace phgit_installer::utils
