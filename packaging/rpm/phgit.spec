# Copyright (C) 2025 Pedro Henrique / phkaiser13
# SPDX-License-Identifier: Apache-2.0
#
# RPM spec template for phgit.
# Workflow must replace @PHGIT_VERSION@ and/or @PHGIT_TARBALL_URL@ and can optionally fill @PHGIT_RELEASE_DATE@.
# Keep comments in English and prefer using RPM macros (e.g. %{version}) inside the spec body.

Name:       phgit
# Workflow should replace this placeholder with the release version (e.g. 1.2.3)
Version:    @PHGIT_VERSION@
Release:    1%{?dist}
Summary:    The Polyglot Assistant for Git & DevOps Workflows

License:    Apache-2.0
URL:        https://github.com/phkaiser13/peitchgit

# Source tarball URL. The workflow can either set this directly, or use the default pattern below.
# Default pattern (workflow can generate): https://github.com/phkaiser13/peitchgit/archive/refs/tags/v@PHGIT_VERSION@.tar.gz
Source0:    @PHGIT_TARBALL_URL@

# Build-time dependencies
BuildRequires:  gcc-c++
BuildRequires:  cmake
BuildRequires:  pkgconfig(lua)
BuildRequires:  pkgconfig(libcurl)
BuildRequires:  nlohmann-json-devel
BuildRequires:  rust
BuildRequires:  cargo

# Runtime dependencies
Requires:       libcurl
Requires:       lua

%description
A modern, extensible, and high-performance command-line tool designed to
unify and streamline your development lifecycle.

%prep
# autosetup will unpack Source0 and chdir into the unpacked dir.
# We assume the tarball extracts to peitchgit-<version>
%autosetup -n peitchgit-%{version}

%build
# Use the bundled CMake macros provided by rpmbuild
%cmake -DCMAKE_BUILD_TYPE=Release
%cmake_build

%install
%cmake_install

%files
%license LICENSE
%{_bindir}/phgit
%dir %{_datadir}/phgit
%{_datadir}/phgit/modules/
%{_datadir}/phgit/plugins/

%changelog
# The workflow may add a changelog entry here. Example placeholder below:
* @PHGIT_RELEASE_DATE@ Pedro Henrique <you@example.com> - @PHGIT_VERSION@-1
- Initial RPM package.
