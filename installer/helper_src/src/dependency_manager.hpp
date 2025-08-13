/* Copyright (C) 2025 Pedro Henrique / phkaiser13
 * File: dependency_manager.hpp
 * This header defines the interface for the DependencyManager class. This class
 * is the core logic unit of the installer helper. It is responsible for
 * identifying required software dependencies (like Git, Terraform, Vault),
 * checking if they are present and valid on the user's system, and orchestrating
 * their download and installation if they are missing.
 *
 * SPDX-License-Identifier: Apache-2.0 */

#pragma once

#include "downloader.hpp"
#include <string>
#include <vector>
#include <memory> // For std::shared_ptr

namespace ph {

/**
 * @enum Dependency
 * @brief Enumerates the external dependencies that phgit requires.
 */
enum class Dependency {
    Git,
    Terraform,
    Vault
};

/**
 * @class DependencyManager
 * @brief Manages the verification and installation of external dependencies.
 */
class DependencyManager {
public:
    /**
     * @brief Constructor for the DependencyManager.
     *
     * @param downloader A shared pointer to a Downloader instance, which will
     *                   be used to fetch any missing dependency installers.
     */
    explicit DependencyManager(std::shared_ptr<Downloader> downloader);

    /**
     * @brief Checks if a specific dependency is installed and available in the system's PATH.
     *
     * @param dep The dependency to check.
     * @return true if the dependency is found, false otherwise.
     */
    bool isInstalled(Dependency dep) const;

    /**
     * @brief Ensures that a specific dependency is installed.
     *
     * If the dependency is not found, this method will attempt to download
     * and run its installer.
     *
     * @param dep The dependency to install.
     * @return true on success or if already installed, false on failure.
     */
    bool ensureInstalled(Dependency dep);

private:
    /**
     * @brief Gets the command-line name of the executable for a dependency.
     * @param dep The dependency.
     * @return A string with the executable name (e.g., "git").
     */
    std::string getExecutableName(Dependency dep) const;

    /**
     * @brief Gets a user-friendly display name for a dependency.
     * @param dep The dependency.
     * @return A string with the display name (e.g., "Git SCM").
     */
    std::string getDisplayName(Dependency dep) const;

    /**
     * @brief Fetches the correct download URL for a given dependency and platform.
     *
     * This is a placeholder for the logic that would query GitHub/HashiCorp APIs.
     *
     * @param dep The dependency for which to find a download URL.
     * @return A string containing the direct download URL. Returns empty on failure.
     */
    std::string getDownloadUrl(Dependency dep);

    /**
     * @brief Runs the downloaded installer executable.
     *
     * @param installerPath The path to the installer file.
     * @param dep The dependency being installed (to select silent flags).
     * @return true on success, false on failure.
     */
    bool runInstaller(const std::string& installerPath, Dependency dep);

    // A shared pointer to the downloader utility.
    // It's shared because the manager doesn't own the downloader, it just uses it.
    std::shared_ptr<Downloader> m_downloader;
};

} // namespace ph