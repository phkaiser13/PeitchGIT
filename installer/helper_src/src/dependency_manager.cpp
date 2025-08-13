/* Copyright (C) 2025 Pedro Henrique / phkaiser13
 * File: dependency_manager.cpp
 * This file implements the logic for the DependencyManager class. It handles
 * the platform-specific details of checking for installed software, fetching
 * the correct download URLs for the current OS and architecture, and executing
 * the installers with the appropriate silent flags.
 *
 * SPDX-License-Identifier: Apache-2.0 */

#include "dependency_manager.hpp"
#include <iostream>
#include <cstdlib>    // For system()
#include <filesystem> // For std::filesystem::temp_directory_path, C++17

// Platform-specific header for process creation if needed later,
// but system() is sufficient for now.
#ifdef _WIN32
#include <windows.h>
#endif

namespace ph {

DependencyManager::DependencyManager(std::shared_ptr<Downloader> downloader)
    : m_downloader(std::move(downloader)) {
    if (!m_downloader) {
        throw std::invalid_argument("Downloader cannot be null.");
    }
}

bool DependencyManager::isInstalled(Dependency dep) const {
    std::string command;
    std::string executable = getExecutableName(dep);

#ifdef _WIN32
    // 'where' is a built-in command on modern Windows to find executables in PATH.
    // Redirect output to NUL to keep the check silent.
    command = "where " + executable + " > nul 2>&1";
#else
    // 'command -v' is the POSIX standard way to check if a command exists.
    // It's more reliable than 'which'. Redirect to /dev/null.
    command = "command -v " + executable + " > /dev/null 2>&1";
#endif

    // system() returns 0 on success.
    return (system(command.c_str()) == 0);
}

bool DependencyManager::ensureInstalled(Dependency dep) {
    std::cout << "Checking for " << getDisplayName(dep) << "..." << std::endl;

    if (isInstalled(dep)) {
        std::cout << "-> " << getDisplayName(dep) << " is already installed." << std::endl;
        return true;
    }

    std::cout << "-> " << getDisplayName(dep) << " not found. Starting installation process." << std::endl;

    // 1. Get the correct download URL
    std::string url = getDownloadUrl(dep);
    if (url.empty()) {
        std::cerr << "Error: Could not determine download URL for " << getDisplayName(dep)
                  << " on this platform." << std::endl;
        return false;
    }
    std::cout << "   - Found download URL: " << url << std::endl;

    // 2. Determine a temporary path for the installer
    std::filesystem::path tempPath = std::filesystem::temp_directory_path();
    std::filesystem::path installerPath = tempPath / (getExecutableName(dep) + "_installer.tmp");

    // 3. Download the installer
    if (!m_downloader->downloadFile(url, installerPath.string(), getDisplayName(dep))) {
        std::cerr << "Error: Failed to download " << getDisplayName(dep) << "." << std::endl;
        return false;
    }
    std::cout << "   - Download complete. Installer saved to: " << installerPath.string() << std::endl;

    // 4. Run the installer
    if (!runInstaller(installerPath.string(), dep)) {
        std::cerr << "Error: Failed to install " << getDisplayName(dep) << "." << std::endl;
        return false;
    }

    // 5. Final verification
    std::cout << "   - Verifying installation..." << std::endl;
    if (isInstalled(dep)) {
        std::cout << "-> Successfully installed " << getDisplayName(dep) << "!" << std::endl;
        return true;
    } else {
        std::cerr << "Error: Installation of " << getDisplayName(dep) << " completed, but it could not be found in the PATH. You may need to restart your terminal or PC." << std::endl;
        return false;
    }
}

std::string DependencyManager::getExecutableName(Dependency dep) const {
    switch (dep) {
        case Dependency::Git:       return "git";
        case Dependency::Terraform: return "terraform";
        case Dependency::Vault:     return "vault";
    }
    return ""; // Should not happen
}

std::string DependencyManager::getDisplayName(Dependency dep) const {
    switch (dep) {
        case Dependency::Git:       return "Git SCM";
        case Dependency::Terraform: return "HashiCorp Terraform";
        case Dependency::Vault:     return "HashiCorp Vault";
    }
    return ""; // Should not happen
}

std::string DependencyManager::getDownloadUrl(Dependency dep) {
    // NOTE: This is a simplified implementation with hardcoded URLs.
    // A production-ready version should query the provider's API (GitHub, HashiCorp)
    // to get the latest version and checksums dynamically.

#if defined(_WIN32) && defined(_M_X64)
    // Windows x64
    switch (dep) {
        case Dependency::Git:       return "https://github.com/git-for-windows/git/releases/download/v2.45.1.windows.1/Git-2.45.1-64-bit.exe";
        case Dependency::Terraform: return "https://releases.hashicorp.com/terraform/1.8.2/terraform_1.8.2_windows_amd64.zip";
        case Dependency::Vault:     return "https://releases.hashicorp.com/vault/1.16.2/vault_1.16.2_windows_amd64.zip";
    }
#elif defined(__APPLE__) && defined(__x86_64__)
    // macOS x64 (Intel)
    // For macOS, we typically download a zip/binary, not a .pkg installer.
    // The user is often expected to use Homebrew, but we provide a fallback.
    switch (dep) {
        case Dependency::Git:       return "https://sourceforge.net/projects/git-osx-installer/files/git-2.44.0-intel-universal-mavericks.dmg/download"; // DMG is more complex, a direct binary or zip is better if available.
        case Dependency::Terraform: return "https://releases.hashicorp.com/terraform/1.8.2/terraform_1.8.2_darwin_amd64.zip";
        case Dependency::Vault:     return "https://releases.hashicorp.com/vault/1.16.2/vault_1.16.2_darwin_amd64.zip";
    }
#elif defined(__APPLE__) && defined(__aarch64__)
    // macOS ARM64 (Apple Silicon)
    switch (dep) {
        case Dependency::Git:       return "https://sourceforge.net/projects/git-osx-installer/files/git-2.44.0-intel-universal-mavericks.dmg/download"; // Universal binary
        case Dependency::Terraform: return "https://releases.hashicorp.com/terraform/1.8.2/terraform_1.8.2_darwin_arm64.zip";
        case Dependency::Vault:     return "https://releases.hashicorp.com/vault/1.16.2/vault_1.16.2_darwin_arm64.zip";
    }
#elif defined(__linux__) && defined(__x86_64__)
    // Linux x64
    // For Linux, we download the binary zip. The installer script will handle placing it.
    switch (dep) {
        case Dependency::Git:       return ""; // On Linux, it's strongly recommended to use the system package manager (apt, yum, etc.). We won't download it.
        case Dependency::Terraform: return "https://releases.hashicorp.com/terraform/1.8.2/terraform_1.8.2_linux_amd64.zip";
        case Dependency::Vault:     return "https://releases.hashicorp.com/vault/1.16.2/vault_1.16.2_linux_amd64.zip";
    }
#endif
    return ""; // Platform not supported or dependency not available
}

bool DependencyManager::runInstaller(const std::string& installerPath, Dependency dep) {
    std::cout << "   - Running installer..." << std::endl;
    std::string command;

    switch (dep) {
        case Dependency::Git:
#ifdef _WIN32
            // Use /VERYSILENT for a non-interactive installation.
            // The start /wait command ensures our program waits for the installer to finish.
            command = "start /wait \"\" \"" + installerPath + "\" /VERYSILENT /NORESTART";
#else
            // On non-Windows, we'd handle a .dmg or prompt for package manager.
            // For this example, we assume it's not a scenario we handle automatically.
            std::cout << "   - Please install Git manually using your system's package manager (e.g., 'sudo apt install git' or 'brew install git')." << std::endl;
            return true; // Return true to not block other installations.
#endif
            break;

        case Dependency::Terraform:
        case Dependency::Vault:
            // For these, the "installer" is just a zip file.
            // A real implementation needs to:
            // 1. Unzip the file.
            // 2. Find the executable inside.
            // 3. Move it to a directory in the system's PATH (e.g., /usr/local/bin or the main app's install dir).
            // This logic is complex and requires a zip library.
            // We will STUB this for now.
            std::cout << "   - NOTE: Automatic installation of " << getDisplayName(dep)
                      << " from a .zip is not yet implemented." << std::endl;
            std::cout << "   - Please manually unzip " << installerPath << " and move the executable to a directory in your PATH." << std::endl;
            // We return true so the main installer can proceed with other tasks.
            // In a real scenario, you'd return the result of the unzip/move operation.
            return true;
    }

    int result = system(command.c_str());
    return (result == 0);
}

} // namespace ph