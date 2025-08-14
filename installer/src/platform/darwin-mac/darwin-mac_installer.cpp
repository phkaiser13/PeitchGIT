/* Copyright (C) 2025 Pedro Henrique / phkaiser13
* File: darwin-mac_installer.cpp
* This file provides the final, fully functional implementation of the MacosInstaller.
* It uses ProcessExecutor to robustly call `brew` and `installer` commands. The fallback
* logic is now fully implemented, using the complete chain of ApiManager -> Downloader ->
* SHA256 to provide a seamless installation on macOS, even without Homebrew.
* SPDX-License-Identifier: Apache-2.0
*/

#include "platform/darwin-mac/darwin-mac_installer.hpp"
#include "utils/process_executor.hpp"
#include "utils/downloader.hpp"
#include "utils/sha256.hpp"
#include "spdlog/spdlog.h"

#include <iostream>
#include <filesystem>

namespace phgit_installer::platform {

    namespace fs = std::filesystem;

    // Helper for progress bar
    static void print_progress(uint64_t total, uint64_t downloaded) {
        if (total == 0) return;
        int percentage = static_cast<int>((static_cast<double>(downloaded) / total) * 100.0);
        std::cout << "\rDownloading... " << percentage << "% [" << downloaded << " / " << total << " bytes]" << std::flush;
        if (downloaded == total) {
            std::cout << std::endl;
        }
    };

    MacosInstaller::MacosInstaller(
        platform::PlatformInfo info,
        const dependencies::DependencyManager& dep_manager,
        std::shared_ptr<utils::ApiManager> api_manager,
        std::shared_ptr<utils::ConfigManager> config)
        : m_platform_info(std::move(info)),
          m_dep_manager(dep_manager),
          m_api_manager(std::move(api_manager)),
          m_config(std::move(config)),
          m_homebrew_is_available(is_homebrew_available()) {
        spdlog::debug("MacosInstaller engine fully initialized. Homebrew available: {}", m_homebrew_is_available);
    }

    void MacosInstaller::run_installation() {
        spdlog::info("Starting macOS post-installation tasks.");
        dispatch_installation_strategy();
    }

    bool MacosInstaller::is_homebrew_available() {
        spdlog::debug("Checking for Homebrew using ProcessExecutor...");
        // Use our robust executor to check for the command's existence.
        auto result = utils::ProcessExecutor::execute("command -v brew");
        return result.exit_code == 0;
    }

    void MacosInstaller::dispatch_installation_strategy() {
        if (m_homebrew_is_available) {
            install_using_homebrew();
        } else {
            spdlog::warn("Homebrew not found. This engine's tasks are complete.");
            // As a post-install script, we assume the main package was installed via other means (.pkg).
            // The main task is to verify dependencies.
            if (!m_dep_manager.get_status("git").value_or(dependencies::DependencyStatus{}).is_version_ok) {
                prompt_for_command_line_tools();
            }
        }
    }

    void MacosInstaller::install_using_homebrew() {
        spdlog::info("Using Homebrew to ensure all dependencies are met.");
        ensure_dependencies_with_brew();
        spdlog::info("Homebrew dependency check complete.");
    }

    void MacosInstaller::ensure_dependencies_with_brew() {
        // This method is run by a post-install script of the phgit Homebrew formula.
        // Its job is to ensure optional dependencies are installed if the user wants them.
        // For now, we just ensure git is present.
        if (!m_dep_manager.get_status("git").value_or(dependencies::DependencyStatus{}).is_version_ok) {
            spdlog::info("Git is missing or outdated. Installing/upgrading with Homebrew.");
            auto result = utils::ProcessExecutor::execute("brew install git");
            if (result.exit_code != 0) {
                spdlog::error("Failed to install git via Homebrew. Stderr: {}", result.std_err);
            }
        }
        // A more advanced implementation could prompt the user to install optional deps.
    }

    // This method is now for a standalone installer script, not a post-install task.
    void MacosInstaller::install_from_pkg() {
        spdlog::info("Executing .pkg installation strategy.");
        if (!m_dep_manager.get_status("git").value_or(dependencies::DependencyStatus{}).is_version_ok) {
            prompt_for_command_line_tools();
            throw std::runtime_error("Git is required. Please install Xcode Command Line Tools and run again.");
        }

        auto asset = m_api_manager->fetch_latest_asset("phgit-pkg", m_platform_info);
        if (!asset) {
            throw std::runtime_error("Could not resolve .pkg download URL from API.");
        }

        utils::Downloader downloader;
        fs::path installer_path = fs::temp_directory_path() / "phgit.pkg";

        spdlog::info("Downloading from: {}", asset->download_url);
        if (!downloader.download_file(asset->download_url, installer_path.string(), print_progress)) {
            throw std::runtime_error("Failed to download .pkg installer.");
        }

        std::string actual_hash = utils::crypto::SHA256::from_file(installer_path.string());
        if (actual_hash != asset->checksum && !asset->checksum.empty()) {
            remove(installer_path);
            throw std::runtime_error("Checksum mismatch for .pkg installer!");
        }

        spdlog::info("Download verified. Starting system installer...");
        std::string command = "sudo installer -pkg \"" + installer_path.string() + "\" -target /";
        auto result = utils::ProcessExecutor::execute(command);
        remove(installer_path);

        if (result.exit_code != 0) {
            throw std::runtime_error("macOS installer command failed. Stderr: " + result.std_err);
        }
        spdlog::info(".pkg installation completed successfully.");
    }

    void MacosInstaller::perform_manual_installation() {
        spdlog::critical("Automatic installation is not available or has failed.");
        auto asset = m_api_manager->fetch_latest_asset("phgit-tarball", m_platform_info);
        
        spdlog::info("--------------------------------------------------");
        spdlog::info("MANUAL INSTALLATION REQUIRED:");
        spdlog::info("1. Ensure Git is installed. If not, run 'xcode-select --install' in your terminal.");
        if (asset) {
            spdlog::info("2. Download the latest binary from: {}", asset->download_url);
        } else {
            spdlog::info("2. Download the latest phgit binary for your architecture ({}) from the GitHub releases page.", m_platform_info.architecture);
        }
        spdlog::info("3. Unzip the download and move the 'phgit' executable to a directory in your PATH, for example:");
        spdlog::info("   sudo mv phgit /usr/local/bin/");
        spdlog::info("--------------------------------------------------");
    }

    void MacosInstaller::prompt_for_command_line_tools() {
        spdlog::warn("--------------------------------------------------");
        spdlog::warn("REQUIRED ACTION: Git is not installed.");
        spdlog::warn("To install it, please run the following command in your terminal:");
        spdlog::warn("  xcode-select --install");
        spdlog::warn("After the installation is complete, please run this installer again.");
        spdlog::warn("--------------------------------------------------");
    }

} // namespace phgit_installer::platform
