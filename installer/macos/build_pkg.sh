#!/bin/bash
# Copyright (C) 2025 Pedro Henrique / phkaiser13
# File: build_pkg.sh
# This script automates the creation of a macOS installer package (.pkg) for phgit.
# It uses Apple's command-line tools (pkgbuild) to create a component package
# that installs the phgit binary and its helper into /usr/local/bin, a standard
# location for user-installed command-line tools on macOS.
#
# SPDX-License-Identifier: Apache-2.0

set -eu

# --- Configuration ---
VERSION="1.0.0"
# A unique identifier for your package, usually in reverse domain name notation.
IDENTIFIER="com.phkaiser13.phgit"

# --- Paths ---
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DIST_DIR="$SCRIPT_DIR/dist"
# Staging area for the files that will be packaged.
STAGING_DIR="$SCRIPT_DIR/build/pkg_root"
# Location of the compiled phgit binaries (for macOS).
SOURCE_BIN_DIR="$SCRIPT_DIR/../../build/bin"

# --- Helper Functions ---
info() {
    printf "\033[0;34m==> %s\033[0m\n" "$1"
}
success() {
    printf "\033[0;32m==> %s\033[0m\n" "$1"
}
error() {
    printf "\033[0;31mERROR: %s\033[0m\n" "$1"
    exit 1
}

# --- Main Build Logic ---
main() {
    info "Starting .pkg installer build for phgit v$VERSION"

    # 1. Check for Xcode command-line tools.
    if ! xcode-select -p &>/dev/null; then
        error "Xcode Command Line Tools are required to build packages. Please run 'xcode-select --install'."
    fi

    # 2. Clean up and create the staging directory.
    info "Setting up build environment..."
    rm -rf "$STAGING_DIR"
    rm -rf "$DIST_DIR"
    mkdir -p "$DIST_DIR"
    # The structure inside STAGING_DIR mimics the target filesystem.
    mkdir -p "$STAGING_DIR/usr/local/bin"

    # 3. Copy binaries to the staging directory.
    info "Staging application files..."
    cp "$SOURCE_BIN_DIR/phgit" "$STAGING_DIR/usr/local/bin/phgit"
    cp "$SOURCE_BIN_DIR/installer_helper" "$STAGING_DIR/usr/local/bin/installer_helper"

    # 4. Set executable permissions.
    chmod +x "$STAGING_DIR/usr/local/bin/phgit"
    chmod +x "$STAGING_DIR/usr/local/bin/installer_helper"

    # 5. Create a post-installation script.
    # This script will run our helper AFTER the files are copied.
    local POSTINSTALL_SCRIPT="$SCRIPT_DIR/postinstall.sh"
    info "Creating post-installation script..."
    cat > "$POSTINSTALL_SCRIPT" <<EOF
#!/bin/bash
# This script is run by the installer after files are copied.

echo "Running phgit dependency check..."
# Run the helper to install Git, Terraform, Vault if needed.
/usr/local/bin/installer_helper

exit 0
EOF
    chmod +x "$POSTINSTALL_SCRIPT"

    # 6. Build the component package using pkgbuild.
    info "Building the component package..."
    local PKG_PATH="$DIST_DIR/phgit-component.pkg"
    pkgbuild --root "$STAGING_DIR" \
             --scripts "$SCRIPT_DIR" \
             --identifier "$IDENTIFIER" \
             --version "$VERSION" \
             "$PKG_PATH"

    # For a simple installer, the component package is often enough.
    # If you wanted to add a license, background image, etc., you would
    # use `productbuild` to wrap the component package.
    # For now, we will rename the component package to be the final artifact.
    local FINAL_PKG_NAME="phgit-${VERSION}.pkg"
    mv "$PKG_PATH" "$DIST_DIR/$FINAL_PKG_NAME"

    # 7. Clean up temporary scripts.
    rm "$POSTINSTALL_SCRIPT"

    success "Successfully created macOS installer at $DIST_DIR/$FINAL_PKG_NAME"
}

main