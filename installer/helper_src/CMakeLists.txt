# Copyright (C) 2025 Pedro Henrique / phkaiser13
# File: CMakeLists.txt
# This file defines the build system configuration for the installer_helper
# executable. It uses CMake, a cross-platform build system generator, to
# ensure the helper can be compiled on Windows, macOS, and Linux.
#
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.15)

project(installer_helper VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include FetchContent module to be able to download dependencies if needed.
include(FetchContent)

# --- Find cURL library ---
# This is required for all network operations.
find_package(CURL REQUIRED)

# --- Find nlohmann_json library ---
# First, try to find it on the system. If not found, download it using FetchContent.
# This mirrors the logic from the root project for consistency.
find_package(nlohmann_json 3.2.0 QUIET)
if(NOT nlohmann_json_FOUND)
    message(STATUS "nlohmann_json not found on system for helper. Fetching via FetchContent...")
    FetchContent_Declare(
        nlohmann_json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG v3.11.3 # Using a specific, tested version
    )
    FetchContent_MakeAvailable(nlohmann_json)
endif()
message(STATUS "Using nlohmann_json for JSON parsing.")

# --- Define the Executable Target ---
add_executable(installer_helper
    src/main.cpp
    src/dependency_manager.cpp
    src/downloader.cpp
)

# --- Link Libraries ---
# Link our executable against cURL (for downloading) and nlohmann_json (for parsing).
target_link_libraries(installer_helper PRIVATE
    CURL::libcurl
    nlohmann_json::nlohmann_json
)

# Set a clean output directory for the final executable.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

message(STATUS "Configured installer_helper build successfully.")