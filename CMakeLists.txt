# Copyright (C) 2025 Pedro Henrique
# CMakeLists.txt - Master build orchestration for the polyglot gitph project.
#
# This CMake script is the central nervous system of our build process. It is
# responsible for compiling the C/C++ core, invoking the native build tools
# for Rust (cargo) and Go (go build), and linking everything together into a
# distributable application.
#
# SPDX-License-Identifier: GPL-3.0-or-later

cmake_minimum_required(VERSION 3.15)

project(gitph LANGUAGES C CXX VERSION 1.0.0)

# --- Global Settings and Options ---
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Define output directories to keep the source tree clean.
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
set(MODULE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin/modules)

# Create the output directory for modules
file(MAKE_DIRECTORY ${MODULE_OUTPUT_PATH})

# --- Find System Dependencies ---
find_package(CURL REQUIRED)

# CORREÇÃO 1: Usar a sintaxe padrão do CMake para encontrar a biblioteca Lua versão 5.4.
# O módulo de busca é "Lua", e a versão "5.4" é passada como argumento.
find_package(Lua 5.4 REQUIRED)

# --- Build C/C++ Core ---

# 1. Build the C++ Logger as a static library to be linked into the core
add_library(logger STATIC src/libs/liblogger/Logger.cpp)
target_include_directories(logger PUBLIC src/ipc/include)

# 2. Gather all C source files for the main executable
file(GLOB_RECURSE CORE_SOURCES "src/core/*.c" "src/libs/libcommon/*.c")
file(GLOB_RECURSE CORE_HEADERS "src/core/*.h" "src/libs/libcommon/*.h" "src/ipc/include/*.h")

# 3. Build the main gitph executable
add_executable(gitph ${CORE_SOURCES} ${CORE_HEADERS})

#commiting here>
target_include_directories(gitph PRIVATE ${CMAKE_SOURCE_DIR}/src)

# 4. Link the core executable with its dependencies
# CORREÇÃO 2: Usar as variáveis padrão definidas pelo FindLua.cmake (LUA_LIBRARIES).
target_link_libraries(gitph
        PRIVATE
        logger
        ${CURL_LIBRARIES}
        ${LUA_LIBRARIES}
)
# On POSIX systems, we need to link against libdl for dynamic loading
if(UNIX AND NOT APPLE)
    target_link_libraries(gitph PRIVATE dl)
endif()

# CORREÇÃO 3: Usar as variáveis padrão definidas pelo FindLua.cmake (LUA_INCLUDE_DIR).
target_include_directories(gitph PUBLIC
    ${CMAKE_SOURCE_DIR}/src/core/cli
    ${CMAKE_SOURCE_DIR}/src/core/config
    ${CMAKE_SOURCE_DIR}/src/core/main
    ${CMAKE_SOURCE_DIR}/src/core/module_loader
    ${CMAKE_SOURCE_DIR}/src/core/platform
    ${CMAKE_SOURCE_DIR}/src/core/scripting
    ${CMAKE_SOURCE_DIR}/src/core/ui
    ${CMAKE_SOURCE_DIR}/src/ipc/include
    ${CMAKE_SOURCE_DIR}/src/libs/libcommon
    ${CMAKE_SOURCE_DIR}/src/libs/liblogger
    ${LUA_INCLUDE_DIR}
)

# --- Build Polyglot Modules (via Custom Commands) ---

# Helper macro to define a build process for a Rust module
macro(add_rust_module module_name)
    add_custom_command(
            OUTPUT ${MODULE_OUTPUT_PATH}/${CMAKE_SHARED_LIBRARY_PREFIX}${module_name}${CMAKE_SHARED_LIBRARY_SUFFIX}
            COMMAND ${CMAKE_COMMAND} -E env "CARGO_TARGET_DIR=${CMAKE_BINARY_DIR}/cargo_target" cargo build --release --manifest-path ${CMAKE_SOURCE_DIR}/src/modules/${module_name}/Cargo.toml
            COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_BINARY_DIR}/cargo_target/release/${CMAKE_SHARED_LIBRARY_PREFIX}${module_name}${CMAKE_SHARED_LIBRARY_SUFFIX}
            ${MODULE_OUTPUT_PATH}/
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/src/modules/${module_name}
            COMMENT "Building Rust module: ${module_name}"
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
            COMMENT "Building Go module: ${module_name}"
            VERBATIM
    )
    add_custom_target(build_${module_name} ALL
            DEPENDS ${MODULE_OUTPUT_PATH}/${CMAKE_SHARED_LIBRARY_PREFIX}${module_name}${CMAKE_SHARED_LIBRARY_SUFFIX}
    )
endmacro()

# --- Invoke the build for each module ---
add_rust_module(git_ops)
add_rust_module(issue_tracker)
add_rust_module(sync_engine)

add_go_module(api_client)
add_go_module(devops_automation)
# Note: The ci_cd_manager is more complex (Go + C++). A full implementation
# would require more steps. For now, we assume its parts are built separately
# or integrated into the main build in a more complex fashion.

# --- Installation Rules ---
# This defines what `make install` will do.
install(TARGETS gitph DESTINATION bin)
install(
        DIRECTORY ${MODULE_OUTPUT_PATH}/
        DESTINATION bin/modules
        FILES_MATCHING PATTERN "*.so" PATTERN "*.dll" PATTERN "*.dylib"
)
