/* Copyright (C) 2025 Pedro Henrique / phkaiser13
* File: api_manager.cpp
* This file implements the ApiManager class. It contains the logic to fetch and parse
* JSON responses from various release APIs. It uses the Downloader to get the raw data,
* nlohmann/json for parsing, and then applies platform-specific filtering logic to find
* the correct downloadable asset. The implementation is designed to handle the distinct
* response schemas of different providers like GitHub and HashiCorp, providing a unified
* interface to the rest of the application.
* SPDX-License-Identifier: Apache-2.0
*/

#include "utils/api_manager.hpp"
#include "utils/downloader.hpp"
#include "nlohmann/json.hpp"
#include "spdlog/spdlog.h"

#include <filesystem>
#include <fstream>
#include <regex>

namespace phgit_installer::utils {

    using json = nlohmann::json;
    namespace fs = std::filesystem;

    // --- PIMPL Implementation ---
    class ApiManager::ApiManagerImpl {
    public:
        std::shared_ptr<ConfigManager> config;
        Downloader downloader;

        explicit ApiManagerImpl(std::shared_ptr<ConfigManager> cfg) : config(std::move(cfg)) {
            downloader.set_user_agent("phgit-installer/1.0");
            downloader.set_timeout(60); // 60-second timeout for API calls
        }

        // Helper to download API response content into a string
        std::optional<std::string> download_api_response(const std::string& url) {
            char temp_path_cstr[L_tmpnam];
            if (tmpnam(temp_path_cstr) == NULL) {
                spdlog::error("Could not create a temporary file name for API response.");
                return std::nullopt;
            }
            std::string temp_file_path = temp_path_cstr;

            if (!downloader.download_file(url, temp_file_path)) {
                spdlog::error("Failed to download API response from {}", url);
                return std::nullopt;
            }

            std::ifstream file(temp_file_path);
            if (!file.is_open()) {
                spdlog::error("Could not open temporary API response file: {}", temp_file_path);
                remove(temp_file_path.c_str());
                return std::nullopt;
            }

            std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
            file.close();
            remove(temp_file_path.c_str());
            return content;
        }
    };

    // --- ApiManager Class Implementation ---
    ApiManager::ApiManager(std::shared_ptr<ConfigManager> config)
        : m_impl(std::make_unique<ApiManagerImpl>(std::move(config))) {}

    ApiManager::~ApiManager() = default;

    std::string ApiManager::resolve_url_template(std::string tpl, const ApiEndpoint& endpoint) {
        // This is a simple replacement. A more complex system could use a map.
        size_t pos;
        while ((pos = tpl.find("{owner}")) != std::string::npos) tpl.replace(pos, 7, endpoint.owner);
        while ((pos = tpl.find("{repo}")) != std::string::npos) tpl.replace(pos, 6, endpoint.repo);
        while ((pos = tpl.find("{product_name}")) != std::string::npos) tpl.replace(pos, 14, endpoint.product_name);
        return tpl;
    }

    std::optional<ReleaseAsset> ApiManager::fetch_latest_asset(const std::string& product_name, const platform::PlatformInfo& platform_info) {
        auto endpoints = m_impl->config->get_api_endpoints();
        auto it = std::find_if(endpoints.begin(), endpoints.end(),
            [&](const ApiEndpoint& ep) { return ep.name == product_name; });

        if (it == endpoints.end()) {
            spdlog::error("No API endpoint configured for product: {}", product_name);
            return std::nullopt;
        }

        const ApiEndpoint& endpoint = *it;
        spdlog::info("Found API endpoint '{}' for product '{}'", endpoint.type, product_name);

        if (endpoint.type == "github") {
            return handle_github_api(endpoint, platform_info);
        }
        if (endpoint.type == "hashicorp") {
            return handle_hashicorp_api(endpoint, platform_info);
        }

        spdlog::error("Unsupported API type '{}' for product '{}'", endpoint.type, product_name);
        return std::nullopt;
    }

    std::optional<ReleaseAsset> ApiManager::handle_github_api(const ApiEndpoint& endpoint, const platform::PlatformInfo& platform_info) {
        std::string url = resolve_url_template(endpoint.url_template, endpoint);
        auto response_str = m_impl->download_api_response(url);
        if (!response_str) return std::nullopt;

        try {
            json data = json::parse(*response_str);
            std::string version = data.value("tag_name", "unknown");
            
            for (const auto& asset : data["assets"]) {
                std::string asset_name = asset.value("name", "");
                // Simple matching logic, can be improved with regex
                bool os_match = (asset_name.find(platform_info.os_family) != std::string::npos);
                bool arch_match = (asset_name.find(platform_info.architecture) != std::string::npos);

                if (os_match && arch_match) {
                    ReleaseAsset result;
                    result.product_name = endpoint.name;
                    result.version = version;
                    result.download_url = asset.value("browser_download_url", "");
                    // GitHub API doesn't provide checksums directly in the asset list.
                    // A more complex implementation would need to find and parse a checksum file.
                    result.checksum = ""; 
                    result.checksum_type = "sha256";
                    spdlog::info("Found matching GitHub asset: {}", asset_name);
                    return result;
                }
            }
        } catch (const json::exception& e) {
            spdlog::error("Failed to parse GitHub API response: {}", e.what());
        }

        spdlog::warn("No matching asset found for {} on {}/{}", endpoint.name, platform_info.os_family, platform_info.architecture);
        return std::nullopt;
    }

    std::optional<ReleaseAsset> ApiManager::handle_hashicorp_api(const ApiEndpoint& endpoint, const platform::PlatformInfo& platform_info) {
        std::string url = resolve_url_template(endpoint.url_template, endpoint);
        auto response_str = m_impl->download_api_response(url);
        if (!response_str) return std::nullopt;

        try {
            json data = json::parse(*response_str);
            std::string version = data.value("version", "unknown");
            
            // Map our platform names to HashiCorp's names
            std::string hc_os = platform_info.os_family;
            if (hc_os == "macos") hc_os = "darwin";
            std::string hc_arch = platform_info.architecture;
            if (hc_arch == "x86_64") hc_arch = "amd64";
            if (hc_arch == "aarch64") hc_arch = "arm64";

            for (const auto& build : data["builds"]) {
                if (build.value("os", "") == hc_os && build.value("arch", "") == hc_arch) {
                    ReleaseAsset result;
                    result.product_name = endpoint.name;
                    result.version = version;
                    result.download_url = build.value("url", "");
                    
                    // HashiCorp provides a checksums file, let's find the right hash
                    auto shasums_url = data.value("shasums_url", "");
                    auto shasums_content = m_impl->download_api_response(shasums_url);
                    if (shasums_content) {
                        std::string asset_filename = fs::path(result.download_url).filename().string();
                        std::stringstream ss(*shasums_content);
                        std::string line;
                        while (std::getline(ss, line)) {
                            if (line.find(asset_filename) != std::string::npos) {
                                result.checksum = std::regex_replace(line, std::regex("\\s+.*"), "");
                                break;
                            }
                        }
                    }
                    result.checksum_type = "sha256";
                    spdlog::info("Found matching HashiCorp build: {}", result.download_url);
                    return result;
                }
            }
        } catch (const json::exception& e) {
            spdlog::error("Failed to parse HashiCorp API response: {}", e.what());
        }

        spdlog::warn("No matching build found for {} on {}/{}", endpoint.name, platform_info.os_family, platform_info.architecture);
        return std::nullopt;
    }

} // namespace phgit_installer::utils
