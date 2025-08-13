/* Copyright (C) 2025 Pedro Henrique / phkaiser13
 * File: main.cpp
 * This is the main entry point for the installer_helper executable. Its role
 * is to orchestrate the entire dependency checking and installation process.
 * It initializes the necessary components (Downloader, DependencyManager),
 * defines the list of required dependencies, and then iterates through them,
 * ensuring each one is installed on the system. It returns a status code
 * to indicate success or failure to the calling process (e.g., an NSIS script
 * or a shell script).
 *
 * SPDX-License-Identifier: Apache-2.0 */

#include "downloader.hpp"
#include "dependency_manager.hpp"

#include <iostream>
#include <vector>
#include <memory>   // For std::make_shared
#include <cstdlib>  // For EXIT_SUCCESS, EXIT_FAILURE

int main() {
    std::cout << "--- phgit Dependency Installer Helper ---" << std::endl;

    try {
        // 1. Create the core components.
        // The Downloader is created first.
        auto downloader = std::make_shared<ph::Downloader>();
        // The DependencyManager is created with the Downloader.
        auto depManager = ph::DependencyManager(downloader);

        // 2. Define the list of all required dependencies.
        const std::vector<ph::Dependency> requiredDependencies = {
            ph::Dependency::Git,
            ph::Dependency::Terraform,
            ph::Dependency::Vault
        };

        bool allSucceeded = true;

        // 3. Process each dependency.
        for (const auto& dep : requiredDependencies) {
            if (!depManager.ensureInstalled(dep)) {
                // If any dependency fails, we mark the entire process as failed,
                // but we continue to try the others.
                allSucceeded = false;
                std::cerr << "-> Critical error processing " << depManager.getDisplayName(dep) << ". The installation may be incomplete." << std::endl;
            }
            std::cout << std::endl; // Add a blank line for better readability between dependencies.
        }

        // 4. Report final status and return the appropriate exit code.
        if (allSucceeded) {
            std::cout << "--- All dependencies are installed successfully. ---" << std::endl;
            return EXIT_SUCCESS; // Returns 0
        } else {
            std::cerr << "--- One or more dependencies failed to install. Please check the log above. ---" << std::endl;
            return EXIT_FAILURE; // Returns 1
        }

    } catch (const std::exception& e) {
        // Catch any critical initialization errors (e.g., libcurl failed).
        std::cerr << "A critical error occurred during initialization: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}