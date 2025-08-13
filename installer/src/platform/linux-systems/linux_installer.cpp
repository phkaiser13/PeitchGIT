/* Copyright (C) 2025 Pedro Henrique / phkaiser13
* File: linux_installer.cpp
* This file implements the LinuxInstaller class. It contains the core logic for installing
* the application on various Linux distributions. It uses a dispatch strategy to select the
* appropriate package manager (APT, DNF, etc.) based on the detected OS ID. For each
* package manager, it handles dependency installation and the installation of the phgit
* package itself. A fallback to a generic tarball installation is provided for maximum
* compatibility.
* SPDX-License-Identifier: Apache-2.0
*/

#include "platform/linux-systems/linux_installer.hpp"
#include "spdlog/spdlog.h"

#include <cstdlib> // For system() - placeholder for a more robust process execution utility
#include <vector>
#include <set>

namespace phgit_installer::platform {

    // Helper function to simulate command execution
    // In a real implementation, this would use a robust process utility like the one
    // in DependencyManager, with proper error handling and output capturing.
    static bool execute_system_command(const std::string& command) {
        spdlog::info("Executing command: {}", command);
        // int return_code = std::system(command.c_str());
        // if (return_code != 0) {
        //     spdlog::error("Command failed with exit code: {}", return_code);
        //     return false;
        // }
        spdlog::warn("Command execution is currently simulated.");
        return true;
    }

    LinuxInstaller::LinuxInstaller(platform::PlatformInfo info, const dependencies::DependencyManager& dep_manager)
        : m_platform_info(std::move(info)), m_dep_manager(dep_manager) {
        spdlog::debug("LinuxInstaller instance created for OS: {}", m_platform_info.os_id);
    }

    void LinuxInstaller::run_installation() {
        spdlog::info("Starting Linux installation process.");
        if (!m_platform_info.is_privileged) {
            spdlog::warn("Installer is not running with root privileges (sudo).");
            spdlog::warn("System-wide installation may fail or require password prompts.");
        }
        dispatch_installation_strategy();
    }

    void LinuxInstaller::dispatch_installation_strategy() {
        const std::string& id = m_platform_info.os_id;
        const std::set<std::string> debian_family = {"debian", "ubuntu", "mint", "elementary", "pop"};
        const std::set<std::string> fedora_family = {"fedora", "rhel", "centos", "rocky", "alma"};
        const std::set<std::string> arch_family = {"arch", "manjaro", "endeavouros", "garuda"};
        const std::set<std::string> suse_family = {"opensuse", "sles"};

        if (debian_family.count(id)) {
            install_using_apt();
        } else if (fedora_family.count(id)) {
            install_using_dnf();
        } else if (arch_family.count(id)) {
            install_using_pacman();
        } else if (suse_family.count(id)) {
            install_using_zypper();
        } else {
            spdlog::warn("Unsupported distribution '{}'. Falling back to tarball installation.", id);
            install_from_tarball();
        }
    }

    void LinuxInstaller::install_using_apt() {
        spdlog::info("Using APT package manager strategy.");
        
        auto git_status = m_dep_manager.get_status("git");
        if (!git_status || !git_status->is_found || !git_status->is_version_ok) {
            spdlog::info("Git is missing or outdated. Attempting to install via APT.");
            execute_system_command("sudo apt-get update -y");
            install_system_dependencies("apt-get install -y", {"git"});
        }
        
        // Placeholder for installing the actual .deb package
        // The path would come from a download manager or be bundled with the installer.
        std::string package_path = "phgit_1.0.0_amd64.deb";
        install_local_package("dpkg -i", package_path);
    }

    void LinuxInstaller::install_using_dnf() {
        spdlog::info("Using DNF/YUM package manager strategy.");

        auto git_status = m_dep_manager.get_status("git");
        if (!git_status || !git_status->is_found || !git_status->is_version_ok) {
            spdlog::info("Git is missing or outdated. Attempting to install via DNF.");
            install_system_dependencies("dnf install -y", {"git"});
        }

        std::string package_path = "phgit-1.0.0-1.x86_64.rpm";
        install_local_package("rpm -ivh", package_path);
    }

    void LinuxInstaller::install_using_pacman() {
        spdlog::info("Using Pacman package manager strategy.");

        auto git_status = m_dep_manager.get_status("git");
        if (!git_status || !git_status->is_found || !git_status->is_version_ok) {
            spdlog::info("Git is missing or outdated. Attempting to install via Pacman.");
            execute_system_command("sudo pacman -Sy --noconfirm");
            install_system_dependencies("pacman -S --noconfirm", {"git"});
        }

        std::string package_path = "phgit-1.0.0-1-x86_64.pkg.tar.zst";
        install_local_package("pacman -U --noconfirm", package_path);
    }

    void LinuxInstaller::install_using_zypper() {
        spdlog::info("Using Zypper package manager strategy.");

        auto git_status = m_dep_manager.get_status("git");
        if (!git_status || !git_status->is_found || !git_status->is_version_ok) {
            spdlog::info("Git is missing or outdated. Attempting to install via Zypper.");
            execute_system_command("sudo zypper refresh");
            install_system_dependencies("zypper install -y", {"git"});
        }

        std::string package_path = "phgit-1.0.0-1.x86_64.rpm";
        install_local_package("rpm -ivh", package_path);
    }

    void LinuxInstaller::install_from_tarball() {
        spdlog::info("Using generic tarball installation strategy.");
        // 1. Download the correct tarball for the architecture.
        // 2. Verify checksum.
        // 3. Extract to a target directory (e.g., /usr/local/phgit or ~/.local/phgit).
        // 4. Update user's .bashrc/.zshrc or system-wide profile to add to PATH.
        spdlog::info("Step 1: Download phgit-1.0.0-{}.tar.gz", m_platform_info.architecture);
        spdlog::info("Step 2: Extract tarball to /usr/local/bin");
        spdlog::info("Step 3: Advise user to add /usr/local/bin to their PATH if not already present.");
        spdlog::info("Tarball installation complete (simulated).");
    }

    void LinuxInstaller::install_system_dependencies(const std::string& package_manager_cmd, const std::vector<std::string>& packages) {
        std::string package_list;
        for(const auto& pkg : packages) {
            package_list += pkg + " ";
        }
        
        std::string full_command = "sudo " + package_manager_cmd + " " + package_list;
        execute_system_command(full_command);
    }

    void LinuxInstaller::install_local_package(const std::string& package_tool_cmd, const std::string& package_path) {
        // In a real scenario, we'd check if the package_path exists first.
        spdlog::info("Installing phgit from local package: {}", package_path);
        std::string full_command = "sudo " + package_tool_cmd + " " + package_path;
        execute_system_command(full_command);
    }

} // namespace phgit_installer::platform
