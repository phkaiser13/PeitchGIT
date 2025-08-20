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

# CORREÇÃO 1: Forçar uso da runtime library estática para evitar conflitos LNK2038
# Isso garante que todas as dependências (spdlog, etc.) usem a mesma configuração
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

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
        # Força a miniz a usar uma política de versão do CMake compatível
        CMAKE_ARGS -DCMAKE_POLICY_VERSION_MINIMUM=3.5
)
FetchContent_MakeAvailable(miniz)

# Find libcurl for downloading
find_package(CURL REQUIRED)

# --- CORREÇÃO 2: Compilar miniz como biblioteca estática separada ---
# Isso resolve os erros LNK2019 criando uma biblioteca com todos os símbolos necessários
add_library(miniz_lib STATIC
        ${miniz_SOURCE_DIR}/miniz.c
        # Incluir outros arquivos da miniz se existirem (algumas versões os têm separados)
        ${miniz_SOURCE_DIR}/miniz_zip.c
        ${miniz_SOURCE_DIR}/miniz_tdef.c
        ${miniz_SOURCE_DIR}/miniz_tinfl.c
)

# Garantir que os diretórios de include da miniz estejam disponíveis
target_include_directories(miniz_lib PUBLIC
        "${miniz_SOURCE_DIR}"
        "${miniz_BINARY_DIR}"
)

# Configurar a miniz_lib para usar a mesma runtime library
if(MSVC)
    target_compile_options(miniz_lib PRIVATE
            $<$<CONFIG:RELEASE>:/MT>
            $<$<CONFIG:DEBUG>:/MTd>
    )
endif()

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
        src/platform/linux-systems/linux_installer.cpp
        src/platform/darwin-mac/darwin-mac_installer.cpp
        src/platform/windows/windows_installer.cpp
        # REMOVIDO: ${miniz_SOURCE_DIR}/miniz.c - agora é parte da miniz_lib
)

# --- Executable Target Definition ---
add_executable(installer ${INSTALLER_SOURCES})
set_target_properties(installer PROPERTIES OUTPUT_NAME "phgit-installer")

# --- Include Directories ---
target_include_directories(installer PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
        ${CURL_INCLUDE_DIRS}
        "${miniz_SOURCE_DIR}"    # Add miniz.h's directory
        "${miniz_BINARY_DIR}"    # Add generated headers like miniz_export.h
)

# --- Linking Dependencies ---
target_link_libraries(installer PRIVATE
        spdlog::spdlog
        CURL::libcurl
        miniz_lib              # ADICIONADO: Link com a biblioteca miniz estática
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

# --- Build Type Configuration ---
# Garantir que Debug e Release tenham configurações consistentes
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

# --- Compiler-specific optimizations ---
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    if(MSVC)
        target_compile_options(installer PRIVATE /O2 /DNDEBUG)
    else()
        target_compile_options(installer PRIVATE -O3 -DNDEBUG)
    endif()
endif()

# --- Packaging ---
# Configure CPack for distribution
set(CPACK_PACKAGE_NAME "phgit-installer")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${PROJECT_DESCRIPTION}")
set(CPACK_PACKAGE_VENDOR "Pedro Henrique / phkaiser13")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/../LICENSE")

# Platform-specific packaging
if(WIN32)
    set(CPACK_GENERATOR "ZIP")
elseif(APPLE)
    set(CPACK_GENERATOR "TGZ")
else()
    set(CPACK_GENERATOR "TGZ")
endif()

include(CPack)

add_custom_command(TARGET installer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
    "${CMAKE_CURRENT_SOURCE_DIR}/config.json"
    "$<TARGET_FILE_DIR:installer>/config.json"
    COMMENT "Copiando config.json para o diretório de saída do build"
)