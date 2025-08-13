/* Copyright (C) 2025 Pedro Henrique / phkaiser13
 * File: dependency_manager.cpp
 * This file implements the logic for the DependencyManager class. It handles
 * the platform-specific details of checking for installed software, dynamically
 * fetching download URLs from provider APIs (GitHub, HashiCorp), and executing
 * the installers with the appropriate silent flags.
 *
 * SPDX-License-Identifier: Apache-2.0 */

#include "dependency_manager.hpp"
#include "platform.hpp" // For platform detection

#include <iostream>
#include <cstdlib>
#include <filesystem>
#include <sstream>
#include <nlohmann/json.hpp> // For parsing API responses
#include <curl/curl.h>

// Use a shorter alias for the json library namespace.
using json = nlohmann::json;

namespace ph {

// Private helper function to perform an in-memory download of a URL's content.
// This is needed to get the JSON response from APIs as a string.
namespace {
    size_t stringWriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
        static_cast<std::string*>(userp)->append(static_cast<char*>(contents), size * nmemb);
        return size * nmemb;
    }

    std::string fetchUrlToString(const std::string& url) {
        CURL* curl = curl_easy_init();
        if (!curl) {
            return "";
        }

        std::string response_string;
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "phgit-installer/1.0");
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, stringWriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);

        CURLcode res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);

        if (res != CURLE_OK) {
            std::cerr << "Error: Failed to fetch URL " << url << ": " << curl_easy_strerror(res) << std::endl;
            return "";
        }
        return response_string;
    }
}

DependencyManager::DependencyManager(std::shared_ptr<Downloader> downloader)
    : m_downloader(std::move(downloader)) {
    if (!m_downloader) {
        throw std::invalid_argument("Downloader cannot be null.");
    }
}

bool DependencyManager::isInstalled(Dependency dep) const {
    std::string command;
    std::string executable = getExecutableName(dep);

    if (platform::currentOS == platform::OperatingSystem::Windows) {
        command = "where " + executable + " > nul 2>&1";
    } else {
        command = "command -v " + executable + " > /dev/null 2>&1";
    }
    return (system(command.c_str()) == 0);
}

bool DependencyManager::ensureInstalled(Dependency dep) {
    std::cout << "Checking for " << getDisplayName(dep) << "..." << std::endl;
    if (isInstalled(dep)) {
        std::cout << "-> " << getDisplayName(dep) << " is already installed." << std::endl;
        return true;
    }

    std::cout << "-> " << getDisplayName(dep) << " not found. Starting installation process." << std::endl;
    std::string url = getDownloadUrl(dep);
    if (url.empty()) {
        std::cerr << "Error: Could not find a valid download URL for " << getDisplayName(dep) << " for this system." << std::endl;
        return false;
    }
    std::cout << "   - Found dynamic download URL: " << url << std::endl;

    std::filesystem::path installerPath = std::filesystem::temp_directory_path() / (getExecutableName(dep) + "_installer.tmp");
    if (!m_downloader->downloadFile(url, installerPath.string(), getDisplayName(dep))) {
        std::cerr << "Error: Failed to download " << getDisplayName(dep) << "." << std::endl;
        return false;
    }
    std::cout << "   - Download complete." << std::endl;

    if (!runInstaller(installerPath.string(), dep)) {
        std::cerr << "Error: Failed to install " << getDisplayName(dep) << "." << std::endl;
        return false;
    }

    if (isInstalled(dep)) {
        std::cout << "-> Successfully installed " << getDisplayName(dep) << "!" << std::endl;
        return true;
    } else {
        std::cerr << "Error: Installation of " << getDisplayName(dep) << " finished, but it could not be found in the PATH." << std::endl;
        return false;
    }
}

std::string DependencyManager::getExecutableName(Dependency dep) const {
    switch (dep) {
        case Dependency::Git: return "git";
        case Dependency::Terraform: return "terraform";
        case Dependency::Vault: return "vault";
    }
    return "";
}

std::string DependencyManager::getDisplayName(Dependency dep) const {
    switch (dep) {
        case Dependency::Git: return "Git SCM";
        case Dependency::Terraform: return "HashiCorp Terraform";
        case Dependency::Vault: return "HashiCorp Vault";
    }
    return "";
}

std::string DependencyManager::getDownloadUrl(Dependency dep) {
    // On Linux, always prefer the system package manager for Git.
    if (dep == Dependency::Git && platform::currentOS == platform::OperatingSystem::Linux) {
        std::cerr << "-> On Linux, Git should be installed via the system package manager (e.g., 'sudo apt install git')." << std::endl;
        return ""; // Returning empty will cause a controlled failure message.
    }

    std::string apiUrl;
    switch (dep) {
        case Dependency::Git:
            apiUrl = "https://api.github.com/repos/git-for-windows/git/releases/latest";
            break;
        case Dependency::Terraform:
            apiUrl = "https://api.releases.hashicorp.com/v1/releases/terraform/latest";
            break;
        case Dependency::Vault:
            apiUrl = "https://api.releases.hashicorp.com/v1/releases/vault/latest";
            break;
    }

    std::cout << "   - Querying API: " << apiUrl << std::endl;
    std::string jsonResponse = fetchUrlToString(apiUrl);
    if (jsonResponse.empty()) return "";

    try {
        json data = json::parse(jsonResponse);
        if (dep == Dependency::Git) {
            // GitHub API parsing
            for (const auto& asset : data["assets"]) {
                std::string assetName = asset["name"];
                if (platform::currentOS == platform::OperatingSystem::Windows && platform::currentArch == platform::Architecture::x64 && assetName.find("64-bit.exe") != std::string::npos) {
                    return asset["browser_download_url"];
                }
                // Add more conditions for other OS/Arch if Git needs to be downloaded there.
            }
        } else {
            // HashiCorp API parsing
            std::string os_str, arch_str;
            if (platform::currentOS == platform::OperatingSystem::Windows) os_str = "windows";
            else if (platform::currentOS == platform::OperatingSystem::MacOS) os_str = "darwin";
            else if (platform::currentOS == platform::OperatingSystem::Linux) os_str = "linux";

            if (platform::currentArch == platform::Architecture::x64) arch_str = "amd64";
            else if (platform::currentArch == platform::Architecture::ARM64) arch_str = "arm64";

            for (const auto& build : data["builds"]) {
                if (build["os"] == os_str && build["arch"] == arch_str) {
                    return build["url"];
                }
            }
        }
    } catch (const json::parse_error& e) {
        std::cerr << "Error: Failed to parse JSON response: " << e.what() << std::endl;
        return "";
    }
    return ""; // Return empty if no suitable asset was found
}

bool DependencyManager::runInstaller(const std::string& installerPath, Dependency dep) {
    std::cout << "   - Running installer..." << std::endl;
    std::string command;

    if (dep == Dependency::Git && platform::currentOS == platform::OperatingSystem::Windows) {
        command = "start /wait \"\" \"" + installerPath + "\" /VERYSILENT /NORESTART";
    } else {
        // For Terraform/Vault (zips) and Git on non-Windows, this requires more logic.
        // A real implementation needs to unzip and move the binary.
        std::cout << "   - NOTE: Automatic installation from a ZIP/DMG is not yet implemented." << std::endl;
        std::cout << "   - Please manually handle the file: " << installerPath << std::endl;
        return true; // Return true to not block other installations.
    }

    return (system(command.c_str()) == 0);
}

} // namespace ph