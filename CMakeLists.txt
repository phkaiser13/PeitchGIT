# Copyright (C) 2025 Pedro Henrique / phkaiser13
# CMakeLists.txt - Build orchestration for the gitph polyglot project.
#
# This CMake script is the central nervous system of our build process.
# It's responsible for compiling the C/C++ core, invoking native build
# tools for Rust (cargo) and Go (go build), and linking everything into a
# distributable application.
#
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.15)

project(gitph LANGUAGES C CXX VERSION 1.0.0)

# --- Global Settings and Options ---
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
include(FetchContent) # Include FetchContent for dependency management

# Define output directories to keep the source tree clean.
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
set(MODULE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin/modules)

# Create the output directory for modules
file(MAKE_DIRECTORY ${MODULE_OUTPUT_PATH})

# --- Dependency Management ---
# This section handles external C/C++ libraries.
#
# How to provide dependencies:
# 1. (Recommended) Use a C++ package manager like vcpkg.
#    - Set the CMake toolchain file: cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=[path/to/vcpkg]/scripts/buildsystems/vcpkg.cmake
#    - `find_package` will automatically find the libraries installed by vcpkg.
#
# 2. Use a system package manager (e.g., `sudo apt-get install`, `brew install`, `choco install`).
#    - `find_package` will search for system-wide installations.
#
# 3. (Fallback) Do nothing.
#    - If a package is not found, CMake's `FetchContent` will automatically
#      download and build it. This is the case for nlohmann_json below.

# --- Find System Dependencies ---
find_package(CURL REQUIRED)
find_package(Lua 5.4 REQUIRED)

# Find nlohmann_json. If not found, it will be fetched automatically later.
find_package(nlohmann_json 3.2.0 QUIET)

# On UNIX, add pkg-config to help find libraries like Lua
if(UNIX)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(LUA lua5.4)
endif()

# --- Automatic Dependency Fetching (if not found on system) ---

# If the nlohmann_json target is not defined after find_package,
# download it from GitHub.
if(NOT nlohmann_json_FOUND)
    message(STATUS "nlohmann_json not found on system. Fetching from GitHub...")
    FetchContent_Declare(
        nlohmann_json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG v3.11.3 # Using a specific version is good practice
    )
    FetchContent_MakeAvailable(nlohmann_json)
    message(STATUS "nlohmann_json fetched successfully.")
endif()


# --- Compile C/C++ Core ---

# 1. Compile the C++ Logger as a static library
add_library(logger STATIC src/libs/liblogger/Logger.cpp)
target_include_directories(logger PUBLIC ${CMAKE_SOURCE_DIR}/src/ipc/include)

# 2. Gather all C source files for the main executable
file(GLOB_RECURSE CORE_SOURCES
    "src/core/cli/*.c"
    "src/core/config/*.c"
    "src/core/main/*.c"
    "src/core/module_loader/*.c"
    "src/core/platform/*.c"
    "src/core/scripting/*.c"
    "src/core/ui/*.c"
    "src/libs/libcommon/*.c"
)

# 3. Compile the main gitph executable
add_executable(gitph ${CORE_SOURCES})

# 4. Add include directories for the main executable
target_include_directories(gitph PUBLIC
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/core
    ${CMAKE_SOURCE_DIR}/src/ipc/include
    ${CMAKE_SOURCE_DIR}/src/libs
    ${LUA_INCLUDE_DIRS}
)

# 5. Link the main executable with its dependencies
target_link_libraries(gitph
    PRIVATE
    logger
    ${CURL_LIBRARIES}
    ${LUA_LIBRARIES}
)

# On POSIX systems, we need to link against libdl for dynamic loading
if(UNIX AND NOT APPLE)
    target_link_libraries(gitph PRIVATE dl m) # Added 'm' for math library, if needed by Lua
endif()


# --- Compile Polyglot Modules (via Custom Commands) ---

# Helper macro to define a build process for a Rust module
macro(add_rust_module module_name)
    add_custom_command(
        OUTPUT ${MODULE_OUTPUT_PATH}/${CMAKE_SHARED_LIBRARY_PREFIX}${module_name}${CMAKE_SHARED_LIBRARY_SUFFIX}
        COMMAND ${CMAKE_COMMAND} -E env "CARGO_TARGET_DIR=${CMAKE_BINARY_DIR}/cargo_target" cargo build --release --manifest-path ${CMAKE_SOURCE_DIR}/src/modules/${module_name}/Cargo.toml
        COMMAND ${CMAKE_COMMAND} -E copy
                ${CMAKE_BINARY_DIR}/cargo_target/release/${CMAKE_SHARED_LIBRARY_PREFIX}${module_name}${CMAKE_SHARED_LIBRARY_SUFFIX}
                ${MODULE_OUTPUT_PATH}/
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/src/modules/${module_name}
        COMMENT "Compiling Rust module: ${module_name}"
        VERBATIM
    )
    add_custom_target(build_${module_name} ALL
        DEPENDS ${MODULE_OUTPUT_PATH}/${CMAKE_SHARED_LIBRARY_PREFIX}${module_name}${CMAKE_SHARED_LIBRARY_SUFFIX}
    )
endmacro()

# Helper macro to define a build process for a Go module
macro(add_go_module module_name)
    add_custom_command(
        OUTPUT ${MODULE_OUTPUT_PATH}/${CMAKE_SHARED_LIBRARY_PREFIX}${module_name}${CMAKE_SHARED_LIBRARY_SUFFIX}
        COMMAND go build -buildmode=c-shared -o ${MODULE_OUTPUT_PATH}/${CMAKE_SHARED_LIBRARY_PREFIX}${module_name}${CMAKE_SHARED_LIBRARY_SUFFIX} .
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/src/modules/${module_name}
        COMMENT "Compiling Go module: ${module_name}"
        VERBATIM
    )
    add_custom_target(build_${module_name} ALL
        DEPENDS ${MODULE_OUTPUT_PATH}/${CMAKE_SHARED_LIBRARY_PREFIX}${module_name}${CMAKE_SHARED_LIBRARY_SUFFIX}
    )
endmacro()

# --- Invoke compilation for each module ---
add_rust_module(git_ops)
add_rust_module(issue_tracker)
add_rust_module(sync_engine)

add_go_module(api_client)
add_go_module(devops_automation)

# --- Specific Build for `ci_cd_manager` Module ---

# 1. Compile the Go parser as a shared library
add_custom_command(
    OUTPUT ${MODULE_OUTPUT_PATH}/${CMAKE_SHARED_LIBRARY_PREFIX}ci_cd_parser${CMAKE_SHARED_LIBRARY_SUFFIX}
    COMMAND go build -buildmode=c-shared -o ${MODULE_OUTPUT_PATH}/${CMAKE_SHARED_LIBRARY_PREFIX}ci_cd_parser${CMAKE_SHARED_LIBRARY_SUFFIX} parser.go
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/src/modules/ci_cd_manager
    COMMENT "Compiling CI/CD Go parser"
    VERBATIM
)
add_custom_target(build_ci_cd_parser ALL
    DEPENDS ${MODULE_OUTPUT_PATH}/${CMAKE_SHARED_LIBRARY_PREFIX}ci_cd_parser${CMAKE_SHARED_LIBRARY_SUFFIX}
)

# 2. Compile the C++ visualizer as a shared library
# NOTE: The nlohmann/json library is now handled automatically by find_package or FetchContent.
add_library(ci_cd_visualizer SHARED src/modules/ci_cd_manager/visualizer/PipelineVisualizer.cpp)
target_include_directories(ci_cd_visualizer PUBLIC
    ${CMAKE_SOURCE_DIR}/src/ipc/include
)

# The visualizer needs access to the logger and nlohmann_json.
# Linking to nlohmann_json::nlohmann_json automatically adds its include directory.
target_link_libraries(ci_cd_visualizer PRIVATE
    logger
    nlohmann_json::nlohmann_json
)

set_target_properties(ci_cd_visualizer PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${MODULE_OUTPUT_PATH}"
)

# --- Installation Rules ---
install(TARGETS gitph DESTINATION bin)
install(
    DIRECTORY ${MODULE_OUTPUT_PATH}/
    DESTINATION bin/modules
    FILES_MATCHING PATTERN "*.so" PATTERN "*.dll" PATTERN "*.dylib"
)

# Add a target for the C installer (not compiled by default with 'all')
add_executable(installer_main installer/src/installer_main.c installer/src/download.c)
target_link_libraries(installer_main PRIVATE ${CURL_LIBRARIES})
set_target_properties(installer_main PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/installer")
