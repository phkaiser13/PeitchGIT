# Copyright (C) 2025 Pedro Henrique / phkaiser13
# File: CMakeLists.txt
# This is the main CMake build script for the phgit installer. It defines the project,
# sets build properties, fetches external dependencies, and configures the installer
# executable. It now includes all source files for the various platform-specific
# components and links against required system libraries.
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

# --- Dependency Management (FetchContent) ---
include(FetchContent)

FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG        v1.14.1
)
FetchContent_MakeAvailable(spdlog)

# --- Source File Aggregation ---
# List all source files that make up the installer executable.
# This makes it easy to manage the file list as the project grows.
set(INSTALLER_SOURCES
    src/main.cpp
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
# Add the 'src' directory and its subdirectories to the include path.
# This allows for cleaner includes like #include "platform/platform_detector.hpp".
target_include_directories(installer PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

# --- Linking Dependencies ---
# Link against third-party and system libraries.
target_link_libraries(installer PRIVATE
    spdlog::spdlog
)

# --- Platform-Specific Settings & Linking ---
if(MSVC)
    # For Windows (MSVC compiler)
    target_compile_options(installer PRIVATE
        $<$<CONFIG:RELEASE>:/MT>
        $<$<CONFIG:DEBUG>:/MTd>
    )
    # Link against libraries required by the WindowsInstaller implementation.
    target_link_libraries(installer PRIVATE advapi32.lib shell32.lib user32.lib)

elseif(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_CLANG)
    # For GCC/Clang on Linux and macOS
    target_compile_options(installer PRIVATE
        -Wall -Wextra -Wpedantic
    )
endif()

if(UNIX AND NOT APPLE)
    # On Linux, link against pthread and dl.
    target_link_libraries(installer PRIVATE pthread dl)
endif()

if(APPLE)
    # On macOS, set the minimum deployment target.
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS deployment version")
    # Link against core macOS frameworks if needed in the future.
    # target_link_libraries(installer PRIVATE "-framework Foundation -framework CoreServices")
endif()


# --- CPack Package Generation (Initial Setup) ---
# This section is now more relevant as we have platform-specific logic.
set(CPACK_PACKAGE_NAME ${PROJECT_NAME})
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${PROJECT_DESCRIPTION}")
set(CPACK_PACKAGE_VENDOR "Pedro Henrique / phkaiser13")
set(CPACK_PACKAGE_CONTACT "phkaiser13@users.noreply.github.com")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/phkaiser13/phgit")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.txt")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "phgit")

if(WIN32)
    set(CPACK_GENERATOR "NSIS")
elseif(UNIX AND NOT APPLE)
    set(CPACK_GENERATOR "DEB;RPM")
elseif(APPLE)
    set(CPACK_GENERATOR "DragNDrop")
endif()

# This command must be at the end of the file to include all CPack settings.
include(CPack)
