# Copyright (C) 2025 Pedro Henrique / phkaiser13
# SPDX-License-Identifier: Apache-2.0
#
# This is the RPM spec file for gitph. It tells tools like `rpmbuild`
# how to build an RPM package from the source code.

# --- METADATA ---
# TODO: Update the Version for every new release.
Name:       gitph
Version:    1.0.0
Release:    1%{?dist}
Summary:    The Polyglot Assistant for Git & DevOps Workflows

License:    Apache-2.0
URL:        https://github.com/phkaiser13/peitchgit
# URL to the source code tarball from the GitHub release.
Source0:    https://github.com/phkaiser13/peitchgit/archive/refs/tags/v%{version}.tar.gz

# --- DEPENDENCIES ---
# BuildRequires lists dependencies needed to *build* the package.
BuildRequires:  gcc-c++
BuildRequires:  cmake
BuildRequires:  pkgconfig(lua)
BuildRequires:  pkgconfig(libcurl)
BuildRequires:  nlohmann-json-devel
BuildRequires:  golang
BuildRequires:  rust
BuildRequires:  cargo

# Requires lists dependencies needed to *run* the package.
Requires:       git
Requires:       libcurl
Requires:       lua

# --- DESCRIPTION ---
%description
A modern, extensible, and high-performance command-line tool designed to
unify and streamline your development lifecycle.

# --- BUILD STEPS ---
# The %prep section prepares the source code (e.g., unpacking the tarball).
%prep
%autosetup -n peitchgit-%{version}

# The %build section compiles the code.
%build
%cmake -DCMAKE_BUILD_TYPE=Release
%cmake_build

# The %install section copies the built files into a temporary install root.
%install
%cmake_install

# --- FILE LIST ---
# The %files section lists all the files that are part of this package.
%files
%license LICENSE
%{_bindir}/gitph
%dir %{_datadir}/gitph
%{_datadir}/gitph/modules/
%{_datadir}/gitph/plugins/

# --- CHANGELOG ---
%changelog
# TODO: Add new entries here for each release.
* Mon Aug 12 2025 Pedro Henrique <your-email> - 1.0.0-1
- Initial RPM package release.
