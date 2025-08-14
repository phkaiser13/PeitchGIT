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

void setup_logging() { /* Omitted for brevity, same as before */ }

/**
 * @brief Main entry point for the phgit installer engine.
 */
int main(int argc, char* argv[]) {
    setup_logging();

    try {
        // Step 1: Load Configuration
        // The config file is expected to be next to the executable.
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

        // The DependencyManager should also be driven by the config
        phgit_installer::dependencies::DependencyManager dep_manager(platform_info, config_manager);
        dep_manager.check_all();

        // Step 3: Select and Execute the Platform-Specific Engine
        std::unique_ptr<phgit_installer::platform::IPlatformInstaller> installer;
        if (platform_info.os_family == "linux") {
            // The installers now need the ApiManager to download optional dependencies
            installer = std::make_unique<phgit_installer::platform::LinuxInstaller>(platform_info, dep_manager, api_manager);
        } else if (platform_info.os_family == "windows") {
            installer = std::make_unique<phgit_installer::platform::WindowsInstaller>(platform_info, dep_manager, api_manager);
        } else if (platform_info.os_family == "macos") {
            installer = std::make_unique<phgit_installer::platform::MacosInstaller>(platform_info, dep_manager, api_manager);
        } else {
            throw InstallerException("Unsupported OS family. Could not select an instal
