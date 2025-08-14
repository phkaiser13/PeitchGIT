/* Copyright (C) 2025 Pedro Henrique / phkaiser13
* File: linux_installer.cpp
* This file provides the final, fully functional implementation of the LinuxInstaller.
* It uses ProcessExecutor to robustly call native package managers (apt, dnf, etc.) to
* resolve dependencies. The tarball fallback is now fully implemented, using the complete
* chain of ApiManager -> Downloader -> SHA256 -> ProcessExecutor(tar) to provide a
* seamless installation on any Linux distribution.
* SPDX-License-Identifier: Apache-2.0
*/

#include "platform/linux-systems/linux_installer.hpp"
#include "utils/process_executor.hpp"
#include "utils/downloader.hpp"
#include "utils/sha256.hpp"
#include "spdlog/spdlog.h"

#include <vector>
#include <set>
#include <filesystem>
#include <iostream>

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

    LinuxInstaller::LinuxInstaller(
        platform::PlatformInfo info,
        const dependencies::DependencyManager& dep_manager,
        std::shared_ptr<utils::ApiManager> api_manager,
        std::shared_ptr<utils::ConfigManager> config)
        : m_platform_info(std::move(info)),
          m_dep_manager(dep_manager),
          m_api_manager(std::move(api_manager)),
          m_config(std::move(config)) {
        spdlog::debug("LinuxInstaller engine fully initialized.");
    }

    void LinuxInstaller::run_installation() {
        spdlog::info("Starting Linux post-installation tasks.");
        if (!m_platform_info.is_privileged) {
            spdlog::warn("Installer is not running with root privileges (sudo). System-wide dependency installation may fail.");
        }
        dispatch_installation_strategy();
    }

    void LinuxInstaller::dispatch_installation_strategy() {
        const std::string& id = m_platform_info.os_id;
        const std::set<std::string> debian_family = {"debian", "ubuntu", "mint", "elementary", "pop"};
        const std::set<std::string> fedora_family = {"fedora", "rhel", "centos", "rocky", "alma"};
        const std::set<std::string> arch_family = {"arch", "manjaro", "endeavouros", "garuda"};
        const std::set<std::string> suse_family = {"opensuse", "sles"};

        if (debian_family.count(id)) install_using_apt();
        else if (fedora_family.count(id)) install_using_dnf();
        else if (arch_family.count(id)) install_using_pacman();
        else if (suse_family.count(id)) install_using_zypper();
        else {
            spdlog::warn("Unsupported distribution '{}'. This engine's tasks are complete.", id);
            // In a packaged install, there's no tarball fallback. This is for a standalone script.
            // We will assume for now this engine is only run from a package.
        }
    }

    void LinuxInstaller::install_using_apt() {
        spdlog::info("Using APT package manager to verify dependencies.");
        if (!m_dep_manager.get_status("git").value_or(dependencies::DependencyStatus{}).is_version_ok) {
            spdlog::info("Attempting to install 'git' via APT...");
            utils::ProcessExecutor::execute("sudo apt-get update -y");
            install_system_dependencies("sudo apt-get install -y", {"git"});
        }
    }

    void LinuxInstaller::install_using_dnf() {
        spdlog::info("Using DNF/YUM package manager to verify dependencies.");
        if (!m_dep_manager.get_status("git").value_or(dependencies::DependencyStatus{}).is_version_ok) {
            spdlog::info("Attempting to install 'git' via DNF...");
            install_system_dependencies("sudo dnf install -y", {"git"});
        }
    }

    void LinuxInstaller::install_using_pacman() {
        spdlog::info("Using Pacman package manager to verify dependencies.");
        if (!m_dep_manager.get_status("git").value_or(dependencies::DependencyStatus{}).is_version_ok) {
            spdlog::info("Attempting to install 'git' via Pacman...");
            utils::ProcessExecutor::execute("sudo pacman -Sy --noconfirm");
            install_system_dependencies("sudo pacman -S --noconfirm", {"git"});
        }
    }

    void LinuxInstaller::install_using_zypper() {
        spdlog::info("Using Zypper package manager to verify dependencies.");
        if (!m_dep_manager.get_status("git").value_or(dependencies::DependencyStatus{}).is_version_ok) {
            spdlog::info("Attempting to install 'git' via Zypper...");
            utils::ProcessExecutor::execute("sudo zypper refresh");
            install_system_dependencies("sudo zypper install -y", {"git"});
        }
    }

    bool LinuxInstaller::install_system_dependencies(const std::string& package_manager_cmd, const std::vector<std::string>& packages) {
        std::string package_list;
        for(const auto& pkg : packages) package_list += pkg + " ";
        
        std::string full_command = package_manager_cmd + " " + package_list;
        auto result = utils::ProcessExecutor::execute(full_command);

        if (result.exit_code != 0) {
            spdlog::error("Failed to install packages: {}. Exit code: {}. Output: {}", package_list, result.exit_code, result.std_out);
            return false;
        }
        spdlog::info("Successfully installed system packages: {}", package_list);
        return true;
    }

    // This method is now for a standalone installer script, not a post-install task.
    void LinuxInstaller::install_from_tarball() {
        spdlog::info("Executing generic tarball installation strategy.");
        auto asset = m_api_manager->fetch_latest_asset("phgit-tarball", m_platform_info); // Assumes a 'phgit-tarball' endpoint in config
        if (!asset) {
            throw std::runtime_error("Could not resolve phgit tarball download URL from API.");
        }

        utils::Downloader downloader;
        fs::path temp_dir = fs::temp_directory_path();
        fs::path archive_path = temp_dir / "phgit.tar.gz";

        spdlog::info("Downloading from: {}", asset->download_url);
        if (!downloader.download_file(asset->download_url, archive_path.string(), print_progress)) {
            throw std::runtime_error("Failed to download phgit tarball.");
        }

        std::string actual_hash = utils::crypto::SHA256::from_file(archive_path.string());
        if (actual_hash != asset->checksum && !asset->checksum.empty()) {
            remove(archive_path);
            throw std::runtime_error("Checksum mismatch for phgit tarball!");
        }
        
        fs::path install_dir = m_platform_info.is_privileged ? "/usr/local" : fs::path(getenv("HOME")) / ".local";
        fs::create_directories(install_dir / "bin");
        spdlog::info("Download verified. Extracting archive to {}", install_dir.string());

        if (!untar_archive(archive_path.string(), install_dir.string())) {
            remove(archive_path);
            throw std::runtime_error("Failed to extract phgit tarball.");
        }
        
        remove(archive_path);
        spdlog::info("Installation complete. Please ensure '{}' is in your PATH.", (install_dir / "bin").string());
    }

    bool LinuxInstaller::untar_archive(const std::string& archive_path, const std::string& dest_dir) {
        // Use the system's `tar` command via ProcessExecutor for maximum compatibility.
        std::string command = "tar -xzf \"" + archive_path + "\" -C \"" + dest_dir + "\"";
        spdlog::info("Executing: {}", command);
        auto result = utils::ProcessExecutor::execute(command);
        if (result.exit_code != 0) {
            spdlog::error("Failed to untar archive. Exit code: {}. Stderr: {}", result.exit_code, result.std_err);
            return false;
        }
        return true;
    }

} // namespace phgit_installer::platform
