#!/bin/bash
# Copyright (C) 2025 Pedro Henrique / phkaiser13
# File: uninstall.sh (macOS)
# This script cleanly removes a script-based installation of phgit from a
# macOS system. It is the direct counterpart to the macOS install.sh script,
# removing the application directory and the symbolic link. It includes a
# confirmation prompt to prevent accidental uninstallation.
#
# SPDX-License-Identifier: Apache-2.0

# --- Script Configuration and Safety ---
set -u

# --- Helper Functions and Variables ---
C_INFO='\033[0;34m'
C_SUCCESS='\033[0;32m'
C_WARN='\033[0;33m'
C_RESET='\033[0m'

info() {
    printf "${C_INFO}==> %s${C_RESET}\n" "$1"
}

success() {
    printf "${C_SUCCESS}==> %s${C_RESET}\n" "$1"
}

# --- Main Uninstallation Logic ---
main() {
    info "Starting phgit uninstallation from macOS..."

    # Define the paths where phgit was installed.
    local INSTALL_DIR="$HOME/.phgit"
    local BIN_DIR="$HOME/.local/bin"
    local SYMLINK_PATH="$BIN_DIR/phgit"

    # Safety first: Confirm with the user.
    printf "${C_WARN}This will remove phgit from your system by deleting:\n"
    printf " - The symbolic link: %s\n" "$SYMLINK_PATH"
    printf " - The installation directory: %s\n${C_RESET}" "$INSTALL_DIR"
    read -p "Are you sure you want to continue? [y/N] " -r REPLY
    echo

    if [[ ! "$REPLY" =~ ^[Yy]$ ]]; then
        info "Uninstallation cancelled."
        exit 0
    fi

    # 1. Remove the symbolic link.
    if [ -L "$SYMLINK_PATH" ]; then
        info "Removing symbolic link: $SYMLINK_PATH"
        rm "$SYMLINK_PATH"
    else
        info "Symbolic link not found (already removed)."
    fi

    # 2. Remove the main installation directory.
    if [ -d "$INSTALL_DIR" ]; then
        info "Removing installation directory: $INSTALL_DIR"
        rm -rf "$INSTALL_DIR"
    else
        info "Installation directory not found (already removed)."
    fi

    # 3. Inform the user about the PATH variable.
    printf "\n"
    info "If you added '$BIN_DIR' to your shell's startup file (~/.zshrc, etc.), you may remove that line manually."

    success "phgit has been successfully uninstalled."
}

main