# Copyright (C) 2025 Pedro Henrique / phkaiser13
# CMakeLists.txt - Build orchestration for the gitph polyglot project.
#
# This CMake script is the central nervous system of our build process.
# It's responsible for compiling the C/C++ core, invoking native build
# tools for Rust (cargo) and Go (go build), and linking everything into a
# distributable application.
#
# SPDX-License-Identifier: Apache-2.0

# FIX: Complete `CMakeLists.txt` with improved, cross-platform library discovery and robust fallbacks.
cmake\_minimum\_required(VERSION 3.15)

project(gitph LANGUAGES C CXX VERSION 1.0.0)

# --- Global Settings and Options ---

set(CMAKE\_CXX\_STANDARD 17)
set(CMAKE\_CXX\_STANDARD\_REQUIRED ON)
include(FetchContent) # Use FetchContent for optional fetch/build fallbacks
include(GNUInstallDirs)

# Keep output directories consistent

set(EXECUTABLE\_OUTPUT\_PATH \${CMAKE\_BINARY\_DIR}/bin)
set(LIBRARY\_OUTPUT\_PATH \${CMAKE\_BINARY\_DIR}/bin)
set(MODULE\_OUTPUT\_PATH \${CMAKE\_BINARY\_DIR}/bin/modules)
file(MAKE\_DIRECTORY \${MODULE\_OUTPUT\_PATH})

# --- Help users by appending common package-manager prefixes to CMAKE\_PREFIX\_PATH ---

# This makes it easier for find\_package() to locate Homebrew, vcpkg, etc.

# Users can still override CMAKE\_PREFIX\_PATH on the command line.

list(APPEND CMAKE\_PREFIX\_PATH
/opt/homebrew          # Apple Silicon Homebrew
/usr/local             # Intel Homebrew / common prefix
/usr/local/opt         # Homebrew opt layout
\$ENV{VCPKG\_ROOT}       # vcpkg (if set)
)

message(STATUS "CMAKE\_PREFIX\_PATH = \${CMAKE\_PREFIX\_PATH}")

# --- Helper function: prefer imported target if available, fallback to variables ---

macro(link\_target\_or\_vars target\_var var\_libs var\_includes out\_link\_var out\_include\_var)
if(TARGET \${target\_var})
set(\${out\_link\_var} \${target\_var} PARENT\_SCOPE)
set(\${out\_include\_var} "" PARENT\_SCOPE)
else()
set(\${out\_link\_var} \${\${var\_libs}} PARENT\_SCOPE)
set(\${out\_include\_var} \${\${var\_includes}} PARENT\_SCOPE)
endif()
endmacro()

# --- Find CMake/PKG-config helpers ---

find\_package(PkgConfig QUIET) # Optional; helpful for some distros

# --- Find CURL ---

# Try target-based find first, then variable-based fallback.

find\_package(CURL QUIET)
if(CURL\_FOUND)
message(STATUS "Found libcurl: \${CURL\_VERSION}")
else()
message(WARNING "libcurl not found by find\_package(CURL). If building locally, install system package (apt/dnf/brew/choco) or use vcpkg, or set CMAKE\_PREFIX\_PATH.")
endif()

# --- Find Lua (robust, multi-strategy for cross-distro/Homebrew/vcpkg) ---

set(\_LUA\_FOUND FALSE)

# 1) Try CMake's FindLua

find\_package(Lua 5.4 QUIET)
if(Lua\_FOUND)
set(\_LUA\_FOUND TRUE)
message(STATUS "Found Lua via CMake's FindLua module.")
endif()

# 2) Try pkg-config names if pkg-config is available

if(NOT \_LUA\_FOUND AND PkgConfig\_FOUND)
\# Try multiple common pkg-config names
foreach(\_lua\_pc\_name lua5.4 lua-5.4 lua)
pkg\_check\_modules(LUA\_PKG QUIET \${\_lua\_pc\_name})
if(LUA\_PKG\_FOUND)
set(\_LUA\_FOUND TRUE)
set(LUA\_INCLUDE\_DIRS \${LUA\_PKG\_INCLUDE\_DIRS})
set(LUA\_LIBRARIES \${LUA\_PKG\_LIBS})
message(STATUS "Found Lua via pkg-config: \${\_lua\_pc\_name}")
break()
endif()
endforeach()
endif()

# 3) Probe common Homebrew/vcpkg prefixes for lua or lua\@5.4

if(NOT \_LUA\_FOUND)
foreach(\_p /opt/homebrew /usr/local /usr/local/opt \$ENV{VCPKG\_ROOT})
if(NOT \_p)
continue()
endif()
\# check for lua\@5.4 or lua
if(EXISTS "\${\_p}/opt/lua\@5.4" OR EXISTS "\${\_p}/opt/lua")
if(EXISTS "\${\_p}/opt/lua\@5.4")
set(\_candidate "\${\_p}/opt/lua\@5.4")
else()
set(\_candidate "\${\_p}/opt/lua")
endif()
if(EXISTS "\${\_candidate}/include")
set(LUA\_INCLUDE\_DIRS "\${\_candidate}/include")
endif()
\# prefer shared library names
if(EXISTS "\${\_candidate}/lib/liblua.dylib")
set(LUA\_LIBRARIES "\${\_candidate}/lib/liblua.dylib")
elseif(EXISTS "\${\_candidate}/lib/liblua.a")
set(LUA\_LIBRARIES "\${\_candidate}/lib/liblua.a")
elseif(EXISTS "\${\_candidate}/lib/liblua5.4.dylib")
set(LUA\_LIBRARIES "\${\_candidate}/lib/liblua5.4.dylib")
endif()
if(LUA\_INCLUDE\_DIRS OR LUA\_LIBRARIES)
set(\_LUA\_FOUND TRUE)
message(STATUS "Found Lua in prefix: \${\_candidate}")
break()
endif()
endif()
endforeach()
endif()

# 4) Final fallback: instruct the user

if(NOT \_LUA\_FOUND)
message(FATAL\_ERROR
"Lua 5.4 not found. Install Lua (system package, Homebrew 'lua' or 'lua\@5.4', vcpkg, or set CMAKE\_PREFIX\_PATH). "
"Example: 'brew install lua' or 'sudo apt install liblua5.4-dev'.")
endif()

# Normalize variables commonly used later

# CMake's FindLua sometimes sets LUA\_INCLUDE\_DIR and LUA\_LIBRARIES or other variations.

if(DEFINED Lua\_INCLUDE\_DIR)
set(LUA\_INCLUDE\_DIRS \${Lua\_INCLUDE\_DIR})
endif()
if(DEFINED Lua\_LIBRARY)
set(LUA\_LIBRARIES \${Lua\_LIBRARY})
elseif(DEFINED Lua\_LIBRARIES)
set(LUA\_LIBRARIES \${Lua\_LIBRARIES})
endif()

# Provide an imported target if available (CMake 3.20+ or vcpkg might provide one)

if(TARGET Lua::Lua)
set(LUA\_LINK\_TARGET Lua::Lua)
else()
set(LUA\_LINK\_TARGET "\${LUA\_LIBRARIES}")
endif()

message(STATUS "Lua include dirs: \${LUA\_INCLUDE\_DIRS}")
message(STATUS "Lua libraries: \${LUA\_LIBRARIES}")

# --- Find nlohmann\_json ---

find\_package(nlohmann\_json 3.2.0 QUIET)
if(nlohmann\_json\_FOUND)
message(STATUS "Found nlohmann\_json via find\_package.")
else()
message(STATUS "nlohmann\_json not found on system. Will fetch using FetchContent.")
FetchContent\_Declare(
nlohmann\_json
GIT\_REPOSITORY [https://github.com/nlohmann/json.git](https://github.com/nlohmann/json.git)
GIT\_TAG v3.11.3
)
FetchContent\_MakeAvailable(nlohmann\_json)
message(STATUS "nlohmann\_json fetched and made available.")
endif()

# --- Project sources ---

# Logger

add\_library(logger STATIC src/libs/liblogger/Logger.cpp)
target\_include\_directories(logger PUBLIC \${CMAKE\_SOURCE\_DIR}/src/ipc/include)

# Core C sources

file(GLOB\_RECURSE CORE\_SOURCES
"src/core/cli/*.c"
"src/core/config/*.c"
"src/core/main/*.c"
"src/core/module\_loader/*.c"
"src/core/platform/*.c"
"src/core/scripting/*.c"
"src/core/ui/*.c"
"src/libs/libcommon/*.c"
)

add\_executable(gitph \${CORE\_SOURCES})

# --- Include directories for the main executable ---

target\_include\_directories(gitph PUBLIC
\${CMAKE\_SOURCE\_DIR}/src
\${CMAKE\_SOURCE\_DIR}/src/core
\${CMAKE\_SOURCE\_DIR}/src/ipc/include
\${CMAKE\_SOURCE\_DIR}/src/libs
)

# Add Lua include dir if available

if(LUA\_INCLUDE\_DIRS)
target\_include\_directories(gitph PUBLIC \${LUA\_INCLUDE\_DIRS})
endif()

# Use modern target for libcurl if available

if(TARGET CURL::libcurl)
set(\_CURL\_LINK\_TARGET CURL::libcurl)
\# CURL::libcurl brings include dirs automatically
else()
if(DEFINED CURL\_LIBRARIES)
set(\_CURL\_LINK\_TARGET \${CURL\_LIBRARIES})
elseif(DEFINED CURL\_LIBRARY)
set(\_CURL\_LINK\_TARGET \${CURL\_LIBRARY})
else()
set(\_CURL\_LINK\_TARGET "")
endif()

```
# If find_package(CURL) provided include directories:
if(DEFINED CURL_INCLUDE_DIRS)
    target_include_directories(gitph PUBLIC ${CURL_INCLUDE_DIRS})
endif()
```

endif()

# Link the main executable with dependencies.

# Prefer imported targets; if not present use variables discovered earlier.

set(\_link\_list logger)

if(\_CURL\_LINK\_TARGET)
list(APPEND \_link\_list \${\_CURL\_LINK\_TARGET})
else()
message(WARNING "libcurl target/vars not found. Linking may fail; ensure curl dev library is installed.")
endif()

if(LUA\_LINK\_TARGET)
list(APPEND \_link\_list \${LUA\_LINK\_TARGET})
else()
message(FATAL\_ERROR "Lua link target/vars not configured correctly.")
endif()

target\_link\_libraries(gitph PRIVATE \${\_link\_list})

# On POSIX systems, link to dl and m where appropriate

if(UNIX AND NOT APPLE)
target\_link\_libraries(gitph PRIVATE dl m)
endif()

# --- Module build macros (Rust / Go) ---

macro(add\_rust\_module module\_name)
add\_custom\_command(
OUTPUT \${MODULE\_OUTPUT\_PATH}/\${CMAKE\_SHARED\_LIBRARY\_PREFIX}\${module\_name}\${CMAKE\_SHARED\_LIBRARY\_SUFFIX}
COMMAND \${CMAKE\_COMMAND} -E env "CARGO\_TARGET\_DIR=\${CMAKE\_BINARY\_DIR}/cargo\_target" cargo build --release --manifest-path \${CMAKE\_SOURCE\_DIR}/src/modules/\${module\_name}/Cargo.toml
COMMAND \${CMAKE\_COMMAND} -E copy
\${CMAKE\_BINARY\_DIR}/cargo\_target/release/\${CMAKE\_SHARED\_LIBRARY\_PREFIX}\${module\_name}\${CMAKE\_SHARED\_LIBRARY\_SUFFIX}
\${MODULE\_OUTPUT\_PATH}/
WORKING\_DIRECTORY \${CMAKE\_SOURCE\_DIR}/src/modules/\${module\_name}
COMMENT "Compiling Rust module: \${module\_name}"
VERBATIM
)
add\_custom\_target(build\_\${module\_name} ALL
DEPENDS \${MODULE\_OUTPUT\_PATH}/\${CMAKE\_SHARED\_LIBRARY\_PREFIX}\${module\_name}\${CMAKE\_SHARED\_LIBRARY\_SUFFIX}
)
endmacro()

macro(add\_go\_module module\_name)
add\_custom\_command(
OUTPUT \${MODULE\_OUTPUT\_PATH}/\${CMAKE\_SHARED\_LIBRARY\_PREFIX}\${module\_name}\${CMAKE\_SHARED\_LIBRARY\_SUFFIX}
COMMAND go build -buildmode=c-shared -o \${MODULE\_OUTPUT\_PATH}/\${CMAKE\_SHARED\_LIBRARY\_PREFIX}\${module\_name}\${CMAKE\_SHARED\_LIBRARY\_SUFFIX} .
WORKING\_DIRECTORY \${CMAKE\_SOURCE\_DIR}/src/modules/\${module\_name}
COMMENT "Compiling Go module: \${module\_name}"
VERBATIM
)
add\_custom\_target(build\_\${module\_name} ALL
DEPENDS \${MODULE\_OUTPUT\_PATH}/\${CMAKE\_SHARED\_LIBRARY\_PREFIX}\${module\_name}\${CMAKE\_SHARED\_LIBRARY\_SUFFIX}
)
endmacro()

# Add modules

add\_rust\_module(git\_ops)
add\_rust\_module(issue\_tracker)
add\_rust\_module(sync\_engine)

add\_go\_module(api\_client)
add\_go\_module(devops\_automation)

# --- Specific module: ci\_cd\_manager ---

add\_custom\_command(
OUTPUT \${MODULE\_OUTPUT\_PATH}/\${CMAKE\_SHARED\_LIBRARY\_PREFIX}ci\_cd\_parser\${CMAKE\_SHARED\_LIBRARY\_SUFFIX}
COMMAND go build -buildmode=c-shared -o \${MODULE\_OUTPUT\_PATH}/\${CMAKE\_SHARED\_LIBRARY\_PREFIX}ci\_cd\_parser\${CMAKE\_SHARED\_LIBRARY\_SUFFIX} parser.go
WORKING\_DIRECTORY \${CMAKE\_SOURCE\_DIR}/src/modules/ci\_cd\_manager
COMMENT "Compiling CI/CD Go parser"
VERBATIM
)
add\_custom\_target(build\_ci\_cd\_parser ALL
DEPENDS \${MODULE\_OUTPUT\_PATH}/\${CMAKE\_SHARED\_LIBRARY\_PREFIX}ci\_cd\_parser\${CMAKE\_SHARED\_LIBRARY\_SUFFIX}
)

add\_library(ci\_cd\_visualizer SHARED src/modules/ci\_cd\_manager/visualizer/PipelineVisualizer.cpp)
target\_include\_directories(ci\_cd\_visualizer PUBLIC
\${CMAKE\_SOURCE\_DIR}/src/ipc/include
)

# Link visualizer against logger and nlohmann\_json, prefer imported target

if(TARGET nlohmann\_json::nlohmann\_json)
target\_link\_libraries(ci\_cd\_visualizer PRIVATE logger nlohmann\_json::nlohmann\_json)
else()
\# If FetchContent provided a target, it will generally be available as nlohmann\_json::nlohmann\_json
\# Otherwise include its headers via include path if available.
target\_link\_libraries(ci\_cd\_visualizer PRIVATE logger)
endif()

set\_target\_properties(ci\_cd\_visualizer PROPERTIES
LIBRARY\_OUTPUT\_DIRECTORY "\${MODULE\_OUTPUT\_PATH}"
)

# --- Installer target ---

add\_executable(installer\_main installer/src/installer\_main.c installer/src/download.c)

# Link curl to installer (prefer imported target)

if(TARGET CURL::libcurl)
target\_link\_libraries(installer\_main PRIVATE CURL::libcurl)
elseif(DEFINED CURL\_LIBRARIES)
target\_link\_libraries(installer\_main PRIVATE \${CURL\_LIBRARIES})
else()
message(WARNING "installer\_main: libcurl not found; installer linking may fail.")
endif()
set\_target\_properties(installer\_main PROPERTIES RUNTIME\_OUTPUT\_DIRECTORY "\${CMAKE\_BINARY\_DIR}/installer")

# --- Installation rules ---

install(TARGETS gitph DESTINATION \${CMAKE\_INSTALL\_BINDIR})
install(
DIRECTORY \${MODULE\_OUTPUT\_PATH}/
DESTINATION \${CMAKE\_INSTALL\_BINDIR}/modules
FILES\_MATCHING PATTERN "*.so" PATTERN "*.dll" PATTERN "\*.dylib"
)

# --- Helpful user messages ---

message(STATUS "Configuration complete. If you still see 'library not found' during link time on macOS or other systems, try:")
message(STATUS "  - Passing explicit prefix paths: -DCMAKE\_PREFIX\_PATH=/opt/homebrew:/usr/local")
message(STATUS "  - On macOS (Apple Silicon): brew install lua curl nlohmann-json")
message(STATUS "  - On Debian/Ubuntu: sudo apt install liblua5.4-dev libcurl4-openssl-dev    (or libcurl4-gnutls-dev)")
message(STATUS "  - On Fedora: sudo dnf install lua-devel libcurl-devel")
message(STATUS "  - With vcpkg: set -DCMAKE\_TOOLCHAIN\_FILE=/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake and install packages there.")
message(STATUS "  - To debug linker errors, re-run the build with VERBOSE=1 (e.g., cmake --build . -- VERBOSE=1).")

# End of CMakeLists.txt
