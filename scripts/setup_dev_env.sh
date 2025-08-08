#!/bin/bash
# Copyright (C) 2025 Pedro Henrique
# setup_dev_env.sh - Attempts to install development dependencies.
#
# This script detects the package manager on common POSIX systems and tries
# to install the required libraries and toolchains for building gitph.
#
# SPDX-License-Identifier: GPL-3.0-or-later

set -e
echo "--- [GITPH] Setting up development environment ---"

if [[ "$(id -u)" -ne 0 ]]; then
  SUDO="sudo"
else
  SUDO=""
fi

if command -v apt-get &> /dev/null; then
    echo "--> Detected Debian/Ubuntu based system."
    $SUDO apt-get update
    $SUDO apt-get install -y build-essential cmake golang rustc liblua5.4-dev libcurl4-openssl-dev
elif command -v dnf &> /dev/null; then
    echo "--> Detected Fedora/CentOS based system."
    $SUDO dnf install -y gcc-c++ cmake golang rust cargo lua5.4-devel libcurl-devel
elif command -v pacman &> /dev/null; then
    echo "--> Detected Arch Linux based system."
    $SUDO pacman -Syu --noconfirm base-devel cmake go rust lua54 curl
elif command -v brew &> /dev/null; then
    echo "--> Detected macOS with Homebrew."
    brew install cmake go rust lua curl
else
    echo "--> Could not detect a supported package manager (apt, dnf, pacman, brew)."
    echo "Please install the following dependencies manually:"
    echo "  - C/C++ Compiler (gcc/g++ or clang)"
    echo "  - CMake"
    echo "  - Go (>= 1.18)"
    echo "  - Rust (cargo)"
    echo "  - Lua 5.4 (development headers)"
    echo "  - libcurl (development headers)"
    exit 1
fi

echo ""
echo "--- [GITPH] Environment setup complete! ---"
echo "You should now be able to run 'make' or './scripts/build.sh'."