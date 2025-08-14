# Copyright (C) 2025 Pedro Henrique / phkaiser13
# File: CMakeLists.txt
# This is the main CMake build script for the phgit installer. It now includes finding
# and linking against libcurl for download capabilities, in addition to managing all
# other source files and platform-specific settings.
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.16)

project(phgit_installer
    VERSION 1.0.0
    DESCRIPTION "A Git helper tool with integrated workflows for Terraform and Vault"
    LANGUAGES CXX
)

# --- Build Configuration ---
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# --- Dependency Management ---
include(FetchContent)

# spdlog for logging
FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG        v1.14.1
)
FetchContent_MakeAvailable(spdlog)

# Find libcurl for downloading
# This uses CMake's built-in FindCURL module, which works well on most systems.
find_package(CURL REQUIRED)

# --- Source File Aggregation ---
set(INSTALLER_SOURCES
    src/main.cpp
    src/platform/platform_detector.cpp
    src/dependencies/dependency_manager.cpp
    src/utils/downloader.cpp # Added downloader source
    src/platform/linux-systems/linux_installer.cpp
    src/platform/darwin-mac/darwin-mac_installer.cpp
    src/platform/windows/windows_installer.cpp
)

# --- Executable Target Definition ---
add_executable(installer ${INSTALLER_SOURCES})
set_target_properties(installer PROPERTIES OUTPUT_NAME "phgit-installer")

# --- Include Directories ---
target_include_directories(installer PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    ${CURL_INCLUDE_DIRS} # Add curl's include directory
)

# --- Linking Dependencies ---
target_link_libraries(installer PRIVATE
    spdlog::spdlog
    CURL::libcurl # Link against the imported libcurl target
)

# --- Platform-Specific Settings & Linking ---
if(MSVC)
    target_compile_options(installer PRIVATE
        $<$<CONFIG:RELEASE>:/MT>
        $<$<CONFIG:DEBUG>:/MTd>
    )
    target_link_libraries(installer PRIVATE advapi32.lib shell32.lib user32.lib)
elseif(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_CLANG)
    target_compile_options(installer PRIVATE
        -Wall -Wextra -Wpedantic
    )
endif()

if(UNIX AND NOT APPLE)
    target_link_libraries(installer PRIVATE pthread dl)
endif()

if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS deployment version")
endif()

# --- CPack Package Generation ---
# (No changes here, but kept for completeness)
set(CPACK_PACKAGE_NAME ${PROJECT_NAME})
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
# ... other CPack settings ...
include(CPack)
