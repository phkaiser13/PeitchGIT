#!/bin/bash
# Copyright (C) 2025 Pedro Henrique / phkaiser13
# File: build_deb.sh
# This script automates the creation of a Debian package (.deb) for phgit.
# It constructs the required directory layout, generates the essential DEBIAN/control
# metadata file, places the application binaries in the correct system location
# according to the Filesystem Hierarchy Standard (FHS), and then uses the
# dpkg-deb tool to build the final package.
#
# SPDX-License-Identifier: Apache-2.0

set -eu

# --- Configuration ---
# These variables can be updated by your CI/CD pipeline.
VERSION="1.0.0"
ARCH="amd64" # Corresponds to x86_64. Use "arm64" for ARM.

# --- Paths ---
# The root of the installer directory.
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# The final package will be placed in installer/linux/dist
DIST_DIR="$SCRIPT_DIR/dist"
# The staging area for building the package.
BUILD_ROOT="$SCRIPT_DIR/build/deb_pkg"
# Location of the compiled phgit binaries.
SOURCE_BIN_DIR="$SCRIPT_DIR/../../build/bin"

# --- Helper Functions ---
info() {
    printf "\033[0;34m==> %s\033[0m\n" "$1"
}
success() {
    printf "\033[0;32m==> %s\033[0m\n" "$1"
}

# --- Main Build Logic ---
main() {
    info "Starting .deb package build for phgit v$VERSION"

    # 1. Clean up previous builds and create directory structure.
    info "Setting up build environment at $BUILD_ROOT"
    rm -rf "$BUILD_ROOT"
    rm -rf "$DIST_DIR"
    mkdir -p "$DIST_DIR"
    mkdir -p "$BUILD_ROOT/DEBIAN"
    mkdir -p "$BUILD_ROOT/usr/bin"

    # 2. Create the DEBIAN/control file. This is the package's metadata.
    info "Generating DEBIAN/control file"
    cat > "$BUILD_ROOT/DEBIAN/control" <<EOF
Package: phgit
Version: ${VERSION}
Architecture: ${ARCH}
Maintainer: Pedro Henrique <phkaiser13@example.com>
Description: A Git helper tool with integrated workflows.
 phgit enhances the standard Git command line with powerful workflows,
 integrating with tools like Terraform and Vault.
Depends: git, libc6 (>= 2.35)
Homepage: https://github.com/phkaiser13/phgit
EOF
    # NOTE: The 'Depends' field is crucial. We list 'git' here, so 'apt'
    # will ensure it's installed. We don't need our helper for this anymore.

    # 3. Copy the application files into the package structure.
    # The files will be installed to /usr/bin/ on the target system.
    info "Copying application binaries to $BUILD_ROOT/usr/bin"
    cp "$SOURCE_BIN_DIR/phgit" "$BUILD_ROOT/usr/bin/phgit"
    # We don't need to package the installer_helper for the .deb package,
    # as the system's package manager handles dependencies.

    # 4. Set correct permissions for the files.
    info "Setting file permissions"
    chmod 755 "$BUILD_ROOT/usr/bin/phgit"

    # 5. Build the .deb package.
    info "Building the Debian package..."
    dpkg-deb --build "$BUILD_ROOT" "$DIST_DIR/phgit_${VERSION}_${ARCH}.deb"

    success "Successfully created package at $DIST_DIR/phgit_${VERSION}_${ARCH}.deb"
}

main