/* Copyright (C) 2025 Pedro Henrique / phkaiser13
* File: dependency_manager.cpp
* Implementation of DependencyManager. This file is responsible for the concrete
* logic of finding executables on the system's PATH, executing them to query
* their version, parsing the output, and comparing it against a required
* minimum version. The implementation is designed to be cross-platform, robust,
* and performant, minimizing allocations and leveraging modern C++ features
* for safety and speed. It uses spdlog for detailed diagnostics.
* SPDX-License-Identifier: Apache-2.0 */

#include "dependencies/dependency_manager.hpp"

// We need the full definition of ConfigManager to call its methods.
// This was the primary source of the compilation error "incomplete type".
#include "utils/config_manager.hpp"

// We assume ProcessExecutor provides a simple, static `execute` method.
#include "utils/process_executor.hpp"

#include <spdlog/spdlog.h>

#include <filesystem>
#include <regex>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>
#include <memory>
#include <optional>
#include <algorithm>
#include <utility>      // std::move
#include <system_error>
#include <cstdlib>      // std::getenv
#include <charconv>     // std::from_chars for high-performance parsing
#include <cctype>       // std::isspace
#include <mutex>

// Platform-specific includes for executable checks
#if !defined(_WIN32) && !defined(_WIN64)
    #include <unistd.h>   // access, X_OK
#endif

namespace phgit_installer::dependencies {

// Filesystem alias for brevity
namespace fs = std::filesystem;

//
// Anonymous namespace for internal linkage helper functions.
// These are implementation details not visible outside this translation unit.
//
namespace {

/**
 * @brief Trim whitespace from both ends of a string_view.
 * This is a non-mutating, zero-allocation version.
 * @param sv The string_view to trim.
 * @return A trimmed string_view.
 */
[[nodiscard]] std::string_view trim_view(std::string_view sv) {
    const auto first = sv.find_first_not_of(" \t\n\r\f\v");
    if (first == std::string_view::npos) {
        return {}; // The string is all whitespace
    }
    const auto last = sv.find_last_not_of(" \t\n\r\f\v");
    return sv.substr(first, (last - first + 1));
}

/**
 * @brief Safely convert a string_view segment to an integer.
 * Uses std::from_chars for maximum performance (non-allocating, locale-independent).
 * @param sv The string_view containing the number.
 * @return An optional containing the integer, or std::nullopt on failure.
 */
[[nodiscard]] std::optional<int> safe_parse_int(std::string_view sv) {
    if (sv.empty()) {
        return std::nullopt;
    }
    int value = 0;
    auto res = std::from_chars(sv.data(), sv.data() + sv.size(), value);
    if (res.ec == std::errc() && res.ptr == sv.data() + sv.size()) {
        return value;
    }
    return std::nullopt;
}

/**
 * @brief Build a command string to query the version from an executable path.
 * Quotes the executable path if it contains spaces to ensure correct execution.
 * @param exe_path The full path to the executable.
 * @return A string formatted as "<quoted_path> --version".
 */
[[nodiscard]] std::string build_version_command(const std::string& exe_path) {
    std::string cmd;
    // Pre-allocate to avoid reallocations for typical paths.
    cmd.reserve(exe_path.length() + 15);

    // Quote if path contains a space. This is a simple but effective heuristic.
    if (exe_path.find(' ') != std::string::npos) {
        cmd += '"';
        cmd += exe_path;
        cmd += '"';
    } else {
        cmd += exe_path;
    }
    cmd += " --version";
    return cmd;
}

} // anonymous namespace

//
// DependencyManager public method implementations
//

DependencyManager::DependencyManager(platform::PlatformInfo info,
                                     std::shared_ptr<utils::ConfigManager> config) noexcept
    : m_platform_info(std::move(info)), m_config(std::move(config)) {
    spdlog::debug("[DependencyManager] Initialized for OS: {}", m_platform_info.os_family);
}

void DependencyManager::check_all() {
    spdlog::info("[DependencyManager] Starting dependency checks.");
    std::vector<DependencyStatus> local_results;

    if (!m_config) {
        spdlog::error("[DependencyManager] ConfigManager is null. Aborting checks.");
        std::lock_guard lock(m_mutex);
        m_dependency_statuses.clear(); // Ensure consistent state
        return;
    }

    const auto dependencies_to_check = m_config->get_dependencies();
    if (dependencies_to_check.empty()) {
        spdlog::warn("[DependencyManager] No dependencies were specified in the configuration.");
        std::lock_guard lock(m_mutex);
        m_dependency_statuses.clear();
        return;
    }

    local_results.reserve(dependencies_to_check.size());

    for (const auto& req : dependencies_to_check) {
        DependencyStatus status;
        status.name = req.name;
        status.minimum_version = req.min_version;
        status.is_required = req.is_required;

        spdlog::debug("[DependencyManager] Checking '{}', required version >= {}", status.name, status.minimum_version);

        auto exe_path_opt = find_executable_in_path(status.name);
        if (!exe_path_opt) {
            spdlog::warn("[DependencyManager] '{}' not found in system PATH.", status.name);
            status.is_found = false;
            local_results.push_back(std::move(status));
            continue;
        }

        status.is_found = true;
        status.found_path = *exe_path_opt;
        spdlog::debug("[DependencyManager] Found '{}' at: {}", status.name, status.found_path);

        std::string command = build_version_command(status.found_path);
        spdlog::trace("[DependencyManager] Executing command: {}", command);

        // Assumes ProcessExecutor::execute returns a struct like:
        // { int exit_code; std::string std_out; std::string std_err; }
        auto proc_result = utils::ProcessExecutor::execute(command);

        if (proc_result.exit_code != 0) {
            spdlog::warn("[DependencyManager] Version command for '{}' failed with exit code {}. Stderr: {}",
                         status.name, proc_result.exit_code, trim_view(proc_result.std_err));
            local_results.push_back(std::move(status));
            continue;
        }

        // Some tools print version to stdout, others to stderr. We check both.
        std::string raw_output = !proc_result.std_out.empty() ? proc_result.std_out : proc_result.std_err;
        status.found_version = parse_version_from_output(raw_output);

        if (status.found_version.empty()) {
            spdlog::warn("[DependencyManager] Could not parse version for '{}' from output: {}",
                         status.name, trim_view(raw_output));
            local_results.push_back(std::move(status));
            continue;
        }

        spdlog::debug("[DependencyManager] '{}' -> parsed version '{}'", status.name, status.found_version);
        status.is_version_ok = (compare_versions(status.found_version, status.minimum_version) >= 0);

        if (status.is_version_ok) {
            spdlog::info("[DependencyManager] OK: '{}' version {} meets requirement >= {}", status.name, status.found_version, status.minimum_version);
        } else {
            spdlog::warn("[DependencyManager] OUTDATED: '{}' version {} is below requirement >= {}", status.name, status.found_version, status.minimum_version);
        }

        local_results.push_back(std::move(status));
    }

    // Atomically update the shared state with the new results.
    {
        std::lock_guard lock(m_mutex);
        m_dependency_statuses = std::move(local_results);
    }
}

std::optional<DependencyStatus> DependencyManager::get_status(const std::string& name) const {
    std::lock_guard lock(m_mutex);
    auto it = std::find_if(m_dependency_statuses.begin(), m_dependency_statuses.end(),
                           [&name](const DependencyStatus& s) { return s.name == name; });

    if (it != m_dependency_statuses.end()) {
        return *it;
    }
    return std::nullopt;
}

const std::vector<DependencyStatus>& DependencyManager::all_statuses() const noexcept {
    // This method is noexcept and lock-free because it returns a const reference.
    // The caller must not hold onto this reference across mutating calls like check_all().
    // The mutex in other methods ensures that reads from this vector are safe.
    return m_dependency_statuses;
}

bool DependencyManager::are_core_dependencies_met() const {
    std::lock_guard lock(m_mutex);
    return std::all_of(m_dependency_statuses.begin(), m_dependency_statuses.end(),
                       [](const DependencyStatus& s) {
                           // A dependency is met if it's not required, OR if it is found and its version is OK.
                           return !s.is_required || (s.is_found && s.is_version_ok);
                       });
}

//
// DependencyManager private helper implementations
//

std::optional<std::string> DependencyManager::find_executable_in_path(const std::string& name) const {
    const char* path_env = std::getenv("PATH");
    if (!path_env) {
        spdlog::error("[DependencyManager] PATH environment variable not found.");
        return std::nullopt;
    }

#if defined(_WIN32) || defined(_WIN64)
    const char delimiter = ';';
    std::vector<std::string> exts;
    const char* pathext_env = std::getenv("PATHEXT");
    if (pathext_env) {
        std::string pathext_str(pathext_env);
        std::stringstream ss(pathext_str);
        std::string token;
        while (std::getline(ss, token, ';')) {
            if (!token.empty()) {
                std::transform(token.begin(), token.end(), token.begin(),
                               [](unsigned char c){ return std::tolower(c); });
                exts.push_back(token);
            }
        }
    }
    if (exts.empty()) { // Provide a sensible default
        exts = {".exe", ".cmd", ".bat", ".com"};
    }
#else
    const char delimiter = ':';
    const std::vector<std::string> exts = {""}; // On POSIX, names usually have no extension
#endif

    std::stringstream path_stream(path_env);
    std::string path_item;
    while (std::getline(path_stream, path_item, delimiter)) {
        if (path_item.empty()) continue;

        fs::path dir(path_item);
        for (const auto& ext : exts) {
            fs::path candidate = dir / (name + ext);
            std::error_code ec;

            if (fs::exists(candidate, ec) && !ec && !fs::is_directory(candidate, ec)) {
#if defined(_WIN32) || defined(_WIN64)
                // On Windows, existence is sufficient.
                return candidate.string();
#else
                // On POSIX, we must also check for execute permissions.
                if (::access(candidate.c_str(), X_OK) == 0) {
                    return candidate.string();
                }
#endif
            }
        }
    }

    return std::nullopt;
}

std::string DependencyManager::parse_version_from_output(const std::string& raw_output) const {
    // This regex is compiled only once and reused, which is highly efficient.
    // It looks for a pattern like "v1.2.3" or "1.2.3" and captures the numeric part.
    // It robustly handles major.minor or major.minor.patch versions.
    static const std::regex version_regex(
        R"((?:[^\d]|^)(v?(?:\d+\.\d+)(?:\.\d+)?))",
        std::regex_constants::ECMAScript | std::regex_constants::icase
    );

    std::smatch match;
    if (std::regex_search(raw_output, match, version_regex) && match.size() > 1) {
        return match[1].str();
    }
    return {};
}

int DependencyManager::compare_versions(const std::string& v1, const std::string& v2) const {
    std::string_view v1_sv(v1);
    std::string_view v2_sv(v2);

    while (!v1_sv.empty() || !v2_sv.empty()) {
        auto parse_component = [](std::string_view& sv) {
            size_t dot_pos = sv.find('.');
            std::string_view component_sv = sv.substr(0, dot_pos);
            sv.remove_prefix(dot_pos == std::string_view::npos ? sv.size() : dot_pos + 1);
            return safe_parse_int(component_sv).value_or(0);
        };

        int part1 = v1_sv.empty() ? 0 : parse_component(v1_sv);
        int part2 = v2_sv.empty() ? 0 : parse_component(v2_sv);

        if (part1 < part2) return -1;
        if (part1 > part2) return 1;
    }

    return 0; // Versions are equal
}

} // namespace phgit_installer::dependencies