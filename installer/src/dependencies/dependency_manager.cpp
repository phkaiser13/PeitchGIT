// Copyright (C) 2025 Pedro Henrique / phkaiser13
// File: dependency_manager.cpp
// SPDX-License-Identifier: Apache-2.0
//
// Implementation of DependencyManager
// - Cross-platform search for executables on PATH
// - Executes "<exe> --version" (or similar) via ProcessExecutor
// - Parses semantic version from output and compares to minimum required
//
// The file aims to be robust and defensive; it logs useful diagnostics via spdlog.

#include "dependencies/dependency_manager.hpp"
#include "utils/process_executor.hpp" // expected to provide a result with exit_code, std_out, std_err

#include <spdlog/spdlog.h>

#include <filesystem>
#include <regex>
#include <sstream>
#include <system_error>
#include <cstdlib>      // getenv
#include <optional>
#include <string>
#include <vector>
#include <memory>
#include <algorithm>
#include <utility>      // std::move
#include <charconv>     // std::from_chars
#include <cctype>       // isspace
#include <mutex>

#if !defined(_WIN32) && !defined(_WIN64)
    #include <unistd.h>   // access, X_OK
    #include <sys/stat.h>
#else
    #include <windows.h>
#endif

namespace phgit_installer::dependencies {

namespace fs = std::filesystem;

//
// Helper functions (internal linkage)
//

namespace {

/** Trim whitespace from both ends of a string (in-place) */
static inline void trim_inplace(std::string &s) {
    // left trim
    size_t start = 0;
    while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start]))) ++start;
    if (start != 0) s.erase(0, start);

    // right trim
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back()))) s.pop_back();
}

/** Safely convert string segment to int using from_chars.
 *  Returns std::optional<int> (empty on conversion failure).
 */
static inline std::optional<int> safe_parse_int(const std::string &seg) {
    if (seg.empty()) return std::nullopt;
    int value = 0;
    const char* begin = seg.data();
    const char* end = seg.data() + seg.size();
    auto res = std::from_chars(begin, end, value);
    if (res.ec == std::errc() && res.ptr == end) {
        return value;
    }
    return std::nullopt;
}

/** Build a command string to query version from an executable path.
 *  We try to avoid unnecessary quoting; if the path contains whitespace we
 *  add quotes around the path. We also append a single argument `--version`.
 *  This string is compatible with ProcessExecutor::execute(const std::string&).
 */
static inline std::string build_version_command(const std::string &exe_path) {
    // Quote if path contains space or special chars
    bool need_quotes = exe_path.find_first_of(" \t()[]{}&^%$#@!`'\"") != std::string::npos;
    std::string cmd;
#if defined(_WIN32) || defined(_WIN64)
    if (need_quotes) cmd += '"';
    cmd += exe_path;
    if (need_quotes) cmd += '"';
    cmd += " --version";
#else
    // POSIX: avoid double quotes unless necessary
    if (need_quotes) {
        // Escape single quotes by wrapping in double quotes and escaping $
        // Simpler: wrap in single quotes, but that breaks embedded single quotes.
        // We'll wrap in double quotes and escape embedded double quotes.
        std::string escaped = exe_path;
        size_t pos = 0;
        while ((pos = escaped.find('"', pos)) != std::string::npos) {
            escaped.insert(pos, "\\");
            pos += 2;
        }
        cmd += '"' + escaped + "\" --version";
    } else {
        cmd += exe_path + " --version";
    }
#endif
    return cmd;
}

} // anonymous namespace

//
// DependencyManager implementation
//

DependencyManager::DependencyManager(platform::PlatformInfo info,
                                     std::shared_ptr<utils::ConfigManager> config) noexcept
    : m_platform_info(std::move(info)), m_config(std::move(config)) {
    spdlog::debug("[DependencyManager] initialized for OS: {}", m_platform_info.os_family);
}

void DependencyManager::check_all() {
    spdlog::info("[DependencyManager] Starting dependency checks.");
    std::vector<DependencyStatus> local_results;

    // Defensive: ensure config pointer is valid
    if (!m_config) {
        spdlog::error("[DependencyManager] ConfigManager pointer is null. Aborting checks.");
        std::lock_guard lock(m_mutex);
        m_dependency_statuses = std::move(local_results);
        return;
    }

    // Expecting ConfigManager::get_dependencies() to return a vector<DependencyRequirement>
    auto dependencies_to_check = m_config->get_dependencies();
    if (dependencies_to_check.empty()) {
        spdlog::warn("[DependencyManager] No dependencies declared by the configuration.");
        std::lock_guard lock(m_mutex);
        m_dependency_statuses = std::move(local_results);
        return;
    }

    for (const auto &dep : dependencies_to_check) {
        DependencyStatus status;
        status.name = dep.name;
        status.minimum_version = dep.min_version;
        status.is_required = dep.is_required;

        spdlog::debug("[DependencyManager] Checking '{}', required >= {}", status.name, status.minimum_version);

        auto exe_path_opt = find_executable_in_path(status.name);
        if (!exe_path_opt) {
            spdlog::warn("[DependencyManager] '{}' not found on PATH.", status.name);
            status.is_found = false;
            local_results.push_back(std::move(status));
            continue;
        }

        status.is_found = true;
        status.found_path = *exe_path_opt;
        spdlog::debug("[DependencyManager] Found {} at {}", status.name, status.found_path);

        // Build and execute version command
        std::string command = build_version_command(status.found_path);
        spdlog::trace("[DependencyManager] Executing: {}", command);

        // NOTE: we assume ProcessExecutor::execute(string) exists and returns a struct:
        // { int exit_code; std::string std_out; std::string std_err; }
        // If your ProcessExecutor has a different API (e.g. execute(argv...)) adapt here.
        auto proc_result = utils::ProcessExecutor::execute(command);

        if (proc_result.exit_code != 0) {
            spdlog::warn("[DependencyManager] Version command for '{}' failed (exit {}). Stderr: {}",
                         status.name, proc_result.exit_code, proc_result.std_err);
            local_results.push_back(std::move(status));
            continue;
        }

        std::string raw_output = !proc_result.std_out.empty() ? proc_result.std_out : proc_result.std_err;
        status.found_version = parse_version_from_output(raw_output);

        if (status.found_version.empty()) {
            spdlog::warn("[DependencyManager] Could not parse version for '{}' from output: {}",
                         status.name, raw_output);
            local_results.push_back(std::move(status));
            continue;
        }

        spdlog::debug("[DependencyManager] {} -> parsed version {}", status.name, status.found_version);
        status.is_version_ok = (compare_versions(status.found_version, status.minimum_version) >= 0);

        if (status.is_version_ok) {
            spdlog::info("[DependencyManager] '{}' OK: {} >= {}", status.name, status.found_version, status.minimum_version);
        } else {
            spdlog::warn("[DependencyManager] '{}' outdated: {} < {}", status.name, status.found_version, status.minimum_version);
        }

        local_results.push_back(std::move(status));
    }

    // Publish results under lock
    {
        std::lock_guard lock(m_mutex);
        m_dependency_statuses = std::move(local_results);
    }
}

std::optional<DependencyStatus> DependencyManager::get_status(const std::string &name) const {
    std::lock_guard lock(m_mutex);
    for (const auto &s : m_dependency_statuses) {
        if (s.name == name) return s;
    }
    return std::nullopt;
}

const std::vector<DependencyStatus>& DependencyManager::all_statuses() const noexcept {
    // Caller should ensure check_all() has been called; we return reference under lock only for atomic swap safety.
    return m_dependency_statuses;
}

bool DependencyManager::are_core_dependencies_met() const {
    std::lock_guard lock(m_mutex);
    for (const auto &s : m_dependency_statuses) {
        if (s.is_required && (!s.is_found || !s.is_version_ok)) return false;
    }
    return true;
}

std::optional<std::string> DependencyManager::find_executable_in_path(const std::string &name) const {
    const char *path_env = std::getenv("PATH");
    if (!path_env) return std::nullopt;

    std::string path_str(path_env);

#if defined(_WIN32) || defined(_WIN64)
    // On Windows, use PATHEXT to determine executable extensions.
    std::vector<std::string> exts;
    const char* pathext_env = std::getenv("PATHEXT");
    if (pathext_env && *pathext_env) {
        std::string pathext(pathext_env);
        std::stringstream ss(pathext);
        std::string token;
        while (std::getline(ss, token, ';')) {
            trim_inplace(token);
            if (!token.empty()) {
                // Ensure token starts with a dot
                if (token.front() != '.') token.insert(token.begin(), '.');
                std::transform(token.begin(), token.end(), token.begin(), [](unsigned char c){ return std::tolower(c); });
                exts.push_back(token);
            }
        }
    }
    if (exts.empty()) {
        exts = { ".exe", ".cmd", ".bat", ".com" };
    }
    const char delimiter = ';';
#else
    const std::vector<std::string> exts = { "" };
    const char delimiter = ':';
#endif

    std::stringstream ss(path_str);
    std::string path_item;
    while (std::getline(ss, path_item, delimiter)) {
        trim_inplace(path_item);
        if (path_item.empty()) continue;

        fs::path dir(path_item);

        for (const auto &ext : exts) {
            fs::path candidate = dir / (name + ext);
            std::error_code ec;
            if (!fs::exists(candidate, ec) || ec) continue;
            if (fs::is_directory(candidate, ec) || ec) continue;

#if defined(_WIN32) || defined(_WIN64)
            // On Windows we accept the file if it exists (executable status is implicit)
            return candidate.string();
#else
            // POSIX: check execute permission bits
            auto perms = fs::status(candidate, ec).permissions();
            if (ec) continue;
            // Check owner/group/others execute bits
            if ((perms & (fs::perms::owner_exec | fs::perms::group_exec | fs::perms::others_exec)) == fs::perms::none) {
                // As a fallback also check access(2) X_OK if available
                if (access(candidate.c_str(), X_OK) != 0) continue;
            }
            return candidate.string();
#endif
        }
    }

    return std::nullopt;
}

std::string DependencyManager::parse_version_from_output(const std::string &raw_output) const {
    // Accept optional 'v' prefix, capture X.Y or X.Y.Z (numeric only)
    // We return the captured numeric group.
    static const std::regex version_regex(R"((?:v)?(\d+\.\d+(?:\.\d+)?))", std::regex_constants::ECMAScript | std::regex_constants::icase);
    std::smatch match;
    if (std::regex_search(raw_output, match, version_regex) && match.size() > 1) {
        return match[1].str();
    }
    return {};
}

int DependencyManager::compare_versions(const std::string &v1, const std::string &v2) const {
    // Split by '.' and compare numeric components using safe_parse_int
    std::vector<int> a, b;
    std::string seg;
    std::stringstream ss1(v1);
    while (std::getline(ss1, seg, '.')) {
        trim_inplace(seg);
        auto val = safe_parse_int(seg);
        a.push_back(val.value_or(0));
    }
    std::stringstream ss2(v2);
    while (std::getline(ss2, seg, '.')) {
        trim_inplace(seg);
        auto val = safe_parse_int(seg);
        b.push_back(val.value_or(0));
    }

    size_t max_len = std::max(a.size(), b.size());
    a.resize(max_len, 0);
    b.resize(max_len, 0);

    for (size_t i = 0; i < max_len; ++i) {
        if (a[i] < b[i]) return -1;
        if (a[i] > b[i]) return 1;
    }
    return 0;
}

} // namespace phgit_installer::dependencies
