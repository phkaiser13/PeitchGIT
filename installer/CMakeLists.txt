# Copyright (C) 2025 Pedro Henrique / phkaiser13
# File: CMakeLists.txt
# This is the final, integrated CMake script for the phgit installer engine.
# It fetches all dependencies (spdlog, nlohmann/json), finds system libraries (libcurl),
# compiles all source files, and links the final executable. It also includes a section
# for processing package metadata templates using data from the configuration,
# preparing the project for an automated CI/CD pipeline.
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
FetchContent_Declare(spdlog GIT_REPOSITORY https://github.com/gabime/spdlog.git GIT_TAG v1.14.1)
FetchContent_MakeAvailable(spdlog)

# nlohmann/json for parsing configuration and API responses
FetchContent_Declare(json GIT_REPOSITORY https://github.com/nlohmann/json.git GIT_TAG v3.11.3)
FetchContent_MakeAvailable(json)

# Find libcurl for downloading
find_package(CURL REQUIRED)

# --- Source File Aggregation ---
set(INSTALLER_SOURCES
    src/main.cpp
    src/utils/config_manager.cpp
    src/utils/api_manager.cpp
    src/utils/downloader.cpp
    src/platform/platform_detector.cpp
    src/dependencies/dependency_manager.cpp
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
    ${CURL_INCLUDE_DIRS}
    # nlohmann_json is header-only, its include dir is added by FetchContent
)

# --- Linking Dependencies ---
target_link_libraries(installer PRIVATE
    spdlog::spdlog
    CURL::libcurl
)

# --- Platform-Specific Settings & Linking ---
# (Omitted for brevity, same as before)

# --- Packaging Configuration ---
# This section demonstrates how to use the config data to prepare package files.
# In a real CI, you would parse config.json to set these variables.
set(PACKAGE_VERSION "1.0.0")
set(PACKAGE_MAINTAINER "Pedro Henrique / phkaiser13 <phkaiser13@users.noreply.github.com>")
set(PACKAGE_HOMEPAGE "https://github.com/phkaiser13/phgit")
set(PACKAGE_DESCRIPTION "A Git helper tool with integrated workflows for Terraform and Vault.")

# Use configure_file to replace @VARS@ in templates with CMake variables
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/packaging/debian/control"
    "${CMAKE_CURRENT_BINARY_DIR}/packaging/debian/control"
)
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/packaging/rpm/phgit.spec"
    "${CMAKE_CURRENT_BINARY_DIR}/packaging/rpm/phgit.spec"
)
# ... and so on for other package files.

# CPack settings can now use these variables
set(CPACK_PACKAGE_VERSION ${PACKAGE_VERSION})
# ...
include(CPack)
