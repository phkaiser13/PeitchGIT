/* Copyright (C) 2025 Pedro Henrique / phkaiser13
* File: main.cpp
* This is the main entry point and orchestrator for the phgit installer. It integrates all
* core components to provide a seamless installation experience. Its responsibilities are
* strictly high-level: initialize logging, detect the host platform, check for dependencies,
* select the appropriate platform-specific installer engine, and execute it. All complex
* logic is delegated to the specialized components, making this file clean, readable,
* and a true representation of the project's modular architecture.
* SPDX-License-Identifier: Apache-2.0
*/

#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <stdexcept>

// Third-party libraries
#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/basic_file_sink.h"

// Project-specific components
#include "platform/platform_detector.hpp"
#include "dependencies/dependency_manager.hpp"
#include "platform/iplatform_installer.hpp"
#include "platform/linux-systems/linux_installer.hpp"
#include "platform/darwin-mac/darwin-mac_installer.hpp"
#include "platform/windows/windows_installer.hpp"

// --- Project-specific Constants ---
namespace phgit_installer::constants {
    const std::string_view PROJECT_NAME = "phgit";
    const std::string_view PROJECT_VERSION = "1.0.0";
}

// --- Custom Exception for Controlled Error Handling ---
class InstallerException : public std::runtime_error {
public:
    explicit InstallerException(const std::string& message)
        : std::runtime_error(message) {}
};

/**
 * @brief Initializes the logging system with console and file sinks.
 */
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
    }
    catch (const spdlog::spdlog_ex& ex) {
        std::cerr << "Log initialization failed: " << ex.what() << std::endl;
        exit(1);
    }
}

/**
 * @brief Main entry point for the phgit installer application.
 * Orchestrates the entire installation flow.
 * @return 0 on success, non-zero on failure.
 */
int main(int argc, char* argv[]) {
    setup_logging();

    try {
        spdlog::info("Starting {} installer v{}",
            phgit_installer::constants::PROJECT_NAME.data(),
            phgit_installer::constants::PROJECT_VERSION.data());

        // Step 1: Detect the current platform (OS, architecture, etc.)
        phgit_installer::platform::PlatformDetector detector;
        phgit_installer::platform::PlatformInfo platform_info = detector.detect();

        if (platform_info.os_family == "unknown" || platform_info.architecture == "unknown") {
            throw InstallerException("Unsupported operating system or architecture detected. Aborting.");
        }

        // Step 2: Check for all required and optional dependencies
        phgit_installer::dependencies::DependencyManager dep_manager(platform_info);
        dep_manager.check_all();

        // Step 3: Select the appropriate installer engine based on the detected platform
        std::unique_ptr<phgit_installer::platform::IPlatformInstaller> installer;
        if (platform_info.os_family == "linux") {
            installer = std::make_unique<phgit_installer::platform::LinuxInstaller>(platform_info, dep_manager);
        } else if (platform_info.os_family == "windows") {
            installer = std::make_unique<phgit_installer::platform::WindowsInstaller>(platform_info, dep_manager);
        } else if (platform_info.os_family == "macos") {
            installer = std::make_unique<phgit_installer::platform::MacosInstaller>(platform_info, dep_manager);
        } else {
            // This case should be caught by the earlier check, but serves as a safeguard.
            throw InstallerException("Could not find a suitable installer for the detected OS family.");
        }

        // Step 4: Run the installation process
        installer->run_installation();

        spdlog::info("phgit installation process finished.");

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
