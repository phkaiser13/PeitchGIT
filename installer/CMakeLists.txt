# Copyright (C) 2025 Pedro Henrique / phkaiser13
# File: installer/CMakeLists.txt
# This CMake script configures the build system for the phgit-installer executable.
# It is responsible for defining the project, managing dependencies, selecting
# platform-specific source files, and configuring the compiler, linker, and
# installation rules. It ensures a robust, cross-platform build and deployment
# process for Windows, macOS, and Linux.
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

if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# --- Dependency Management (FetchContent) ---
include(FetchContent)

FetchContent_Declare(spdlog
        GIT_REPOSITORY https://github.com/gabime/spdlog.git
        GIT_TAG v1.13.0
)
FetchContent_MakeAvailable(spdlog)

FetchContent_Declare(miniz
        GIT_REPOSITORY https://github.com/richgel999/miniz.git
        GIT_TAG v3.0.2
)
FetchContent_MakeAvailable(miniz)

find_package(CURL REQUIRED)

# --- Static Library for Miniz ---
add_library(miniz_lib STATIC
        ${miniz_SOURCE_DIR}/miniz.c
        ${miniz_SOURCE_DIR}/miniz_zip.c
)
target_include_directories(miniz_lib PUBLIC "${miniz_SOURCE_DIR}")

# --- Source File Aggregation ---
set(INSTALLER_SOURCES
        src/main.cpp
        src/utils/sha256.cpp
        src/utils/config_manager.cpp
        src/utils/api_manager.cpp
        src/utils/downloader.cpp
        src/utils/process_executor.cpp
        src/platform/platform_detector.cpp
        src/dependencies/dependency_manager.cpp
)

# Conditionally append the platform-specific installer implementation.
if(WIN32)
    list(APPEND INSTALLER_SOURCES src/platform/windows/windows_installer.cpp)
elseif(APPLE)
    list(APPEND INSTALLER_SOURCES src/platform/darwin-mac/darwin-mac_installer.cpp)
elseif(UNIX)
    list(APPEND INSTALLER_SOURCES src/platform/linux-systems/linux_installer.cpp)
else()
    message(FATAL_ERROR "Unsupported platform for the installation engine.")
endif()

# --- Executable Target Definition & Properties ---
add_executable(installer ${INSTALLER_SOURCES})
set_target_properties(installer PROPERTIES OUTPUT_NAME "phgit-installer")

# Include GNU standard installation directories (e.g., CMAKE_INSTALL_BINDIR).
include(GNUInstallDirs)

# Inject the installation data path into the C++ code as a preprocessor macro.
# This allows the executable to find its config file in a production environment.
# The path will resolve to something like "/usr/share/phgit" on Linux.
target_compile_definitions(installer PRIVATE
        "INSTALL_DATA_DIR=\"${CMAKE_INSTALL_DATADIR}/phgit\""
)

# --- Include Directories ---
target_include_directories(installer PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
        ${CURL_INCLUDE_DIRS}
)

# --- Linking Dependencies ---
target_link_libraries(installer PRIVATE
        spdlog::spdlog
        CURL::libcurl
        miniz_lib
)

# --- Platform-Specific Settings & Linking ---
if(MSVC)
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

# --- Build Type & Optimizations ---
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    if(MSVC)
        target_compile_options(installer PRIVATE /O2 /DNDEBUG)
    else()
        target_compile_options(installer PRIVATE -O3 -DNDEBUG)
    endif()
endif()

# --- Post-Build Command for Development ---
# Copies config.json to the build output directory. This ensures the executable
# can run directly from the build folder for testing and development.
add_custom_command(TARGET installer POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        "${CMAKE_CURRENT_SOURCE_DIR}/config.json"
        "$<TARGET_FILE_DIR:installer>/config.json"
        COMMENT "Copying config.json to the build output directory for development"
)

# --- Installation Rules for Production ---
# These rules define what happens when `cmake --install .` is run.

# 1. Install the main executable to the standard binary location (e.g., /usr/bin).
install(TARGETS installer
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# 2. Install the configuration file to the data directory we defined earlier.
# This is the CRITICAL step that makes the INSTALL_DATA_DIR macro work in production.
install(FILES config.json
        DESTINATION ${CMAKE_INSTALL_DATADIR}/phgit
)

# --- Packaging (CPack) ---
# CPack uses the installation rules above to create packages (DEB, RPM, ZIP, etc.).
set(CPACK_PACKAGE_NAME "phgit-installer")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${PROJECT_DESCRIPTION}")
set(CPACK_PACKAGE_VENDOR "Pedro Henrique / phkaiser13")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/../LICENSE")

if(WIN32)
    set(CPACK_GENERATOR "ZIP")
else()
    set(CPACK_GENERATOR "TGZ")
endif()

include(CPack)