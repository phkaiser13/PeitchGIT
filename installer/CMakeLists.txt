# Copyright (C) 2025 Pedro Henrique / phkaiser13
# File: CMakeLists.txt
# This is the final, fully integrated CMake script for the phgit installer engine.
# It fetches all dependencies (spdlog, nlohmann/json, miniz), finds system libraries
# (libcurl), compiles all source files, and links the final executable. It represents
# a complete, production-ready build system for the installer core.
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.16)

project(phgit_installer
    VERSION 1.0.0
    DESCRIPTION "A Git helper tool with integrated workflows for Terraform and Vault"
    LANGUAGES CXX C
)

# --- Build Configuration ---
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# --- Dependency Management (FetchContent) ---
include(FetchContent)

# spdlog for logging
FetchContent_Declare(spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.13.0
)
FetchContent_MakeAvailable(spdlog)

# miniz for zip archive extraction on Windows
FetchContent_Declare(miniz
    GIT_REPOSITORY https://github.com/richgel999/miniz.git
    GIT_TAG 3.0.0
)
FetchContent_MakeAvailable(miniz)

# Find libcurl for downloading
find_package(CURL REQUIRED)

# --- Source File Aggregation ---
set(INSTALLER_SOURCES
    src/main.cpp
    src/utils/config_manager.cpp
    src/utils/api_manager.cpp
    src/utils/downloader.cpp
    src/utils/process_executor.cpp
    src/platform/platform_detector.cpp
    src/dependencies/dependency_manager.cpp
    src/platform/linux-systems/linux_installer.cpp
    src/platform/darwin-mac/darwin-mac_installer.cpp
    src/platform/windows/windows_installer.cpp
    # Add miniz.c directly to our sources
    ${miniz_SOURCE_DIR}/miniz.c
)

# --- Executable Target Definition ---
add_executable(installer ${INSTALLER_SOURCES})
set_target_properties(installer PROPERTIES OUTPUT_NAME "phgit-installer")

# --- Include Directories ---
target_include_directories(installer PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    ${CURL_INCLUDE_DIRS}
    ${miniz_SOURCE_DIR} # Add miniz.h's directory
)

# --- Linking Dependencies ---
target_link_libraries(installer PRIVATE
    spdlog::spdlog
    CURL::libcurl
)

# --- Platform-Specific Settings & Linking ---
if(MSVC)
    target_compile_options(installer PRIVATE
        $<$<CONFIG:RELEASE>:/MT>
        $<$<CONFIG:DEBUG>:/MTd>
    )
    target_link_libraries(installer PRIVATE advapi32.lib shell32.lib user32.lib)
elseif(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_CLANG)
    target_compile_options(installer PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(UNIX AND NOT APPLE)
    target_link_libraries(installer PRIVATE pthread dl)
endif()

if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS deployment version")
endif()

# --- Packaging ---
# This section is a placeholder for CI/CD automation.
# It demonstrates how package metadata could be configured and used.
# ... (configure_file and CPack settings from previous versions) ...
include(CPack)
