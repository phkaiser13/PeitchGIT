# Copyright (C) 2025 Pedro Henrique
# CMakeLists.txt - Orquestração de compilação para o projeto poliglota gitph.
#
# Este script CMake é o sistema nervoso central do nosso processo de build.
# É responsável por compilar o núcleo C/C++, invocar as ferramentas de compilação
# nativas para Rust (cargo) e Go (go build), e ligar tudo numa
# aplicação distribuível.
#
# SPDX-License-Identifier: GPL-3.0-or-later

cmake_minimum_required(VERSION 3.15)

project(gitph LANGUAGES C CXX VERSION 1.0.0)

# --- Configurações Globais e Opções ---
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Define diretórios de saída para manter a árvore de fontes limpa.
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
set(MODULE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin/modules)

# Cria o diretório de saída para os módulos
file(MAKE_DIRECTORY ${MODULE_OUTPUT_PATH})

# --- Encontrar Dependências de Sistema ---
find_package(CURL REQUIRED)
find_package(Lua 5.4 REQUIRED)
# Adiciona o pkg-config para ajudar a encontrar bibliotecas como a Lua
find_package(PkgConfig REQUIRED)
pkg_check_modules(LUA lua5.4)


# --- Compilar o Núcleo C/C++ ---

# 1. Compila o Logger C++ como uma biblioteca estática
add_library(logger STATIC src/libs/liblogger/Logger.cpp)
target_include_directories(logger PUBLIC ${CMAKE_SOURCE_DIR}/src/ipc/include)

# 2. Reúne todos os ficheiros fonte C para o executável principal
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

# 3. Compila o executável principal gitph
add_executable(gitph ${CORE_SOURCES})

# 4. Adiciona os diretórios de inclusão para o executável principal
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
    ${LUA_INCLUDE_DIRS}
    # Adicione o diretório para nlohmann/json se o tiver localmente
    # "src/libs/nlohmann"
)

# 5. Liga o executável principal com as suas dependências
target_link_libraries(gitph
    PRIVATE
    logger
    ${CURL_LIBRARIES}
    ${LUA_LIBRARIES}
)

# Em sistemas POSIX, precisamos de ligar contra libdl para carregamento dinâmico
if(UNIX AND NOT APPLE)
    target_link_libraries(gitph PRIVATE dl m) # Adicionado 'm' para a biblioteca matemática, se necessário pela Lua
endif()


# --- Compilar Módulos Poliglotas (via Comandos Personalizados) ---

# Macro auxiliar para definir um processo de compilação para um módulo Rust
macro(add_rust_module module_name)
    add_custom_command(
        OUTPUT ${MODULE_OUTPUT_PATH}/${CMAKE_SHARED_LIBRARY_PREFIX}${module_name}${CMAKE_SHARED_LIBRARY_SUFFIX}
        COMMAND ${CMAKE_COMMAND} -E env "CARGO_TARGET_DIR=${CMAKE_BINARY_DIR}/cargo_target" cargo build --release --manifest-path ${CMAKE_SOURCE_DIR}/src/modules/${module_name}/Cargo.toml
        COMMAND ${CMAKE_COMMAND} -E copy
                ${CMAKE_BINARY_DIR}/cargo_target/release/${CMAKE_SHARED_LIBRARY_PREFIX}${module_name}${CMAKE_SHARED_LIBRARY_SUFFIX}
                ${MODULE_OUTPUT_PATH}/
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/src/modules/${module_name}
        COMMENT "A compilar o módulo Rust: ${module_name}"
        VERBATIM
    )
    add_custom_target(build_${module_name} ALL
        DEPENDS ${MODULE_OUTPUT_PATH}/${CMAKE_SHARED_LIBRARY_PREFIX}${module_name}${CMAKE_SHARED_LIBRARY_SUFFIX}
    )
endmacro()

# Macro auxiliar para definir um processo de compilação para um módulo Go
macro(add_go_module module_name)
    add_custom_command(
        OUTPUT ${MODULE_OUTPUT_PATH}/${CMAKE_SHARED_LIBRARY_PREFIX}${module_name}${CMAKE_SHARED_LIBRARY_SUFFIX}
        COMMAND go build -buildmode=c-shared -o ${MODULE_OUTPUT_PATH}/${CMAKE_SHARED_LIBRARY_PREFIX}${module_name}${CMAKE_SHARED_LIBRARY_SUFFIX} .
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/src/modules/${module_name}
        COMMENT "A compilar o módulo Go: ${module_name}"
        VERBATIM
    )
    add_custom_target(build_${module_name} ALL
        DEPENDS ${MODULE_OUTPUT_PATH}/${CMAKE_SHARED_LIBRARY_PREFIX}${module_name}${CMAKE_SHARED_LIBRARY_SUFFIX}
    )
endmacro()

# --- Invoca a compilação para cada módulo ---
add_rust_module(git_ops)
add_rust_module(issue_tracker)
add_rust_module(sync_engine)

add_go_module(api_client)
add_go_module(devops_automation)

# --- Compilação Específica para o Módulo `ci_cd_manager` ---

# 1. Compila o parser Go como uma biblioteca partilhada
add_custom_command(
    OUTPUT ${MODULE_OUTPUT_PATH}/${CMAKE_SHARED_LIBRARY_PREFIX}ci_cd_parser${CMAKE_SHARED_LIBRARY_SUFFIX}
    COMMAND go build -buildmode=c-shared -o ${MODULE_OUTPUT_PATH}/${CMAKE_SHARED_LIBRARY_PREFIX}ci_cd_parser${CMAKE_SHARED_LIBRARY_SUFFIX} parser.go
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/src/modules/ci_cd_manager
    COMMENT "A compilar o parser Go do CI/CD"
    VERBATIM
)
add_custom_target(build_ci_cd_parser ALL
    DEPENDS ${MODULE_OUTPUT_PATH}/${CMAKE_SHARED_LIBRARY_PREFIX}ci_cd_parser${CMAKE_SHARED_LIBRARY_SUFFIX}
)

# 2. Compila o visualizador C++ como uma biblioteca partilhada
# NOTA: Isto assume que a biblioteca nlohmann/json está disponível no caminho de inclusão.
# Se a descarregou para, por exemplo, 'src/libs/nlohmann', descomente a linha `target_include_directories`.
add_library(ci_cd_visualizer SHARED src/modules/ci_cd_manager/visualizer/PipelineVisualizer.cpp)
target_include_directories(ci_cd_visualizer PUBLIC
    ${CMAKE_SOURCE_DIR}/src/ipc/include
    # Descomente a linha abaixo se o nlohmann/json estiver em src/libs
    # ${CMAKE_SOURCE_DIR}/src/libs
)
# O visualizador precisa de aceder ao logger
target_link_libraries(ci_cd_visualizer PRIVATE logger)
set_target_properties(ci_cd_visualizer PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${MODULE_OUTPUT_PATH}"
)

# --- Regras de Instalação ---
install(TARGETS gitph DESTINATION bin)
install(
    DIRECTORY ${MODULE_OUTPUT_PATH}/
    DESTINATION bin/modules
    FILES_MATCHING PATTERN "*.so" PATTERN "*.dll" PATTERN "*.dylib"
)

# Adiciona um alvo para o instalador C (não compilado por defeito com 'all')
add_executable(installer_main installer/src/installer_main.c installer/src/download.c)
target_link_libraries(installer_main PRIVATE ${CURL_LIBRARIES})
set_target_properties(installer_main PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/installer")
