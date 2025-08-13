# Copyright (C) 2025 Pedro Henrique / phkaiser13
# File: CMakeLists.txt
# This file defines the build system configuration for the installer_helper
# executable. It uses CMake, a cross-platform build system generator, to
# ensure the helper can be compiled on Windows, macOS, and Linux.
#
# Responsibilities:
# 1. Define the project and required C++ standard.
# 2. Find necessary third-party libraries (like cURL for networking).
# 3. Specify the source files to be compiled.
# 4. Create the final executable target ('installer_helper').
#
# SPDX-License-Identifier: Apache-2.0

# Set the minimum required version of CMake.
cmake_minimum_required(VERSION 3.15)

# Define the project name, version, and language.
project(installer_helper VERSION 1.0 LANGUAGES CXX)

# Set the C++ standard to C++17. This gives us access to modern features
# like std::filesystem, which is useful for path manipulation.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find the cURL library, which is essential for downloading dependencies.
# The 'REQUIRED' keyword will cause CMake to fail if cURL is not found,
# preventing a broken build.
find_package(CURL REQUIRED)

# Define the executable target.
# The executable will be named 'installer_helper'.
add_executable(installer_helper
    src/main.cpp
    src/dependency_manager.cpp
    src/downloader.cpp
)

# Link the cURL library to our executable.
# This makes the cURL functions available to our code.
target_link_libraries(installer_helper PRIVATE CURL::libcurl)

# Set output directory for the executable to be in a 'bin' folder
# relative to the build directory. This keeps the build artifacts organized.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Print a status message to the console during configuration.
message(STATUS "Configured installer_helper build. cURL found at: ${CURL_LIBRARIES}")