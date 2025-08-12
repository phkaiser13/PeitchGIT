cmake_minimum_required(VERSION 3.15)

project(gitph LANGUAGES C CXX VERSION 1.0.0)

# --- Global Settings and Options ---
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
include(FetchContent)

# Define output directories
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
set(MODULE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin/modules)
file(MAKE_DIRECTORY ${MODULE_OUTPUT_PATH})

# Allow users / CI to pass overrides:
# -D CMAKE_PREFIX_PATH=... or environment variables:
#   LUA_DIR, CURL_DIR, NLOHMANN_DIR, VCPKG_ROOT
if(DEFINED ENV{LUA_DIR} AND NOT DEFINED LUA_DIR)
    set(LUA_DIR $ENV{LUA_DIR})
endif()
if(DEFINED ENV{CURL_DIR} AND NOT DEFINED CURL_DIR)
    set(CURL_DIR $ENV{CURL_DIR})
endif()
if(DEFINED ENV{NLOHMANN_DIR} AND NOT DEFINED NLOHMANN_DIR)
    set(NLOHMANN_DIR $ENV{NLOHMANN_DIR})
endif()
if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "vcpkg toolchain")
endif()

# Add common Homebrew prefixes on macOS so find_package / pkg-config can discover brewed libs.
if(APPLE)
    if(EXISTS "/opt/homebrew/opt")
        list(APPEND CMAKE_PREFIX_PATH "/opt/homebrew/opt")
    endif()
    if(EXISTS "/usr/local/opt")
        list(APPEND CMAKE_PREFIX_PATH "/usr/local/opt")
    endif()
endif()

# Increase find debug verbosity optionally (uncomment for troubleshooting)
# set(CMAKE_FIND_DEBUG_MODE 1)

# --- Helper: try pkg-config with candidate names ---
find_package(PkgConfig QUIET)
function(try_pkg_config OUT_PREFIX)
    # call like: try_pkg_config(LUA lua5.4 lua liblua lua-5.4)
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
            # Export useful variables with prefix OUT_PREFIX (upper-case)
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
    # Modern FindCURL provides CURL::libcurl
    if(TARGET CURL::libcurl)
        set(CURL_TARGET_NAME "CURL::libcurl")
    else()
        # Create an imported target that wraps the variables from FindCURL
        add_library(CURL::libcurl UNKNOWN IMPORTED)
        set_target_properties(CURL::libcurl PROPERTIES
            IMPORTED_LOCATION "${CURL_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${CURL_INCLUDE_DIRS}"
        )
        set(CURL_TARGET_NAME "CURL::libcurl")
    endif()
else()
    # Try pkg-config fallback (multiple possible package names)
    try_pkg_config(CURL libcurl curl)
    if(CURL_PKG_FOUND)
        # Create imported target from pkg-config result
        add_library(CURL::libcurl UNKNOWN IMPORTED)
        # pkg-config may list multiple dirs, take the first library path for IMPORTED_LOCATION
        list(GET CURL_PKG_LIBRARY_DIRS 0 _curl_libdir)
        # Try to resolve a library path (best-effort)
        if(_curl_libdir)
            # We won't attempt to guess the exact filename. Use INTERFACE include dirs and link via -lcurl
            set_target_properties(CURL::libcurl PROPERTIES
                INTERFACE_INCLUDE_DIRECTORIES "${CURL_PKG_INCLUDE_DIRS}"
                INTERFACE_LINK_LIBRARIES "${CURL_PKG_LIBRARIES}"
            )
        else()
            set_target_properties(CURL::libcurl PROPERTIES
                INTERFACE_INCLUDE_DIRECTORIES "${CURL_PKG_INCLUDE_DIRS}"
                INTERFACE_LINK_LIBRARIES "${CURL_PKG_LIBRARIES}"
            )
        endif()
        set(CURL_TARGET_NAME "CURL::libcurl")
    endif()
endif()

if(NOT CURL_TARGET_NAME)
    message(FATAL_ERROR "Could not locate libcurl. Use system package manager (apt/dnf/pacman/brew/choco) or provide CURL_DIR or CMAKE_PREFIX_PATH.")
endif()

# --- Find Lua (try find_package, then pkg-config with multiple names) ---
set(LUA_TARGET_NAME "")
# Preferred: CMake's FindLua
find_package(Lua 5.4 QUIET)
if(Lua_FOUND)
    if(TARGET Lua::Lua)
        set(LUA_TARGET_NAME "Lua::Lua")
    else()
        # --- BLOCO CORRIGIDO ---
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

        # Criar como um alvo INTERFACE para lidar corretamente com listas de bibliotecas
        add_library(Lua::Lua INTERFACE IMPORTED)
        if(_lua_lib)
            # INTERFACE_LINK_LIBRARIES processa corretamente a lista separada por ;
            set_target_properties(Lua::Lua PROPERTIES
                INTERFACE_LINK_LIBRARIES "${_lua_lib}"
            )
        endif()
        if(_lua_inc)
            set_target_properties(Lua::Lua PROPERTIES
                INTERFACE_INCLUDE_DIRECTORIES "${_lua_inc}"
            )
        endif()
        set(LUA_TARGET_NAME "Lua::Lua")
        # --- FIM DO BLOCO CORRIGIDO ---
    endif()
else()
    # pkg-config fallback (try several common names)
    try_pkg_config(LUA lua5.4 lua-5.4 lua liblua lua52 lua53)
    if(LUA_PKG_FOUND)
        add_library(Lua::Lua UNKNOWN IMPORTED)
        # Use INTERFACE_LINK_LIBRARIES as pkg-config returns link flags like -llua-5.4
        set_target_properties(Lua::Lua PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${LUA_PKG_INCLUDE_DIRS}"
            INTERFACE_LINK_LIBRARIES "${LUA_PKG_LIBRARIES}"
        )
        set(LUA_TARGET_NAME "Lua::Lua")
    endif()
endif()

if(NOT LUA_TARGET_NAME)
    message(FATAL_ERROR "Could not locate Lua 5.4 development files. Install via your package manager (apt/dnf/pacman/brew/choco) or provide LUA_DIR / CMAKE_PREFIX_PATH.")
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

# --- Summary of resolved targets (informational) ---
message(STATUS "Using CURL target: ${CURL_TARGET_NAME}")
message(STATUS "Using Lua target: ${LUA_TARGET_NAME}")
if(nlohmann_json_FOUND)
    message(STATUS "nlohmann_json: found (package)")
else()
    message(STATUS "nlohmann_json: provided by FetchContent")
endif()

# --- Compile C/C++ Core ---

# Logger library
add_library(logger STATIC src/libs/liblogger/Logger.cpp)
target_include_directories(logger PUBLIC ${CMAKE_SOURCE_DIR}/src/ipc/include)

# Collect C sources
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

# Add include directories
target_include_directories(gitph PUBLIC
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/core
    ${CMAKE_SOURCE_DIR}/src/ipc/include
    ${CMAKE_SOURCE_DIR}/src/libs
)

# If the find_package produced include dirs as variables, add them
if(DEFINED LUA_INCLUDE_DIR)
    target_include_directories(gitph PUBLIC ${LUA_INCLUDE_DIR})
endif()
if(DEFINED Lua_PKG_INCLUDE_DIRS)
    target_include_directories(gitph PUBLIC ${Lua_PKG_INCLUDE_DIRS})
endif()
if(DEFINED CURL_INCLUDE_DIRS)
    target_include_directories(gitph PUBLIC ${CURL_INCLUDE_DIRS})
endif()
if(DEFINED CURL_PKG_INCLUDE_DIRS)
    target_include_directories(gitph PUBLIC ${CURL_PKG_INCLUDE_DIRS})
endif()

# Link the main executable with dependencies using the resolved imported targets
target_link_libraries(gitph
    PRIVATE
    logger
    ${CURL_TARGET_NAME}
    ${LUA_TARGET_NAME}
)

# On POSIX systems (except macOS), link dl and m if needed
if(UNIX AND NOT APPLE)
    target_link_libraries(gitph PRIVATE dl m)
endif()

# --- Compile Polyglot Modules ---

# Macro aprimorada para compilar módulos Rust.
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

# Compila o parser de CI/CD como um módulo Rust, usando o caminho aninhado e o nome correto da biblioteca.
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

# visualizer (C++)
add_library(ci_cd_visualizer SHARED src/modules/ci_cd_manager/visualizer/PipelineVisualizer.cpp)
target_include_directories(ci_cd_visualizer PUBLIC
    ${CMAKE_SOURCE_DIR}/src/ipc/include
)
# link logger and nlohmann_json
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

# CORREÇÃO: Consolidado a criação do executável do instalador em um único bloco.
# Este helper só será compilado no Windows, para ser usado pelo NSIS.
if(WIN32)
    add_executable(installer_helper installer/src/installer_main.c installer/src/download.c)
    # Link installer to libcurl via the same resolved target
    target_link_libraries(installer_helper PRIVATE ${CURL_TARGET_NAME})
    set_target_properties(installer_helper PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/installer")
endif()
