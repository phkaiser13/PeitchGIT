#!/bin/bash
# Copyright (C) 2025 Pedro Henrique / phkaiser13
# File: build_rpm.sh
# This script automates the creation of an RPM package (.rpm) for phgit,
# suitable for Fedora, RHEL, CentOS, and other RPM-based distributions.
# It generates a .spec file on the fly, sets up the required rpmbuild
# directory structure, and then invokes rpmbuild to create the final package.
#
# SPDX-License-Identifier: Apache-2.0

set -eu

# --- Configuration ---
VERSION="1.0.0"
RELEASE="1" # RPM-specific release number, reset to 1 for each new version.
ARCH="x86_64" # RPM architecture name for amd64.

# --- Paths ---
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DIST_DIR="$SCRIPT_DIR/dist"
# rpmbuild requires a specific directory tree. We'll create it temporarily.
RPM_BUILD_DIR="$SCRIPT_DIR/build/rpm"
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
    info "Starting .rpm package build for phgit v${VERSION}-${RELEASE}"

    # 1. Clean up and create the rpmbuild directory structure.
    info "Setting up rpmbuild environment at $RPM_BUILD_DIR"
    rm -rf "$RPM_BUILD_DIR"
    rm -rf "$DIST_DIR"
    mkdir -p "$DIST_DIR"
    for dir in BUILD RPMS SOURCES SPECS SRPMS; do
        mkdir -p "$RPM_BUILD_DIR/$dir"
    done

    # 2. Copy source binary to the SOURCES directory.
    info "Staging source binary"
    cp "$SOURCE_BIN_DIR/phgit" "$RPM_BUILD_DIR/SOURCES/phgit"

    # 3. Generate the .spec file. This is the heart of the RPM build.
    info "Generating phgit.spec file"
    cat > "$RPM_BUILD_DIR/SPECS/phgit.spec" <<EOF
# Spec file for phgit
Name:       phgit
Version:    ${VERSION}
Release:    ${RELEASE}%{?dist}
Summary:    A Git helper tool with integrated workflows.
License:    Apache-2.0
URL:        https://github.com/phkaiser13/phgit
Source0:    phgit

# This tells RPM that our source is a pre-compiled binary, not source code.
BuildArch: ${ARCH}

# Here we declare dependencies. 'Requires' is the RPM equivalent of 'Depends'.
# The system package manager (dnf, yum) will handle installing git.
Requires:   git

%description
phgit enhances the standard Git command line with powerful workflows,
integrating with tools like Terraform and Vault.

%install
# This section describes how to install the files into the build root.
# The build root is a temporary staging area.
mkdir -p %{buildroot}/usr/bin
install -m 755 %{SOURCE0} %{buildroot}/usr/bin/phgit

%files
# This section lists every file that will be included in the final package.
# We also set default file attributes (permissions, user, group).
%defattr(-,root,root,-)
/usr/bin/phgit

%changelog
# This is a log of changes, usually maintained manually or by CI.
* $(date +"%a %b %d %Y") Pedro Henrique <phkaiser13@example.com> - ${VERSION}-${RELEASE}
- Initial package release.
EOF

    # 4. Build the RPM package using rpmbuild.
    info "Building the RPM package..."
    # We use --define "_topdir ..." to tell rpmbuild to use our temporary
    # directory instead of the system-wide default (~/rpmbuild).
    rpmbuild -bb \
        --define "_topdir $RPM_BUILD_DIR" \
        "$RPM_BUILD_DIR/SPECS/phgit.spec"

    # 5. Move the final package to the distribution directory.
    local final_rpm_path
    final_rpm_path=$(find "$RPM_BUILD_DIR/RPMS/" -name "*.rpm" | head -n 1)
    info "Moving final package to $DIST_DIR"
    mv "$final_rpm_path" "$DIST_DIR/"

    # 6. Clean up the temporary build directory.
    info "Cleaning up build environment"
    rm -rf "$RPM_BUILD_DIR"

    success "Successfully created RPM package in $DIST_DIR/"
}

main