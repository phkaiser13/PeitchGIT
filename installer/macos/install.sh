#!/bin/bash
# Copyright (C) 2025 Pedro Henrique / phkaiser13
# File: install.sh (macOS)
# This script provides a user-friendly, non-root, command-line installation
# method for phgit on macOS. It serves as an alternative to Homebrew or a .pkg
# installer. It installs the application and its helper, runs the helper to
# manage dependencies, and places the executable in a standard user-local
# binary directory.
#
# SPDX-License-Identifier: Apache-2.0

# --- Script Configuration and Safety ---
set -eu

# --- Helper Functions and Variables ---
C_INFO='\033[0;34m'
C_SUCCESS='\033[0;32m'
C_WARN='\033[0;33m'
C_ERROR='\033[0;31m'
C_RESET='\033[0m'

info() {
    printf "${C_INFO}==> %s${C_RESET}\n" "$1"
}

success() {
    printf "${C_SUCCESS}==> %s${C_RESET}\n" "$1"
}

warn() {
    printf "${C_WARN}==> WARNING: %s${C_RESET}\n" "$1"
}

# --- Main Installation Logic ---
main() {
    info "Starting phgit installation for macOS..."

    # 1. Define installation paths.
    local INSTALL_DIR="$HOME/.phgit"
    local BIN_DIR="$HOME/.local/bin"
    
    # Assume the script is run from the extracted archive root.
    local SOURCE_BIN_DIR
    SOURCE_BIN_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/../bin" # Assumes script is in installer/macos

    # 2. Verify that the source binaries exist.
    if [[ ! -f "$SOURCE_BIN_DIR/phgit" || ! -f "$SOURCE_BIN_DIR/installer_helper" ]]; then
        printf "${C_ERROR}FATAL: Could not find 'phgit' or 'installer_helper' in '%s'.\n" "$SOURCE_BIN_DIR"
        printf "Please ensure you have the correct binaries for macOS.${C_RESET}\n"
        exit 1
    fi

    # 3. Create installation directories.
    info "Creating installation directory at $INSTALL_DIR"
    mkdir -p "$INSTALL_DIR"
    mkdir -p "$BIN_DIR"

    # 4. Copy application files and set permissions.
    info "Installing phgit files..."
    cp "$SOURCE_BIN_DIR/phgit" "$INSTALL_DIR/phgit"
    cp "$SOURCE_BIN_DIR/installer_helper" "$INSTALL_DIR/installer_helper"
    chmod +x "$INSTALL_DIR/phgit"
    chmod +x "$INSTALL_DIR/installer_helper"

    # 5. Run the C++ dependency helper.
    info "Running dependency check (this may download dependencies)..."
    if ! "$INSTALL_DIR/installer_helper"; then
        printf "${C_ERROR}The dependency installer failed. phgit may not work correctly.${C_RESET}\n"
        exit 1
    fi
    info "Dependency check complete."

    # 6. Create a symbolic link in the user's binary path.
    info "Placing phgit executable in $BIN_DIR"
    ln -sf "$INSTALL_DIR/phgit" "$BIN_DIR/phgit"

    # 7. Check if the installation directory is in the user's PATH.
    if [[ ":$PATH:" != *":$BIN_DIR:"* ]]; then
        warn "Your PATH variable does not seem to include '$BIN_DIR'."
        printf "To use the 'phgit' command, please add the following line to your shell's startup file (~/.zshrc or ~/.bash_profile):\n\n"
        printf "    ${C_SUCCESS}export PATH=\"\$HOME/.local/bin:\$PATH\"${C_RESET}\n\n"
        printf "After adding it, please restart your terminal or run 'source ~/.zshrc' (or equivalent).\n"
    fi

    success "phgit has been installed successfully!"
    printf "You can now use the 'phgit' command. Try running 'phgit --version'.\n"
}

main