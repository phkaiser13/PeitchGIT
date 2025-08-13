/* Copyright (C) 2025 Pedro Henrique / phkaiser13
* File: platform_detector.h
* This header file defines the interface for the PlatformDetector class and its associated
* data structure, PlatformInfo. The PlatformDetector is a crucial component responsible for
* performing comprehensive environment detection. It identifies the operating system family,
* specific distribution/version, hardware architecture, and user privilege level. This information
* is fundamental for the installer to make correct decisions about which installation strategy
* and packages to use.
* SPDX-License-Identifier: Apache-2.0
*/

#pragma once // Modern include guard

#include <string>
#include <vector>

namespace phgit_installer::platform {

    /**
     * @struct PlatformInfo
     * @brief A data structure to hold all detected information about the host system.
     * This struct is populated by the PlatformDetector and used throughout the installer
     * to guide its behavior.
     */
    struct PlatformInfo {
        // Broad OS category, e.g., "linux", "windows", "macos", "bsd".
        std::string os_family;

        // Machine-readable OS identifier, e.g., "ubuntu", "fedora", "win11", "14.0".
        // For Linux, this corresponds to the ID field in /etc/os-release.
        std::string os_id;

        // Human-readable OS name, e.g., "Ubuntu", "Windows 11", "macOS Sonoma".
        std::string os_name;

        // OS version string, e.g., "22.04", "10.0.22631", "14.4.1".
        std::string os_version;

        // System hardware architecture, e.g., "x86_64", "aarch64", "armv7l".
        std::string architecture;

        // True if the installer is running with administrative/root privileges.
        bool is_privileged = false;
    };

    /**
     * @class PlatformDetector
     * @brief Provides methods to detect detailed information about the current platform.
     * It encapsulates all platform-specific detection logic, offering a clean
     * and simple interface to the rest of the application.
     */
    class PlatformDetector {
    public:
        /**
         * @brief The main public method to perform platform detection.
         * @return A PlatformInfo struct populated with the system's details.
         * @throws std::runtime_error if a critical piece of information (like OS family)
         *         cannot be determined.
         */
        PlatformInfo detect();

    private:
        /**
         * @brief Determines the base OS family and architecture using preprocessor macros.
         * This is the first and most fundamental detection step.
         * @param info A reference to the PlatformInfo struct to be populated.
         */
        void detect_os_family_and_arch(PlatformInfo& info);

        /**
         * @brief Performs detailed detection for Linux distributions.
         * It parses /etc/os-release to get distro ID, name, and version.
         * @param info A reference to the PlatformInfo struct to be populated.
         */
        void detect_linux_distro(PlatformInfo& info);

        /**
         * @brief Performs detailed detection for Windows versions.
         * @note The actual implementation is complex and involves WinAPI calls.
         * @param info A reference to the PlatformInfo struct to be populated.
         */
        void detect_windows_version(PlatformInfo& info);

        /**
         * @brief Performs detailed detection for macOS versions.
         * @note The actual implementation may involve calling `sw_vers` or using system APIs.
         * @param info A reference to the PlatformInfo struct to be populated.
         */
        void detect_macos_version(PlatformInfo& info);

        /**
         * @brief Checks if the current process has administrative/root privileges.
         * @return True if privileged, false otherwise.
         */
        bool check_privileges();
    };

} // namespace phgit_installer::platform
