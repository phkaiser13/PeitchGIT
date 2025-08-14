# Copyright (C) 2025 Pedro Henrique / phkaiser13
# File: CMakeLists.txt
# This is the main CMake build script for the phgit installer. It defines the project,
# sets build properties, fetches external dependencies using FetchContent, and configures
# the installer executable. It also includes CPack configuration for generating
# platform-native packages, which will be expanded upon as platform-specific
# logic is implemented.
# SPDX-License-Identifier: Apache-2.0

# Require a modern version of CMake that supports FetchContent and other features.
cmake_minimum_required(VERSION 3.16)

# Define the project, its version, and the primary language.
# This information will be used by CPack for package metadata.
project(phgit_installer
    VERSION 1.0.0
    DESCRIPTION "A Git helper tool with integrated workflows for Terraform and Vault"
    LANGUAGES CXX
)

# --- Build Configuration ---
# Enforce C++17 standard, as specified in the technical requirements.
# This ensures access to features like std::filesystem and std::string_view.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF) # Disable compiler-specific extensions for better portability.

# --- Dependency Management (FetchContent) ---
# Use FetchContent to manage dependencies in a clean, self-contained way.
# This avoids requiring the user to pre-install libraries.
include(FetchContent)

# spdlog - For high-performance, feature-rich logging.
# We fetch it directly from its Git repository.
FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG        v1.14.1 # Use a specific, stable tag for reproducibility.
)
# Make the dependency available to the project. For a header-only library like spdlog,
# this sets up the necessary include directories.
FetchContent_MakeAvailable(spdlog)

# TODO: Add FetchContent declarations for nlohmann/json and libcurl when they are needed.

# --- Executable Target Definition ---
# Define the main installer executable.
# Source files will be added to this list as they are created.
add_executable(installer
    src/main.cpp
)

# Set a clear output name for the final binary.
set_target_properties(installer PROPERTIES OUTPUT_NAME "phgit-installer")

# Add the 'src' directory to the include path.
# This allows for cleaner includes, e.g., #include "core/component.h"
# instead of #include "src/core/component.h".
target_include_directories(installer PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

# --- Linking Dependencies ---
# Link the installer executable against its required libraries.
target_link_libraries(installer PRIVATE
    spdlog::spdlog # Link to the spdlog target created by FetchContent.
)

# --- Platform-Specific Settings ---
# This section will be expanded to handle compiler flags, static linking,
# and other platform-specific requirements.
if(MSVC)
    # For Windows (MSVC compiler), ensure we use the static runtime library in Release builds
    # to minimize external dependencies (msvcrt.dll).
    target_compile_options(installer PRIVATE
        $<$<CONFIG:RELEASE>:/MT>
        $<$<CONFIG:DEBUG>:/MTd>
    )
elseif(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_CLANG)
    # For GCC/Clang, add common warning flags for high-quality code.
    target_compile_options(installer PRIVATE
        -Wall -Wextra -Wpedantic
    )
endif()

if(UNIX AND NOT APPLE)
    # On Linux, we often need to link against pthread and dl for various system functions.
    target_link_libraries(installer PRIVATE pthread dl)
endif()

if(APPLE)
    # On macOS, set the minimum deployment target as specified.
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS deployment version")
endif()


# --- CPack Package Generation (Initial Setup) ---
# This section configures CPack, CMake's packaging tool. It will be used to generate
# .deb, .rpm, .msi, .dmg, etc.
set(CPACK_PACKAGE_NAME ${PROJECT_NAME})
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${PROJECT_DESCRIPTION}")
set(CPACK_PACKAGE_VENDOR "Pedro Henrique / phkaiser13")
set(CPACK_PACKAGE_CONTACT "phkaiser13@users.noreply.github.com")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/phkaiser13/phgit") # Placeholder URL
# Assume a LICENSE file exists in the root directory. CPack will include it.
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.txt")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "phgit")

# Define which installer generators to use. This will be refined later.
if(WIN32)
    set(CPACK_GENERATOR "NSIS")
elseif(UNIX AND NOT APPLE)
    # On Linux, we can generate multiple types. The build script will later choose one.
    set(CPACK_GENERATOR "DEB;RPM")
elseif(APPLE)
    # On macOS, DragNDrop creates a DMG image.
    set(CPACK_GENERATOR "DragNDrop")
endif()

# This command must be at the end of the file to include all CPack settings.
include(CPack)
