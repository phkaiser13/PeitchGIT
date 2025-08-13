/* Copyright (C) 2025 Pedro Henrique / phkaiser13
* File: platform_detector.cpp
* This file implements the PlatformDetector class. It contains the low-level,
* platform-specific code required to accurately identify the host system.
* For Linux, it parses /etc/os-release. For Windows, it uses the WinAPI to check
* for version and privilege information. For macOS, it provides the framework for
* similar checks. The goal is to abstract away the complexity of platform detection
* behind a simple and clean interface.
* SPDX-License-Identifier: Apache-2.0
*/

#include "platform/platform_detector.hpp"
#include "spdlog/spdlog.h"

#include <stdexcept>
#include <fstream>
#include <sstream>

// Platform-specific includes
#if defined(_WIN32) || defined(_WIN64)
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
    #include <VersionHelpers.h> // For IsWindows10OrGreater
#elif defined(__linux__)
    #include <unistd.h> // For getuid()
#elif defined(__APPLE__) && defined(__MACH__)
    #include <unistd.h> // For getuid()
    // For a more robust implementation, a utility to run external commands
    // like `sw_vers` would be needed here.
#endif


namespace phgit_installer::platform {

    PlatformInfo PlatformDetector::detect() {
        spdlog::debug("Initiating platform detection process.");
        PlatformInfo info;

        // 1. Detect broad OS family and architecture using compile-time macros.
        detect_os_family_and_arch(info);
        if (info.os_family == "unknown") {
            spdlog::critical("Could not determine OS family. The platform is unsupported.");
            throw std::runtime_error("Unsupported operating system.");
        }

        // 2. Dispatch to OS-specific detection routines.
        if (info.os_family == "linux") {
            detect_linux_distro(info);
        } else if (info.os_family == "windows") {
            detect_windows_version(info);
        } else if (info.os_family == "macos") {
            detect_macos_version(info);
        }

        // 3. Check for administrative privileges.
        info.is_privileged = check_privileges();

        spdlog::info("Platform detection complete:");
        spdlog::info("  OS Family: {}", info.os_family);
        spdlog::info("  OS Name: {}", info.os_name.empty() ? "N/A" : info.os_name);
        spdlog::info("  OS ID: {}", info.os_id.empty() ? "N/A" : info.os_id);
        spdlog::info("  OS Version: {}", info.os_version.empty() ? "N/A" : info.os_version);
        spdlog::info("  Architecture: {}", info.architecture);
        spdlog::info("  Privileges: {}", info.is_privileged ? "Administrator/Root" : "User");

        return info;
    }

    void PlatformDetector::detect_os_family_and_arch(PlatformInfo& info) {
        #if defined(_WIN32) || defined(_WIN64)
            info.os_family = "windows";
            #if defined(_M_AMD64)
                info.architecture = "x86_64";
            #elif defined(_M_ARM64)
                info.architecture = "aarch64";
            #else
                info.architecture = "unknown";
            #endif
        #elif defined(__linux__)
            info.os_family = "linux";
            #if defined(__x86_64__)
                info.architecture = "x86_64";
            #elif defined(__aarch64__)
                info.architecture = "aarch64";
            #elif defined(__arm__)
                info.architecture = "armv7l"; // Assuming ARMv7, could be refined
            #elif defined(__i386__)
                info.architecture = "i686";
            #else
                info.architecture = "unknown";
            #endif
        #elif defined(__APPLE__) && defined(__MACH__)
            info.os_family = "macos";
            #if defined(__x86_64__)
                info.architecture = "x86_64";
            #elif defined(__aarch64__)
                info.architecture = "aarch64";
            #else
                info.architecture = "unknown";
            #endif
        #else
            info.os_family = "unknown";
            info.architecture = "unknown";
        #endif
    }

    void PlatformDetector::detect_linux_distro(PlatformInfo& info) {
        spdlog::debug("Performing Linux distribution detection by parsing /etc/os-release.");
        std::ifstream os_release_file("/etc/os-release");
        if (!os_release_file.is_open()) {
            spdlog::warn("/etc/os-release not found. Falling back to generic 'linux'.");
            info.os_id = "linux"; // Generic fallback ID
            info.os_name = "Linux";
            return;
        }

        std::string line;
        while (std::getline(os_release_file, line)) {
            if (line.empty()) continue;
            
            auto delimiter_pos = line.find('=');
            if (delimiter_pos == std::string::npos) continue;

            std::string key = line.substr(0, delimiter_pos);
            std::string value = line.substr(delimiter_pos + 1);

            // Remove quotes from value if they exist
            if (value.length() >= 2 && value.front() == '"' && value.back() == '"') {
                value = value.substr(1, value.length() - 2);
            }

            if (key == "ID") {
                info.os_id = value;
            } else if (key == "NAME") {
                info.os_name = value;
            } else if (key == "VERSION_ID") {
                info.os_version = value;
            }
        }
    }

    void PlatformDetector::detect_windows_version(PlatformInfo& info) {
        spdlog::debug("Performing Windows version detection.");
        // A more robust implementation would use RtlGetVersion, but IsWindows*OrGreater
        // from VersionHelpers.h is sufficient for many use cases.
        if (IsWindows10OrGreater()) {
            // Further checks could be done via the registry to differentiate 10 and 11.
            // HKLM\SOFTWARE\Microsoft\Windows NT\CurrentVersion -> ProductName/DisplayVersion
            info.os_name = "Windows 10 or newer";
            info.os_id = "win10+";
            info.os_version = "10.0+"; // Placeholder
        } else if (IsWindows8Point1OrGreater()) {
            info.os_name = "Windows 8.1";
            info.os_id = "win8.1";
            info.os_version = "6.3";
        } else {
            info.os_name = "Older Windows";
            info.os_id = "win_old";
        }
    }

    void PlatformDetector::detect_macos_version(PlatformInfo& info) {
        spdlog::debug("Performing macOS version detection.");
        // STUB: A real implementation would execute `sw_vers` and parse the output.
        // For example:
        //   std::string product = execute_command("sw_vers -productName");
        //   std::string version = execute_command("sw_vers -productVersion");
        // This requires a robust cross-platform utility for running shell commands.
        info.os_name = "macOS";
        info.os_id = "macos";
        info.os_version = "11.0+"; // Placeholder, matches minimum requirement.
    }

    bool PlatformDetector::check_privileges() {
        spdlog::debug("Checking for administrator/root privileges.");
        #if defined(_WIN32) || defined(_WIN64)
            BOOL is_admin = FALSE;
            HANDLE token = NULL;
            if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token)) {
                TOKEN_ELEVATION elevation;
                DWORD size = sizeof(TOKEN_ELEVATION);
                if (GetTokenInformation(token, TokenElevation, &elevation, sizeof(elevation), &size)) {
                    is_admin = elevation.TokenIsElevated;
                }
            }
            if (token) {
                CloseHandle(token);
            }
            return static_cast<bool>(is_admin);
        #elif defined(__linux__) || (defined(__APPLE__) && defined(__MACH__))
            return (getuid() == 0);
        #else
            return false; // Unsupported platform, assume no privileges.
        #endif
    }

} // namespace phgit_installer::platform
