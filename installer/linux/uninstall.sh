#!/bin/bash
# Copyright (C) 2025 Pedro Henrique / phkaiser13
# File: uninstall.sh
# This script cleanly removes phgit from a Linux system. It is designed to
# be the direct counterpart to the install.sh script, removing the application
# directory and the symbolic link from the user's local binary path. It includes
# a confirmation prompt to prevent accidental uninstallation.
#
# SPDX-License-Identifier: Apache-2.0

# --- Script Configuration and Safety ---
set -u # Treat unset variables as an error.

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
    info "Starting phgit uninstallation..."

    # Define the paths where phgit was installed.
    # These must match the paths used in the install.sh script.
    local INSTALL_DIR="$HOME/.phgit"
    local BIN_DIR="$HOME/.local/bin"
    local SYMLINK_PATH="$BIN_DIR/phgit"

    # Safety first: Confirm with the user before deleting anything.
    printf "${C_WARN}This will remove phgit from your system by deleting:\n"
    printf " - The symbolic link: %s\n" "$SYMLINK_PATH"
    printf " - The installation directory: %s\n${C_RESET}" "$INSTALL_DIR"
    read -p "Are you sure you want to continue? [y/N] " -r REPLY
    echo # Move to a new line

    if [[ ! "$REPLY" =~ ^[Yy]$ ]]; then
        info "Uninstallation cancelled."
        exit 0
    fi

    # 1. Remove the symbolic link from the user's binary path.
    if [ -L "$SYMLINK_PATH" ]; then
        info "Removing symbolic link: $SYMLINK_PATH"
        rm "$SYMLINK_PATH"
    else
        info "Symbolic link not found (already removed)."
    fi

    # 2. Remove the main installation directory.
    if [ -d "$INSTALL_DIR" ]; then
        info "Removing installation directory: $INSTALL_DIR"
        # Use rm -rf to remove the directory and all its contents.
        rm -rf "$INSTALL_DIR"
    else
        info "Installation directory not found (already removed)."
    fi

    # 3. Inform the user about the PATH variable.
    # We don't automatically edit user config files as it's risky.
    printf "\n"
    info "If you added '$BIN_DIR' to your shell's startup file (~/.bashrc, ~/.zshrc), you may remove that line manually."

    success "phgit has been successfully uninstalled."
}

# Execute the main function.
main