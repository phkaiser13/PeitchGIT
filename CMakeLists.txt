cmake_minimum_required(VERSION 3.15)
project(phgit VERSION 1.0.0 LANGUAGES C CXX)

# ----------------------
# Global user options
# ----------------------
#logica.instalador: opção que controla se o "installer engine" será construído.
#logica.instalador: por padrão está OFF — ative com -DPHGIT_BUILD_INSTALLER=ON
option(PHGIT_BUILD_INSTALLER "Build the installer engine" OFF)

option(PHGIT_ENABLE_TESTS "Enable building tests" OFF)
option(PHGIT_USE_SYSTEM_CURL "Prefer system libcurl over package manager" ON)
option(PHGIT_USE_SYSTEM_LUA "Prefer system Lua over package manager" ON)
option(PHGIT_VERBOSE_FIND "Be verbose about dependency discovery" ON)

# Allow users/CI to override common variables. Prefer environment variables if set.
if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "vcpkg toolchain file" FORCE)
endif()

# Legacy compatibility: allow either CURL_DIR or CURL_ROOT, LUA_DIR or LUA_ROOT
if(DEFINED ENV{CURL_DIR} AND NOT DEFINED CURL_DIR)
    set(CURL_DIR $ENV{CURL_DIR})
endif()
if(DEFINED ENV{CURL_ROOT} AND NOT DEFINED CURL_DIR)
    set(CURL_DIR $ENV{CURL_ROOT})
endif()
if(DEFINED ENV{LUA_DIR} AND NOT DEFINED LUA_DIR)
    set(LUA_DIR $ENV{LUA_DIR})
endif()
if(DEFINED ENV{LUA_ROOT} AND NOT DEFINED LUA_DIR)
    set(LUA_DIR $ENV{LUA_ROOT})
endif()

# ----------------------
# Output directories (single-config generator friendly)
# ----------------------
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(MODULE_OUTPUT_PATH ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/modules)
file(MAKE_DIRECTORY ${MODULE_OUTPUT_PATH})

# ----------------------
# Helper functions
# ----------------------
# Create an imported target from include dir + library path
function(create_imported_library target_name include_dirs lib_path)
    if(NOT TARGET ${target_name})
        add_library(${target_name} UNKNOWN IMPORTED)
        set_target_properties(${target_name} PROPERTIES
                INTERFACE_INCLUDE_DIRECTORIES "${include_dirs}"
                IMPORTED_LOCATION "${lib_path}"
        )
    endif()
endfunction()

# Small helper for verbose messages
function(phgit_msg txt)
    if(PHGIT_VERBOSE_FIND)
        message(STATUS "[phgit] ${txt}")
    endif()
endfunction()

# ----------------------
# Platform-specific search defaults
# ----------------------
# Common prefix hints
set(_COMMON_PREFIXES)
if(APPLE)
    list(APPEND _COMMON_PREFIXES /opt/homebrew /usr/local)
elseif(WIN32)
    list(APPEND _COMMON_PREFIXES "C:/Program Files" "C:/Program Files (x86)" "C:/libs" "C:/vcpkg/installed")
else()
    list(APPEND _COMMON_PREFIXES /usr /usr/local /opt)
endif()

# Try pkg-config (if available)
find_package(PkgConfig QUIET)

# ----------------------
# Find libcurl robustly
# (sem alterações lógicas — mantive sua estratégia original)
# ----------------------
set(CURL_TARGET_NAME "")

if(DEFINED CURL_DIR AND EXISTS "${CURL_DIR}")
    phgit_msg("CURL_DIR provided by user: ${CURL_DIR}")
    find_path(_curl_inc curl/curl.h PATHS ${CURL_DIR}/include ${CURL_DIR}/include/curl NO_DEFAULT_PATH)
    find_library(_curl_lib NAMES libcurl curl curl-lib PATHS ${CURL_DIR}/lib ${CURL_DIR}/lib64 NO_DEFAULT_PATH)
    if(_curl_inc AND _curl_lib)
        create_imported_library(CURL::libcurl "${_curl_inc}" "${_curl_lib}")
        set(CURL_TARGET_NAME "CURL::libcurl")
    endif()
endif()

if(NOT CURL_TARGET_NAME)
    find_package(CURL QUIET)
    if(CURL_FOUND AND TARGET CURL::libcurl)
        phgit_msg("found CURL via find_package(CURL)")
        set(CURL_TARGET_NAME "CURL::libcurl")
    elseif(CURL_FOUND)
        if(DEFINED CURL_LIBRARY AND DEFINED CURL_INCLUDE_DIRS)
            create_imported_library(CURL::libcurl "${CURL_INCLUDE_DIRS}" "${CURL_LIBRARY}")
            set(CURL_TARGET_NAME "CURL::libcurl")
        endif()
    endif()
endif()

if(NOT CURL_TARGET_NAME AND PKG_CONFIG_FOUND)
    phgit_msg("Attempting to locate libcurl via pkg-config")
    pkg_check_modules(_pc_CURL libcurl curl QUIET)
    if(_pc_CURL_FOUND)
        create_imported_library(CURL::libcurl "${_pc_CURL_INCLUDE_DIRS}" "${_pc_CURL_LIBRARIES}")
        set(CURL_TARGET_NAME "CURL::libcurl")
    endif()
endif()

if(NOT CURL_TARGET_NAME)
    phgit_msg("Trying manual search for libcurl in common prefixes")
    foreach(_p IN LISTS _COMMON_PREFIXES)
        find_path(_curl_inc curl/curl.h PATHS ${_p} ${_p}/include ${_p}/include/curl)
        find_library(_curl_lib NAMES libcurl curl PATHS ${_p}/lib ${_p}/lib64 ${_p}/lib/x64)
        if(_curl_inc AND _curl_lib)
            create_imported_library(CURL::libcurl "${_curl_inc}" "${_curl_lib}")
            set(CURL_TARGET_NAME "CURL::libcurl")
            break()
        endif()
    endforeach()
endif()

if(NOT CURL_TARGET_NAME)
    message(FATAL_ERROR "Could not locate libcurl.
  Suggestions:
    - Install libcurl development package (apt: libcurl4-openssl-dev, dnf: libcurl-devel, pacman: curl)
    - On Windows: use vcpkg (vcpkg install curl:x64-windows) and pass -DCMAKE_TOOLCHAIN_FILE=.../vcpkg.cmake
    - Or provide -DCURL_DIR=path/to/curl (folder that contains include/ and lib/)")
endif()

# ----------------------
# Find Lua 5.4 (robust)
# ----------------------
set(LUA_TARGET_NAME "")

if(DEFINED LUA_DIR AND EXISTS "${LUA_DIR}")
    phgit_msg("LUA_DIR provided by user: ${LUA_DIR}")
    find_path(_lua_inc lua.h PATHS ${LUA_DIR}/include ${LUA_DIR}/include/lua5.4 ${LUA_DIR}/include/lua)
    find_library(_lua_lib NAMES lua54 lua5.4 lua PATHS ${LUA_DIR}/lib ${LUA_DIR}/lib64 NO_DEFAULT_PATH)
    if(_lua_inc AND _lua_lib)
        create_imported_library(Lua::Lua "${_lua_inc}" "${_lua_lib}")
        set(LUA_TARGET_NAME "Lua::Lua")
    endif()
endif()

if(NOT LUA_TARGET_NAME)
    find_package(Lua 5.4 QUIET)
    if(Lua_FOUND AND TARGET Lua::Lua)
        phgit_msg("found Lua via find_package(Lua 5.4)")
        set(LUA_TARGET_NAME "Lua::Lua")
    elseif(Lua_FOUND)
        if(DEFINED LUA_LIBRARIES OR DEFINED LUA_LIBRARY)
            if(DEFINED LUA_LIBRARIES)
                set(_lu_lib ${LUA_LIBRARIES})
            else()
                set(_lu_lib ${LUA_LIBRARY})
            endif()
            if(DEFINED LUA_INCLUDE_DIR OR DEFINED LUA_INCLUDE_DIRS)
                if(DEFINED LUA_INCLUDE_DIRS)
                    set(_lu_inc ${LUA_INCLUDE_DIRS})
                else()
                    set(_lu_inc ${LUA_INCLUDE_DIR})
                endif()
                add_library(Lua::Lua INTERFACE IMPORTED)
                if(_lu_lib)
                    set_target_properties(Lua::Lua PROPERTIES INTERFACE_LINK_LIBRARIES "${_lu_lib}")
                endif()
                if(_lu_inc)
                    set_target_properties(Lua::Lua PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${_lu_inc}")
                endif()
                set(LUA_TARGET_NAME "Lua::Lua")
            endif()
        endif()
    endif()
endif()

if(NOT LUA_TARGET_NAME AND PKG_CONFIG_FOUND)
    phgit_msg("Attempting to locate Lua via pkg-config")
    pkg_check_modules(_pc_LUA lua5.4 lua-5.4 lua liblua QUIET)
    if(_pc_LUA_FOUND)
        create_imported_library(Lua::Lua "${_pc_LUA_INCLUDE_DIRS}" "${_pc_LUA_LIBRARIES}")
        set(LUA_TARGET_NAME "Lua::Lua")
    endif()
endif()

if(NOT LUA_TARGET_NAME)
    message(FATAL_ERROR "Could not locate Lua 5.4 development files.
  Suggestions:
    - Install Lua 5.4 development package (apt: liblua5.4-dev, dnf: lua-devel, pacman: lua)
    - On macOS: brew install lua@5.4
    - On Windows: vcpkg install lua:x64-windows and pass -DCMAKE_TOOLCHAIN_FILE=.../vcpkg.cmake
    - Or provide -DLUA_DIR=path/to/lua (folder that contains include/ and lib/)")
endif()

# ----------------------
# nlohmann_json (fallback to FetchContent if not found)
# ----------------------
find_package(nlohmann_json 3.2.0 QUIET)
if(NOT nlohmann_json_FOUND)
    phgit_msg("nlohmann_json not found — fetching via FetchContent")
    include(FetchContent)
    FetchContent_Declare(
            nlohmann_json
            GIT_REPOSITORY https://github.com/nlohmann/json.git
            GIT_TAG v3.11.3
    )
    FetchContent_MakeAvailable(nlohmann_json)
endif()

# ----------------------
# Core targets and includes
# ----------------------
add_library(logger STATIC src/libs/liblogger/Logger.cpp)
# logger public includes
target_include_directories(logger PUBLIC ${CMAKE_SOURCE_DIR}/src/ipc/include)

# Gather core sources
file(GLOB_RECURSE CORE_C_SOURCES
        ${CMAKE_SOURCE_DIR}/src/core/cli/*.c
        ${CMAKE_SOURCE_DIR}/src/core/config/*.c
        ${CMAKE_SOURCE_DIR}/src/core/main/*.c
        ${CMAKE_SOURCE_DIR}/src/core/module_loader/*.c
        ${CMAKE_SOURCE_DIR}/src/core/platform/*.c
        ${CMAKE_SOURCE_DIR}/src/core/scripting/*.c
        ${CMAKE_SOURCE_DIR}/src/core/ui/*.c
        ${CMAKE_SOURCE_DIR}/src/libs/libcommon/*.c
)
add_executable(phgit ${CORE_C_SOURCES})

# Add general include directories (project headers)
target_include_directories(phgit PUBLIC
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/src/core
        ${CMAKE_SOURCE_DIR}/src/ipc/include
        ${CMAKE_SOURCE_DIR}/src/libs
)

# If imported targets provide include info we don't have to duplicate, but add safe guards
if(TARGET Lua::Lua)
    phgit_msg("Linking phgit with Lua::Lua")
    target_link_libraries(phgit PRIVATE Lua::Lua)
endif()
if(TARGET CURL::libcurl)
    phgit_msg("Linking phgit with CURL::libcurl")
    target_link_libraries(phgit PRIVATE CURL::libcurl)
endif()

# Always link the logger
target_link_libraries(phgit PRIVATE logger)

# Platform specific libraries
if(UNIX AND NOT APPLE)
    target_link_libraries(phgit PRIVATE dl m)
endif()

# ----------------------
# Export useful compile options and warnings
# ----------------------
# Set sane warnings and treat some as errors in CI
if(MSVC)
    target_compile_options(phgit PRIVATE /W4 /permissive-)
else()
    target_compile_options(phgit PRIVATE -Wall -Wextra -Wpedantic)
endif()

# ----------------------
# Rust module build macro (cargo integration)
# ----------------------
find_program(CARGO_EXECUTABLE cargo HINTS ENV PATH)
if(NOT CARGO_EXECUTABLE)
    phgit_msg("cargo not found in PATH — Rust modules will fail to build if enabled")
endif()

macro(add_rust_module module_path library_name)
    if(NOT CARGO_EXECUTABLE)
        message(WARNING "Cargo not found; skipping Rust module ${library_name}")
        return()
    endif()

    set(OUTPUT_LIB ${MODULE_OUTPUT_PATH}/${CMAKE_SHARED_LIBRARY_PREFIX}${library_name}${CMAKE_SHARED_LIBRARY_SUFFIX})
    set(CARGO_TARGET_DIR ${CMAKE_BINARY_DIR}/cargo_target)
    set(SOURCE_LIB ${CARGO_TARGET_DIR}/release/${CMAKE_SHARED_LIBRARY_PREFIX}${library_name}${CMAKE_SHARED_LIBRARY_SUFFIX})

    add_custom_command(
            OUTPUT ${OUTPUT_LIB}
            COMMAND ${CMAKE_COMMAND} -E env "CARGO_TARGET_DIR=${CARGO_TARGET_DIR}" ${CARGO_EXECUTABLE} build --release --manifest-path ${CMAKE_SOURCE_DIR}/src/modules/${module_path}/Cargo.toml
            COMMAND ${CMAKE_COMMAND} -E make_directory ${MODULE_OUTPUT_PATH}
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${SOURCE_LIB} ${OUTPUT_LIB}
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/src/modules/${module_path}
            COMMENT "Compiling Rust module ${library_name}"
            VERBATIM
    )
    add_custom_target(${library_name}_module ALL DEPENDS ${OUTPUT_LIB})
endmacro()

# Add Rust modules
add_rust_module(git_ops git_ops)
add_rust_module(issue_tracker issue_tracker)
add_rust_module(sync_engine sync_engine)
add_rust_module(api_client api_client)
add_rust_module(devops_automation devops_automation)
add_rust_module(ci_cd_manager/parser workflow_parser)

# ----------------------
# C++ shared module example
# ----------------------
add_library(ci_cd_visualizer SHARED src/modules/ci_cd_manager/visualizer/PipelineVisualizer.cpp)
target_include_directories(ci_cd_visualizer PUBLIC ${CMAKE_SOURCE_DIR}/src/ipc/include)
# Link JSON lib if available
if(TARGET nlohmann_json::nlohmann_json)
    target_link_libraries(ci_cd_visualizer PRIVATE nlohmann_json::nlohmann_json)
endif()
set_target_properties(ci_cd_visualizer PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${MODULE_OUTPUT_PATH})

# ----------------------
# Installer logic (isolated and documented)
#logica.instalador: Começo da seção dedicada ao instalador.
#logica.instalador: Tudo relacionado ao build/instalação do instalador fica aqui.
#logica.instalador: Se preferir, mova a implementação completa para installer/installer.cmake e use include().
# Example to extract to separate file:
#    file(READ "${CMAKE_SOURCE_DIR}/installer/installer.cmake" _dummy)  # placeholder
#    include("${CMAKE_SOURCE_DIR}/installer/installer.cmake")
#logica.instalador: Fim dos comentários gerais. Código real abaixo.
# ----------------------

if(PHGIT_BUILD_INSTALLER)
    #logica.instalador: Modo BUILD do instalador ativado.
    #logica.instalador: A pasta installer/ deve conter o CMakeLists.txt específico do instalador.
    phgit_msg("PHGIT_BUILD_INSTALLER=ON — adicionando subdiretório 'installer'")
    add_subdirectory(installer)

    #logica.instalador: Regras de instalação relacionadas ao instalador.
    #logica.instalador: Instala o arquivo de configuração do instalador, se existir.
    if(EXISTS "${CMAKE_SOURCE_DIR}/installer/config.json")
        install(FILES ${CMAKE_SOURCE_DIR}/installer/config.json DESTINATION bin)
    endif()
else()
    #logica.instalador: Instalador desativado (padrão). Para ativar, rode:
    #logica.instalador: cmake -S . -B build -DPHGIT_BUILD_INSTALLER=ON
    phgit_msg("PHGIT_BUILD_INSTALLER=OFF — instalador não será criado")
endif()

# ----------------------
# Install rules (núcleo do projeto)
# ----------------------
install(TARGETS phgit RUNTIME DESTINATION bin)
install(TARGETS ci_cd_visualizer LIBRARY DESTINATION bin/modules)
install(DIRECTORY ${MODULE_OUTPUT_PATH}/ DESTINATION bin/modules FILES_MATCHING PATTERN "*${CMAKE_SHARED_LIBRARY_SUFFIX}")

# ----------------------
# Final summary message
# ----------------------
phgit_msg("Configuration finished. Targets: phgit, logger, ci_cd_visualizer (if enabled), Rust modules (if cargo available).")
phgit_msg("To build with vcpkg: cmake -S . -B build -G Ninja -DCMAKE_TOOLCHAIN_FILE=/path/to/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x64-windows && cmake --build build")
phgit_msg("Or provide -DCURL_DIR=... -DLUA_DIR=... to help discovery on custom installs.")
