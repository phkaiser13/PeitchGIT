/* Copyright (C) 2025 Pedro Henrique / phkaiser13
* File: dependency_manager.cpp
* This file implements the DependencyManager class. It provides the logic for finding
* executables in the system's PATH, running them to query their version, and comparing
* the result against a minimum requirement. It uses std::filesystem for path manipulation
* and platform-specific APIs for process creation (`popen` on POSIX, `CreateProcess` on
* Windows) to ensure robust, cross-platform compatibility and performance.
* SPDX-License-Identifier: Apache-2.0
*/

#include "dependencies/dependency_manager.hpp"
#include "spdlog/spdlog.h"

#include <array>
#include <filesystem>
#include <regex>
#include <sstream>
#include <stdexcept>

// Platform-specific includes for process execution
#if defined(_WIN32) || defined(_WIN64)
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
#else
    #include <cstdio> // For popen
#endif

namespace phgit_installer::dependencies {

    namespace fs = std::filesystem;

    DependencyManager::DependencyManager(platform::PlatformInfo info)
        : m_platform_info(std::move(info)) {
        spdlog::debug("DependencyManager initialized for OS: {}", m_platform_info.os_family);
    }

    void DependencyManager::check_all() {
        spdlog::info("Starting check for all external dependencies.");
        m_dependency_statuses.clear();
        
        // Define all dependencies to be checked
        check_dependency("git", "2.20.0", true);
        check_dependency("terraform", "1.0.0", false);
        check_dependency("vault", "1.10.0", false);
    }

    std::optional<DependencyStatus> DependencyManager::get_status(const std::string& name) const {
        for (const auto& status : m_dependency_statuses) {
            if (status.name == name) {
                return status;
            }
        }
        return std::nullopt;
    }

    bool DependencyManager::are_core_dependencies_met() const {
        for (const auto& status : m_dependency_statuses) {
            if (status.is_required && (!status.is_found || !status.is_version_ok)) {
                return false; // A required dependency is missing or outdated.
            }
        }
        return true;
    }

    void DependencyManager::check_dependency(const std::string& name, const std::string& min_version, bool is_required) {
        DependencyStatus status;
        status.name = name;
        status.minimum_version = min_version;
        status.is_required = is_required;

        spdlog::debug("Checking dependency: {}", name);

        auto executable_path = find_executable_in_path(name);
        if (!executable_path) {
            spdlog::warn("Dependency '{}' not found in PATH.", name);
            status.is_found = false;
            m_dependency_statuses.push_back(status);
            return;
        }

        status.is_found = true;
        status.found_path = *executable_path;
        spdlog::debug("Found '{}' at: {}", name, status.found_path);

        auto version_output = get_output_from_command("\"" + status.found_path + "\" --version");
        if (!version_output) {
            spdlog::warn("Could not execute '{} --version'. Unable to determine version.", name);
            m_dependency_statuses.push_back(status);
            return;
        }

        status.found_version = parse_version_from_output(*version_output);
        if (status.found_version.empty()) {
            spdlog::warn("Could not parse version for '{}' from output: {}", name, *version_output);
            m_dependency_statuses.push_back(status);
            return;
        }

        spdlog::debug("Found version for '{}': {}", name, status.found_version);
        status.is_version_ok = (compare_versions(status.found_version, status.minimum_version) >= 0);

        if (status.is_version_ok) {
            spdlog::info("Dependency '{}' is OK (version {}, required >= {}).", name, status.found_version, status.minimum_version);
        } else {
            spdlog::warn("Dependency '{}' is outdated (version {}, required >= {}).", name, status.found_version, status.minimum_version);
        }

        m_dependency_statuses.push_back(status);
    }

    std::optional<std::string> DependencyManager::find_executable_in_path(const std::string& name) {
        const char* path_env = std::getenv("PATH");
        if (!path_env) {
            return std::nullopt;
        }

        std::string path_str(path_env);
        #if defined(_WIN32) || defined(_WIN64)
            const char delimiter = ';';
            const std::vector<std::string> extensions = { ".exe", ".cmd", ".bat" };
            const std::string exe_name = name + ".exe"; // Common case
        #else
            const char delimiter = ':';
            const std::vector<std::string> extensions = { "" };
            const std::string& exe_name = name;
        #endif

        std::stringstream ss(path_str);
        std::string path_item;
        while (std::getline(ss, path_item, delimiter)) {
            if (path_item.empty()) continue;

            for (const auto& ext : extensions) {
                fs::path full_path = fs::path(path_item) / (name + ext);
                std::error_code ec;
                if (fs::exists(full_path, ec) && !fs::is_directory(full_path, ec) && (fs::status(full_path, ec).permissions() & fs::perms::owner_exec) != fs::perms::none) {
                    return full_path.string();
                }
            }
        }
        return std::nullopt;
    }

    std::optional<std::string> DependencyManager::get_output_from_command(const std::string& command) {
        #if defined(_WIN32) || defined(_WIN64)
            // More complex but robust implementation for Windows to avoid console windows.
            HANDLE h_child_stdout_rd = NULL;
            HANDLE h_child_stdout_wr = NULL;
            SECURITY_ATTRIBUTES sa;
            sa.nLength = sizeof(SECURITY_ATTRIBUTES);
            sa.bInheritHandle = TRUE;
            sa.lpSecurityDescriptor = NULL;

            if (!CreatePipe(&h_child_stdout_rd, &h_child_stdout_wr, &sa, 0)) return std::nullopt;
            if (!SetHandleInformation(h_child_stdout_rd, HANDLE_FLAG_INHERIT, 0)) return std::nullopt;

            PROCESS_INFORMATION pi;
            STARTUPINFOA si;
            ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));
            ZeroMemory(&si, sizeof(STARTUPINFOA));
            si.cb = sizeof(STARTUPINFOA);
            si.hStdError = h_child_stdout_wr;
            si.hStdOutput = h_child_stdout_wr;
            si.dwFlags |= STARTF_USESTDHANDLES;

            std::vector<char> cmd_vec(command.begin(), command.end());
            cmd_vec.push_back(0);

            if (!CreateProcessA(NULL, cmd_vec.data(), NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
                CloseHandle(h_child_stdout_rd);
                CloseHandle(h_child_stdout_wr);
                return std::nullopt;
            }

            CloseHandle(h_child_stdout_wr); // Close the write end of the pipe in the parent process.

            std::string result;
            std::array<char, 128> buffer;
            DWORD bytes_read;
            while (ReadFile(h_child_stdout_rd, buffer.data(), buffer.size(), &bytes_read, NULL) && bytes_read != 0) {
                result.append(buffer.data(), bytes_read);
            }

            CloseHandle(h_child_stdout_rd);
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);

            // Return the first line of the output
            return result.substr(0, result.find_first_of("\r\n"));
        #else
            // Simpler implementation for POSIX systems.
            std::string result;
            std::array<char, 128> buffer;
            // Redirect stderr to stdout to capture version info from commands that print there.
            std::string cmd_with_redirect = command + " 2>&1";
            FILE* pipe = popen(cmd_with_redirect.c_str(), "r");
            if (!pipe) {
                return std::nullopt;
            }
            while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
                result += buffer.data();
            }
            pclose(pipe);
            // Return the first line of the output
            return result.substr(0, result.find_first_of("\n"));
        #endif
    }

    std::string DependencyManager::parse_version_from_output(const std::string& raw_output) {
        // Regex to find patterns like X.Y.Z or vX.Y.Z
        std::regex version_regex("(\\d+\\.\\d+\\.\\d+)");
        std::smatch match;
        if (std::regex_search(raw_output, match, version_regex) && match.size() > 1) {
            return match[1].str();
        }
        return "";
    }

    int DependencyManager::compare_versions(const std::string& v1, const std::string& v2) {
        std::stringstream ss1(v1);
        std::stringstream ss2(v2);
        std::string segment;
        std::vector<int> v1_parts, v2_parts;

        while(std::getline(ss1, segment, '.')) v1_parts.push_back(std::stoi(segment));
        while(std::getline(ss2, segment, '.')) v2_parts.push_back(std::stoi(segment));

        size_t max_size = std::max(v1_parts.size(), v2_parts.size());
        v1_parts.resize(max_size, 0);
        v2_parts.resize(max_size, 0);

        for (size_t i = 0; i < max_size; ++i) {
            if (v1_parts[i] < v2_parts[i]) return -1;
            if (v1_parts[i] > v2_parts[i]) return 1;
        }
        return 0;
    }

} // namespace phgit_installer::dependencies
