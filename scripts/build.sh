#!/bin/bash
# Copyright (C) 2025 Pedro Henrique
# build.sh - Build script for gitph on POSIX systems.
#
# This script orchestrates the build process by calling CMake to configure
# the project and then to compile all C/C++/Go/Rust components.
#
# SPDX-License-Identifier: GPL-3.0-or-later

# Exit immediately if a command exits with a non-zero status.
set -e

# --- Variables ---
BUILD_DIR="build"

# --- Main Logic ---
echo "--- [GITPH] Configuring project with CMake ---"
# Generate the build system in the build/ directory.
cmake -S . -B "${BUILD_DIR}"

echo ""
echo "--- [GITPH] Compiling all modules and core application ---"
# Run the actual build process (compiles C/C++, Go, Rust).
cmake --build "${BUILD_DIR}" --parallel

echo ""
echo "--- [GITPH] Build complete! ---"
echo "Executable is available at: ${BUILD_DIR}/bin/gitph"
echo "Modules are in: ${BUILD_DIR}/bin/modules/"