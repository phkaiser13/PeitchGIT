/* Copyright (C) 2025 Pedro Henrique / phkaiser13
* File: api_manager.hpp
* This header defines the ApiManager class, which is responsible for interacting with
* external release APIs like GitHub and HashiCorp. It uses the ConfigManager to get
* API endpoint information and the Downloader to fetch API responses. Its primary role
* is to resolve a product name (e.g., "terraform") and platform details into a concrete
* download URL and checksum for the latest available artifact, abstracting away the
* differences between various API providers.
* SPDX-License-Identifier: Apache-2.0
*/

#pragma once

#include "utils/config_manager.hpp"
#include "platform/platform_detector.hpp"

#include <string>
#include <optional>
#include <memory>

namespace phgit_installer::utils {

    /**
     * @struct ReleaseAsset
     * @brief Represents a downloadable asset from a release API.
     */
    struct ReleaseAsset {
        std::string product_name;
        std::string version;
        std::string download_url;
        std::string checksum; // The SHA256 hash of the asset
        std::string checksum_type; // e.g., "sha256"
    };

    /**
     * @class ApiManager
     * @brief A client for fetching release information from various APIs.
     */
    class ApiManager {
    public:
        /**
         * @brief Constructs the ApiManager.
         * @param config A shared pointer to a pre-loaded ConfigManager instance.
         */
        explicit ApiManager(std::shared_ptr<ConfigManager> config);
        ~ApiManager();

        /**
         * @brief Fetches the latest release asset for a given product and platform.
         * This is the main public method of the class.
         * @param product_name The name of the product to look for (e.g., "terraform").
         * @param platform_info The detected platform details to match against assets.
         * @return An optional containing the ReleaseAsset if a suitable one is found.
         */
        std::optional<ReleaseAsset> fetch_latest_asset(const std::string& product_name, const platform::PlatformInfo& platform_info);

    private:
        /**
         * @brief Handles API requests for GitHub releases.
         * @param endpoint The API endpoint configuration.
         * @param platform_info The platform details for asset matching.
         * @return An optional ReleaseAsset.
         */
        std::optional<ReleaseAsset> handle_github_api(const ApiEndpoint& endpoint, const platform::PlatformInfo& platform_info);

        /**
         * @brief Handles API requests for HashiCorp releases.
         * @param endpoint The API endpoint configuration.
         * @param platform_info The platform details for asset matching.
         * @return An optional ReleaseAsset.
         */
        std::optional<ReleaseAsset> handle_hashicorp_api(const ApiEndpoint& endpoint, const platform::PlatformInfo& platform_info);

        /**
         * @brief Replaces placeholders in a URL template with actual values.
         * @param tpl The URL template string (e.g., "https://.../{owner}/{repo}").
         * @param endpoint The ApiEndpoint containing values for replacement.
         * @return The final, resolved URL.
         */
        std::string resolve_url_template(std::string tpl, const ApiEndpoint& endpoint);

        // PIMPL idiom to hide implementation details (like the Downloader instance).
        class ApiManagerImpl;
        std::unique_ptr<ApiManagerImpl> m_impl;
    };

} // namespace phgit_installer::utils
