/* Copyright (C) 2025 Pedro Henrique / phkaiser13
* File: main.cpp
* This is the data-driven entry point for the phgit installer engine. It
* initializes all core components, loading its configuration from an external JSON file.
* This engine is designed to be the intelligent core inside a native package (DEB, RPM,
* MSI, etc.), handling dynamic tasks like dependency verification, optional component
* downloads via live APIs, and system integration. This file implements a robust,
* hybrid configuration search path to support both development and production environments.
* SPDX-License-Identifier: Apache-2.0
*/

#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <stdexcept>
#include <filesystem>

// Third-party libraries
#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/basic_file_sink.h"

// Project-specific components
#include "utils/config_manager.hpp"
#include "utils/api_manager.hpp"
#include "platform/platform_detector.hpp"
#include "dependencies/dependency_manager.hpp"
#include "platform/iplatform_installer.hpp"
#include "platform/linux-systems/linux_installer.hpp"
#include "platform/darwin-mac/darwin-mac_installer.hpp"
#include "platform/windows/windows_installer.hpp"

namespace fs = std::filesystem;

// Custom Exception for controlled, high-level error handling.
class InstallerException : public std::runtime_error {
public:
    explicit InstallerException(const std::string& message) : std::runtime_error(message) {}
};

void setup_logging() {
    try {
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console_sink->set_level(spdlog::level::info);
        auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("phgit_installer.log", true);
        file_sink->set_level(spdlog::level::debug);
        std::vector<spdlog::sink_ptr> sinks{ console_sink, file_sink };
        auto logger = std::make_shared<spdlog::logger>("phgit_installer", sinks.begin(), sinks.end());
        logger->set_level(spdlog::level::debug);
        logger->flush_on(spdlog::level::debug);
        spdlog::set_default_logger(logger);
    } catch (const spdlog::spdlog_ex& ex) {
        std::cerr << "Log initialization failed: " << ex.what() << std::endl;
        exit(1);
    }
}

/**
 * @brief Locates the config.json file using a robust, hybrid strategy.
 *
 * This function implements a two-step search to find the configuration file:
 * 1.  **Production Path:** It first checks for a path provided at compile time via the
 *     `INSTALL_DATA_DIR` macro. This is the standard for installed applications
 *     (e.g., /usr/share/phgit/config.json on Linux).
 * 2.  **Development Path (Fallback):** If the production path is not defined or the file
 *     is not found, it falls back to looking for `config.json` in the same directory
 *     as the executable. This supports easy, out-of-the-box execution in a build directory.
 *
 * @param argv0 The first command-line argument (path to the executable).
 * @return The absolute path to config.json if found, otherwise an empty path.
 */
fs::path find_config_path(const char* argv0) {
#ifdef INSTALL_DATA_DIR
    // Production environment: Look in the compile-time defined data directory.
    // This path is set by CMake during the build process.
    fs::path install_path = fs::path(INSTALL_DATA_DIR) / "config.json";
    if (fs::exists(install_path)) {
        spdlog::debug("Found configuration file in install path: {}", install_path.string());
        return install_path;
    }
#endif

    // Development environment (or fallback): Look next to the executable.
    try {
        fs::path exe_path = fs::canonical(fs::path(argv0));
        fs::path dev_path = exe_path.parent_path() / "config.json";
        if (fs::exists(dev_path)) {
            spdlog::debug("Found configuration file in development path: {}", dev_path.string());
            return dev_path;
        }
    } catch (const fs::filesystem_error& e) {
        spdlog::warn("Could not resolve executable path to find config.json: {}", e.what());
    }

    return {}; // Return empty path if not found in any location.
}


/**
 * @brief Main entry point for the phgit installer engine.
 */
int main(int argc, char* argv[]) {
    setup_logging();

    try {
        // Step 1: Load Configuration using the robust search logic.
        fs::path config_path = find_config_path(argv[0]);
        if (config_path.empty()) {
            throw InstallerException("Could not find config.json in standard installation or development paths. Cannot proceed.");
        }

        auto config_manager = std::make_shared<phgit_installer::utils::ConfigManager>();
        if (!config_manager->load_from_file(config_path.string())) {
            throw InstallerException("Failed to load or parse config.json from: " + config_path.string());
        }
        auto metadata = config_manager->get_package_metadata().value_or(phgit_installer::utils::PackageMetadata{});
        spdlog::info("Starting {} installer engine v{}", metadata.name, metadata.version);

        // Step 2: Initialize Core Components
        auto api_manager = std::make_shared<phgit_installer::utils::ApiManager>(config_manager);
        phgit_installer::platform::PlatformDetector detector;
        phgit_installer::platform::PlatformInfo platform_info = detector.detect();
        phgit_installer::dependencies::DependencyManager dep_manager(platform_info, config_manager);
        dep_manager.check_all();

        // Step 3: Select and Execute the Platform-Specific Engine (Factory Pattern)
        std::unique_ptr<phgit_installer::platform::IPlatformInstaller> installer;
        if (platform_info.os_family == "linux") {
            installer = std::make_unique<phgit_installer::platform::LinuxInstaller>(platform_info, dep_manager, api_manager, config_manager);
        } else if (platform_info.os_family == "windows") {
            installer = std::make_unique<phgit_installer::platform::WindowsInstaller>(platform_info, dep_manager, api_manager, config_manager);
        } else if (platform_info.os_family == "macos") {
            installer = std::make_unique<phgit_installer::platform::MacosInstaller>(platform_info, dep_manager, api_manager, config_manager);
        } else {
            throw InstallerException("Unsupported OS family. Could not select an installer engine.");
        }

        // Step 4: Run the installation tasks
        installer->run_installation();
        spdlog::info("Installer engine finished its tasks successfully.");

    } catch (const InstallerException& e) {
        spdlog::critical("A critical installer error occurred: {}", e.what());
        return 1;
    } catch (const std::exception& e) {
        spdlog::critical("An unexpected standard error occurred: {}", e.what());
        return 1;
    } catch (...) {
        spdlog::critical("An unknown, non-standard error occurred. Aborting.");
        return 1;
    }

    return 0;
}