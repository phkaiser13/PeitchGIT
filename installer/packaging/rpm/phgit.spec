/* Copyright (C) 2025 Pedro Henrique / phkaiser13
* File: phgit.spec
* This is an RPM spec file used to build the 'phgit' package for RPM-based
* Linux distributions like Fedora, CentOS, and RHEL. It defines metadata,
* build dependencies, and the steps required to compile and install the software.
* It also includes a '%postun' scriptlet, which is a post-uninstallation script
* that ensures any system-wide state or cache files created by the application
* are cleaned up upon the package's final removal (not during an upgrade).
* SPDX-License-Identifier: Apache-2.0 */

Name:       phgit
Version:    @PACKAGE_VERSION@
Release:    1%{?dist}
Summary:    A Git helper tool with integrated workflows for Terraform and Vault

License:    Apache-2.0
URL:        @PACKAGE_HOMEPAGE@
# Source0 will be the tarball of the source code created by 'make dist' or similar
Source0:    %{name}-%{version}.tar.gz

BuildRequires:  gcc-c++
BuildRequires:  cmake
BuildRequires:  libcurl-devel
BuildRequires:  spdlog-devel
BuildRequires:  nlohmann-json-devel
Requires:       git >= 2.20.0

%description
@PACKAGE_DESCRIPTION@

%prep
# The %setup macro unpacks the source tarball and changes into its directory.
%setup -q

%build
# Standard CMake build process.
%cmake .
%cmake_build

%install
# Installs the built files into the RPM build root.
%cmake_install

%postun
# This is a post-uninstallation scriptlet. It runs after the package's files
# have been removed from the system.
# The argument $1 indicates the number of packages of this name that will be
# left on the system after the transaction.
#
# $1 == 0: This is the final uninstallation (a 'dnf remove').
#          This is when we should perform the cleanup.
# $1 >= 1: This is an upgrade. The old package is being uninstalled, but a
#          new one is being installed. We must NOT delete state files here.
if [ $1 -eq 0 ] ; then
    # This block only runs on a complete removal, not an upgrade.
    APP_STATE_DIR="/var/lib/phgit"
    
    # Check if the state directory exists before trying to remove it.
    if [ -d "$APP_STATE_DIR" ]; then
        echo "Purging phgit system-wide state files from ${APP_STATE_DIR}..."
        # Recursively and forcefully remove the directory.
        rm -rf "$APP_STATE_DIR"
    fi
fi

%files
# This section lists all the files that are part of the package.
%license LICENSE.txt
%doc README.md
%{_bindir}/phgit
# Add other files like man pages, completion scripts, etc.

%changelog
* Mon Jul 29 2024 Pedro Henrique / phkaiser13 <phkaiser13@users.noreply.github.com> - @PACKAGE_VERSION@-1
- Add %postun scriptlet for cleanup on final removal.
- Initial RPM packaging