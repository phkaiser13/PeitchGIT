#!/bin/bash
# Copyright (C) 2025 Pedro Henrique
# check_deps.sh - Verifies that all necessary build dependencies are installed.
#
# This script checks for the presence of compilers and tools required
# to build the gitph project from source.
#
# SPDX-License-Identifier: GPL-3.0-or-later

set -e
echo "--- [GITPH] Checking for required build dependencies ---"

HAS_ERROR=0

# Helper function to check for a command
check_command() {
    if command -v "$1" &> /dev/null; then
        echo "[ OK ] Found '$1'"
    else
        echo "[FAIL] Missing '$1'. Please install it and ensure it's in your PATH."
        HAS_ERROR=1
    fi
}

check_command "gcc"
check_command "g++"
check_command "cmake"
check_command "go"
check_command "cargo"
check_command "make"

echo ""
echo "Note: Development headers for 'libcurl' and 'lua5.4' are also required."
echo "CMake will perform the final check for these libraries during configuration."
echo ""

if [ "$HAS_ERROR" -ne 0 ]; then
    echo "One or more dependencies are missing. Please install them before proceeding."
    exit 1
else
    echo "All primary tool dependencies seem to be installed."
fi