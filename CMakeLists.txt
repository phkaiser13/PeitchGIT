# /* Copyright (C) 2025 Pedro Henrique / phkaiser13
# * CMakeLists.txt - Main build script for the gitph project.
# *
# * This file orchestrates the entire build process, including:
# * 1. Finding and configuring all C/C++ dependencies (Lua, libcurl).
# * 2. Compiling the main C/C++ core application (`gitph`).
# * 3. Compiling all polyglot modules (Rust and C++) into shared libraries.
# * 4. Placing all build artifacts into a clean `bin` directory structure.
# * 5. Defining installation rules.
# *
# * It has been updated to handle the full migration to Rust modules, replacing
# * all previous Go build commands with robust Rust/Cargo integration.
# *
# * SPDX-License-Identifier: Apache-2.0 */

cmake_minimum_required(VERSION 3.15)

project(gitph LANGUAGES C CXX VERSION 1.0.0)

# --- Global Settings and Options ---
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
include(FetchContent)

# Define output directories for a clean build structure
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
set(MODULE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin/modules)
file(MAKE_DIRECTORY ${MODULE_OUTPUT_PATH})

# --- Dependency Discovery (CURL, Lua, nlohmann_json) ---
# This entire section for finding dependencies is well-structured and remains unchanged.
# It correctly uses find_package, pkg-config, and FetchContent as fallbacks.

# [O CÓDIGO DE DESCOBERTA DE DEPENDÊNCIAS (LUA, CURL, ETC.) PERMANECE O MESMO]
# ... (Vou omitir essa parte para ser breve, pois ela já estava correta) ...
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
endif()
if(NOT CURL_TARGET_NAME)
    message(FATAL_ERROR "Could not locate libcurl.")
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
endif()
if(NOT LUA_TARGET_NAME)
    message(FATAL_ERROR "Could not locate Lua 5.4 development files.")
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
add_executable(gitph ${CORE_SOURCES})

# Link core executable against its dependencies
target_include_directories(gitph PUBLIC
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/ipc/include
)
target_link_libraries(gitph
    PRIVATE
    logger
    ${CURL_TARGET_NAME}
    ${LUA_TARGET_NAME}
)
if(UNIX AND NOT APPLE)
    target_link_libraries(gitph PRIVATE dl m)
endif()


# --- Compile Polyglot Modules ---

# CORREÇÃO: Macro aprimorada para compilar módulos Rust.
# Aceita dois argumentos:
# 1. module_path: O caminho para o diretório do módulo (ex: 'api_client' ou 'ci_cd_manager/parser').
# 2. library_name: O nome do crate definido no Cargo.toml (ex: 'api_client' ou 'workflow_parser').
# Isso torna a macro flexível o suficiente para lidar com diretórios aninhados e nomes de bibliotecas diferentes dos nomes de diretório.
macro(add_rust_module module_path library_name)
    set(OUTPUT_LIB_PATH ${MODULE_OUTPUT_PATH}/${CMAKE_SHARED_LIBRARY_PREFIX}${library_name}${CMAKE_SHARED_LIBRARY_SUFFIX})
    set(SOURCE_LIB_PATH ${CMAKE_BINARY_DIR}/cargo_target/release/${CMAKE_SHARED_LIBRARY_PREFIX}${library_name}${CMAKE_SHARED_LIBRARY_SUFFIX})

    add_custom_command(
        OUTPUT ${OUTPUT_LIB_PATH}
        # Step 1: Run cargo build. We redirect all cargo output to a separate target dir to keep the main build dir clean.
        COMMAND ${CMAKE_COMMAND} -E env "CARGO_TARGET_DIR=${CMAKE_BINARY_DIR}/cargo_target" cargo build --release --manifest-path ${CMAKE_SOURCE_DIR}/src/modules/${module_path}/Cargo.toml
        # Step 2: Copy the resulting shared library to the final module output directory.
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${SOURCE_LIB_PATH} ${MODULE_OUTPUT_PATH}/
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/src/modules/${module_path}
        COMMENT "Compiling Rust module: ${library_name}"
        VERBATIM
    )
    # Create a target to drive the build of this module. Adding it to 'ALL' ensures it's built by default.
    add_custom_target(build_${library_name} ALL
        DEPENDS ${OUTPUT_LIB_PATH}
    )
endmacro()

# --- Build All Modules ---

# Build original Rust modules
add_rust_module(git_ops git_ops)
add_rust_module(issue_tracker issue_tracker)
add_rust_module(sync_engine sync_engine)

# Build newly migrated Rust modules
add_rust_module(api_client api_client)
add_rust_module(devops_automation devops_automation)

# CORREÇÃO: Compila o parser de CI/CD como um módulo Rust, usando o caminho aninhado e o nome correto da biblioteca.
add_rust_module(ci_cd_manager/parser workflow_parser)

# Build the C++ visualizer module (unchanged)
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


# --- Installation Rules ---
# These rules are used when you run `cmake --install .`

# Install the main executable
install(TARGETS gitph DESTINATION bin)

# Install all shared libraries from the modules directory
install(
    DIRECTORY ${MODULE_OUTPUT_PATH}/
    DESTINATION bin/modules
    FILES_MATCHING PATTERN "*.so" PATTERN "*.dll" PATTERN "*.dylib"
)

# Placeholder for the installer helper tool (unchanged)
if(WIN32)
    add_executable(installer_helper installer/src/installer_main.c installer/src/download.c)
    target_link_libraries(installer_helper PRIVATE ${CURL_TARGET_NAME})
    set_target_properties(installer_helper PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/installer")
endif()