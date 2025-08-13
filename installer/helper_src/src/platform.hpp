/* Copyright (C) 2025 Pedro Henrique / phkaiser13
 * File: platform.hpp
 * This header file provides a clean, centralized, and type-safe mechanism for
 * detecting the current operating system and CPU architecture at compile time.
 * It abstracts away platform-specific preprocessor macros into easy-to-use
 * enums and constexpr variables. This helps in writing cleaner, more readable,
 * and more maintainable cross-platform C++ code.
 *
 * SPDX-License-Identifier: Apache-2.0 */

#pragma once

namespace ph::platform {

/**
 * @enum OperatingSystem
 * @brief Defines the supported operating systems in a type-safe manner.
 */
enum class OperatingSystem {
    Windows,
    MacOS,
    Linux,
    Unsupported
};

/**
 * @enum Architecture
 * @brief Defines the supported CPU architectures in a type-safe manner.
 */
enum class Architecture {
    x64,        // Also known as amd64 or x86_64
    ARM64,      // Also known as aarch64
    Unsupported
};

/**
 * @var currentOS
 * @brief A compile-time constant representing the current operating system.
 *
 * This variable is evaluated at compile time, ensuring zero runtime overhead
 * for platform detection.
 */
constexpr OperatingSystem currentOS =
#if defined(_WIN32)
    OperatingSystem::Windows;
#elif defined(__APPLE__)
    OperatingSystem::MacOS;
#elif defined(__linux__)
    OperatingSystem::Linux;
#else
    OperatingSystem::Unsupported;
#endif

/**
 * @var currentArch
 * @brief A compile-time constant representing the current CPU architecture.
 *
 * This variable is evaluated at compile time, ensuring zero runtime overhead
 * for architecture detection.
 */
constexpr Architecture currentArch =
#if defined(_M_X64) || defined(__x86_64__)
    Architecture::x64;
#elif defined(_M_ARM64) || defined(__aarch64__)
    Architecture::ARM64;
#else
    Architecture::Unsupported;
#endif

} // namespace ph::platform