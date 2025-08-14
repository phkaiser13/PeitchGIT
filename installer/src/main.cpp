/* Copyright (C) 2025 Pedro Henrique / phkaiser13
* File: main.cpp
* This is the final, data-driven entry point for the phgit installer engine. It
* initializes all core components, loading its configuration from an external JSON file.
* This engine is designed to be the intelligent core inside a native package (DEB, RPM,
* MSI, etc.), handling dynamic tasks like dependency verification, optional component
* downloads via live APIs, and system integration.
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

// Custom Exception for controlled error handling
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
 * @brief Main entry point for the phgit installer engine.
 */
int main(int argc, char* argv[]) {
    setup_logging();

    try {
        // Step 1: Load Configuration
        fs::path exe_path = fs::canonical(fs::path(argv[0]));
        fs::path config_path = exe_path.parent_path() / "config.json";
        auto config_manager = std::make_shared<phgit_installer::utils::ConfigManager>();
        if (!config_manager->load_from_file(config_path.string())) {
            throw InstallerException("Could not load or parse config.json. Cannot proceed.");
        }
        auto metadata = config_manager->get_package_metadata().value_or(phgit_installer::utils::PackageMetadata{});
        spdlog::info("Starting {} installer engine v{}", metadata.name, metadata.version);

        // Step 2: Initialize Core Components
        auto api_manager = std::make_shared<phgit_installer::utils::ApiManager>(config_manager);
        phgit_installer::platform::PlatformDetector detector;
        phgit_installer::platform::PlatformInfo platform_info = detector.detect();
        phgit_installer::dependencies::DependencyManager dep_manager(platform_info, config_manager);
        dep_manager.check_all();

        // Step 3: Select and Execute the Platform-Specific Engine
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
        spdlog::critical("An unknown error occurred. Aborting.");
        return 1;
    }

    return 0;
}
