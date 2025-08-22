# /* Copyright (C) 2025 Pedro Henrique / phkaiser13
# * CMakeLists.txt - Main build script for the phgit project.
# *
# * This file orchestrates the entire build process, including:
# * 1. Finding and configuring all C/C++ dependencies (Lua, libcurl).
# * 2. Compiling the main C/C++ core application (`phgit`).
# * 3. Compiling all polyglot modules (Rust and C++) into shared libraries.
# * 4. Placing all build artifacts into a clean `bin` directory structure.
# * 5. Defining installation rules.
# *
# * It has been updated to handle the full migration to Rust modules, replacing
# * all previous Go build commands with robust Rust/Cargo integration.
# *
# * Important: Some comments still in portuguese.
# * SPDX-License-Identifier: Apache-2.0 */

cmake_minimum_required(VERSION 3.15)

project(phgit LANGUAGES C CXX VERSION 1.0.0)

# --- Global Settings and Options ---
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
include(FetchContent)

# Define output directories for a clean build structure
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
set(MODULE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin/modules)
file(MAKE_DIRECTORY ${MODULE_OUTPUT_PATH})


# ==========================================================================================
# BEGIN: MERGED DEPENDENCY DISCOVERY LOGIC
# This entire section is transplanted from the working CMake file.
# It provides a much more robust way to find dependencies by:
# 1. Respecting environment variables (LUA_DIR, CURL_DIR, VCPKG_ROOT).
# 2. Checking standard system paths (including Homebrew on macOS).
# 3. Using pkg-config as a reliable fallback if find_package fails.
# 4. Creating modern imported targets (e.g., `CURL::libcurl`) for clean linking.
# ==========================================================================================

# Allow users / CI to pass overrides
if(DEFINED ENV{LUA_DIR} AND NOT DEFINED LUA_DIR)
set(LUA_DIR $ENV{LUA_DIR})
endif()
if(DEFINED ENV{CURL_DIR} AND NOT DEFINED CURL_DIR)
set(CURL_DIR $ENV{CURL_DIR})
endif()
if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "vcpkg toolchain")
endif()

# Add common Homebrew prefixes on macOS
if(APPLE)
if(EXISTS "/opt/homebrew/opt")
list(APPEND CMAKE_PREFIX_PATH "/opt/homebrew/opt")
endif()
if(EXISTS "/usr/local/opt")
list(APPEND CMAKE_PREFIX_PATH "/usr/local/opt")
endif()
endif()

# --- Helper: try pkg-config with candidate names ---
find_package(PkgConfig QUIET)
function(try_pkg_config OUT_PREFIX)
set(options)
set(oneValueArgs)
set(multiValueArgs NAMES)
cmake_parse_arguments(PKGC "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

if(NOT PKG_CONFIG_FOUND)
return()
endif()

foreach(name IN LISTS PKGC_NAMES)
pkg_check_modules(_pc_${OUT_PREFIX} ${name} QUIET)
if(_pc_${OUT_PREFIX}_FOUND)
set(${OUT_PREFIX}_PKG_NAME ${name} PARENT_SCOPE)
string(TOUPPER ${OUT_PREFIX} _UP)
set(${_UP}_PKG_FOUND TRUE PARENT_SCOPE)
set(${_UP}_PKG_INCLUDE_DIRS ${_pc_${OUT_PREFIX}_INCLUDE_DIRS} PARENT_SCOPE)
set(${_UP}_PKG_LIBRARY_DIRS ${_pc_${OUT_PREFIX}_LIBRARY_DIRS} PARENT_SCOPE)
set(${_UP}_PKG_LIBRARIES ${_pc_${OUT_PREFIX}_LIBRARIES} PARENT_SCOPE)
return()
endif()
endforeach()
endfunction()

# --- Find CURL (prefer find_package, fallback to pkg-config) ---
set(CURL_TARGET_NAME "")
find_package(CURL QUIET)
if(CURL_FOUND)
if(TARGET CURL::libcurl)
set(CURL_TARGET_NAME "CURL::libcurl")
else()
add_library(CURL::libcurl UNKNOWN IMPORTED)
set_target_properties(CURL::libcurl PROPERTIES
IMPORTED_LOCATION "${CURL_LIBRARY}"
INTERFACE_INCLUDE_DIRECTORIES "${CURL_INCLUDE_DIRS}"
)
set(CURL_TARGET_NAME "CURL::libcurl")
endif()
else()
try_pkg_config(CURL libcurl curl)
if(CURL_PKG_FOUND)
add_library(CURL::libcurl UNKNOWN IMPORTED)
set_target_properties(CURL::libcurl PROPERTIES
INTERFACE_INCLUDE_DIRECTORIES "${CURL_PKG_INCLUDE_DIRS}"
INTERFACE_LINK_LIBRARIES "${CURL_PKG_LIBRARIES}"
)
set(CURL_TARGET_NAME "CURL::libcurl")
endif()
endif()

if(NOT CURL_TARGET_NAME)
message(FATAL_ERROR "Could not locate libcurl. Use system package manager or provide CURL_DIR.")
endif()

# --- Find Lua (try find_package, then pkg-config with multiple names) ---
set(LUA_TARGET_NAME "")
find_package(Lua 5.4 QUIET)
if(Lua_FOUND)
if(TARGET Lua::Lua)
set(LUA_TARGET_NAME "Lua::Lua")
else()
if(DEFINED LUA_LIBRARIES)
set(_lua_lib "${LUA_LIBRARIES}")
elseif(DEFINED LUA_LIBRARY)
set(_lua_lib "${LUA_LIBRARY}")
endif()
if(DEFINED LUA_INCLUDE_DIR)
set(_lua_inc "${LUA_INCLUDE_DIR}")
elseif(DEFINED LUA_INCLUDE_DIRS)
set(_lua_inc "${LUA_INCLUDE_DIRS}")
endif()
add_library(Lua::Lua INTERFACE IMPORTED)
if(_lua_lib)
set_target_properties(Lua::Lua PROPERTIES INTERFACE_LINK_LIBRARIES "${_lua_lib}")
endif()
if(_lua_inc)
set_target_properties(Lua::Lua PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${_lua_inc}")
endif()
set(LUA_TARGET_NAME "Lua::Lua")
endif()
else()
try_pkg_config(LUA lua5.4 lua-5.4 lua liblua)
if(LUA_PKG_FOUND)
add_library(Lua::Lua UNKNOWN IMPORTED)
set_target_properties(Lua::Lua PROPERTIES
INTERFACE_INCLUDE_DIRECTORIES "${LUA_PKG_INCLUDE_DIRS}"
INTERFACE_LINK_LIBRARIES "${LUA_PKG_LIBRARIES}"
)
set(LUA_TARGET_NAME "Lua::Lua")
endif()
endif()

if(NOT LUA_TARGET_NAME)
message(FATAL_ERROR "Could not locate Lua 5.4 development files. Install it or provide LUA_DIR.")
endif()

# --- Find nlohmann_json (try package, else FetchContent) ---
find_package(nlohmann_json 3.2.0 QUIET)
if(NOT nlohmann_json_FOUND)
message(STATUS "nlohmann_json not found on system. Fetching via FetchContent...")
FetchContent_Declare(
nlohmann_json
GIT_REPOSITORY https://github.com/nlohmann/json.git
GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(nlohmann_json)
endif()

# ==========================================================================================
# END: MERGED DEPENDENCY DISCOVERY LOGIC
# ==========================================================================================


# --- Compile C/C++ Core ---

# Logger library (static)
add_library(logger STATIC src/libs/liblogger/Logger.cpp)
target_include_directories(logger PUBLIC ${CMAKE_SOURCE_DIR}/src/ipc/include)

# Main executable
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
add_executable(phgit ${CORE_SOURCES})

# --- FIX: Correctly add all necessary include directories to the target ---
# This is the second part of the fix. Modern imported targets (like Lua::Lua)
# automatically add their include directories when you link them. However, for
# maximum compatibility with older find modules, it's safest to add them
# explicitly if the variables are defined. This also includes internal project headers.
target_include_directories(phgit PUBLIC
${CMAKE_SOURCE_DIR}/src
${CMAKE_SOURCE_DIR}/src/core
${CMAKE_SOURCE_DIR}/src/ipc/include
${CMAKE_SOURCE_DIR}/src/libs
# Add include dirs found by find_package/pkg-config
${LUA_INCLUDE_DIRS}
${LUA_PKG_INCLUDE_DIRS}
${CURL_INCLUDE_DIRS}
${CURL_PKG_INCLUDE_DIRS}
)

# Link core executable against its dependencies
# This part was already correct, using the modern target names.
target_link_libraries(phgit
PRIVATE
logger
${CURL_TARGET_NAME}
${LUA_TARGET_NAME}
)
if(UNIX AND NOT APPLE)
target_link_libraries(phgit PRIVATE dl m)
endif()


# --- Compile Polyglot Modules ---
# This section remains unchanged, preserving your new Rust macro and build logic.

macro(add_rust_module module_path library_name)
set(OUTPUT_LIB_PATH ${MODULE_OUTPUT_PATH}/${CMAKE_SHARED_LIBRARY_PREFIX}${library_name}${CMAKE_SHARED_LIBRARY_SUFFIX})
set(SOURCE_LIB_PATH ${CMAKE_BINARY_DIR}/cargo_target/release/${CMAKE_SHARED_LIBRARY_PREFIX}${library_name}${CMAKE_SHARED_LIBRARY_SUFFIX})

add_custom_command(
OUTPUT ${OUTPUT_LIB_PATH}
COMMAND ${CMAKE_COMMAND} -E env "CARGO_TARGET_DIR=${CMAKE_BINARY_DIR}/cargo_target" cargo build --release --manifest-path ${CMAKE_SOURCE_DIR}/src/modules/${module_path}/Cargo.toml
COMMAND ${CMAKE_COMMAND} -E copy_if_different ${SOURCE_LIB_PATH} ${MODULE_OUTPUT_PATH}/
WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/src/modules/${module_path}
COMMENT "Compiling Rust module: ${library_name}"
VERBATIM
)
add_custom_target(build_${library_name} ALL
DEPENDS ${OUTPUT_LIB_PATH}
)
endmacro()

# --- Build All Modules ---
add_rust_module(git_ops git_ops)
add_rust_module(issue_tracker issue_tracker)
add_rust_module(sync_engine sync_engine)
add_rust_module(api_client api_client)
add_rust_module(devops_automation devops_automation)
add_rust_module(ci_cd_manager/parser workflow_parser)

# Build the C++ visualizer module
add_library(ci_cd_visualizer SHARED src/modules/ci_cd_manager/visualizer/PipelineVisualizer.cpp)
target_include_directories(ci_cd_visualizer PUBLIC
${CMAKE_SOURCE_DIR}/src/ipc/include
)
target_link_libraries(ci_cd_visualizer PRIVATE
logger
nlohmann_json::nlohmann_json
)
set_target_properties(ci_cd_visualizer PROPERTIES
LIBRARY_OUTPUT_DIRECTORY "${MODULE_OUTPUT_PATH}"
)
# # ==========================================================================================
# # Installer Engine Subproject
# # ==========================================================================================

# # This section integrates the C++ installer engine located in the 'installer' subdirectory.
# # It uses an option to allow developers to enable or disable its build as needed.
# # When enabled, it builds the 'phgit-installer' executable and its dependencies.

# option(PHGIT_BUILD_INSTALLER "Build the phgit installer engine" ON)

# if(PHGIT_BUILD_INSTALLER)
#    message(STATUS "Installer engine build is enabled. Adding subdirectory.")
#    # This command tells CMake to enter the 'installer' directory and process its
#    # CMakeLists.txt file. All targets defined there (like the 'installer' executable)
#    # will become part of this main build.
#   add_subdirectory(installer)
# else()
#    message(STATUS "Installer engine build is disabled.")
# endif()

# # --- Installation Rules ---
# install(TARGETS phgit DESTINATION bin)
# install(
#    DIRECTORY ${MODULE_OUTPUT_PATH}/
#    DESTINATION bin/modules
#    FILES_MATCHING PATTERN "*.so" PATTERN "*.dll" PATTERN "*.dylib"
# )

# # --- ADDING THIS PART ---
# # Installation rules for the installer engine
# if(PHGIT_BUILD_INSTALLER)
#     # Install the installer executable itself into the main bin directory
#     install(TARGETS installer DESTINATION bin)

#     # IMPORTANT: Install the configuration file alongside the installer executable
#     install(
#         FILES ${CMAKE_SOURCE_DIR}/installer/config.json
#         DESTINATION bin
#     )
# endif()
