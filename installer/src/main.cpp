/* Copyright (C) 2025 Pedro Henrique / phkaiser13
* File: main.cpp
* This is the main entry point for the phgit installer. It orchestrates the entire installation
* process by integrating and calling core components. It initializes logging, handles command-line
* arguments, uses the PlatformDetector to understand the environment, checks for all required
* dependencies using the DependencyManager, and then delegates the actual installation work to a
* platform-specific installer module.
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


// --- Forward Declarations & Stubs for Core Components ---
// The following stubs will be replaced with real implementations in their own files.
namespace phgit_installer {

    /**
     * @class IPlatformInstaller
     * @brief Interface for platform-specific installation logic.
     */
    class IPlatformInstaller {
    public:
        virtual ~IPlatformInstaller() = default;
        virtual void run_installation() = 0;
    };

    // STUB implementations for platform-specific installers
    class LinuxInstaller : public IPlatformInstaller {
    public:
        explicit LinuxInstaller(const platform::PlatformInfo& info) : platform_info(info) {}
        void run_installation() override {
            spdlog::info("Running Linux installer for distro: {}", platform_info.os_id);
            // STUB: Logic for apt, dnf, pacman, etc. and dependency installation would go here.
            spdlog::info("Linux installation steps would be executed here.");
            spdlog::info("Linux installation completed successfully.");
        }
    private:
        platform::PlatformInfo platform_info;
    };

    class WindowsInstaller : public IPlatformInstaller {
    public:
        void run_installation() override {
            spdlog::info("Running Windows installer...");
            // STUB: Logic for MSI, NSIS, winget, etc. and dependency installation would go here.
            spdlog::info("Windows installation steps would be executed here.");
            spdlog::info("Windows installation completed successfully.");
        }
    };

    class MacosInstaller : public IPlatformInstaller {
    public:
        void run_installation() override {
            spdlog::info("Running macOS installer...");
            // STUB: Logic for .pkg, .dmg, homebrew, etc. and dependency installation would go here.
            spdlog::info("macOS installation steps would be executed here.");
            spdlog::info("macOS installation completed successfully.");
        }
    };

} // namespace phgit_installer


/**
 * @brief Initializes the logging system.
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
 */
int main(int argc, char* argv[]) {
    setup_logging();

    try {
        spdlog::info("Starting {} installer v{}",
            phgit_installer::constants::PROJECT_NAME.data(),
            phgit_installer::constants::PROJECT_VERSION.data());

        // Step 1: Platform Detection
        phgit_installer::platform::PlatformDetector detector;
        phgit_installer::platform::PlatformInfo platform_info = detector.detect();

        if (platform_info.os_family == "unknown" || platform_info.architecture == "unknown") {
            throw InstallerException("Unsupported operating system or architecture detected. Aborting.");
        }

        // Step 2: Dependency Checking
        phgit_installer::dependencies::DependencyManager dep_manager(platform_info);
        dep_manager.check_all();

        if (!dep_manager.are_core_dependencies_met()) {
            spdlog::warn("One or more required dependencies are missing or outdated.");
            spdlog::warn("The installer will attempt to install them if possible.");
        } else {
            spdlog::info("All required dependencies are met.");
        }

        // Step 3: Installer Selection and Execution
        std::unique_ptr<phgit_installer::IPlatformInstaller> installer;
        if (platform_info.os_family == "linux") {
            installer = std::make_unique<phgit_installer::LinuxInstaller>(platform_info);
        } else if (platform_info.os_family == "windows") {
            installer = std::make_unique<phgit_installer::WindowsInstaller>();
        } else if (platform_info.os_family == "macos") {
            installer = std::make_unique<phgit_installer::MacosInstaller>();
        } else {
            throw InstallerException("Could not find a suitable installer for the detected OS.");
        }

        installer->run_installation();

        spdlog::info("phgit installation process finished successfully.");

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
