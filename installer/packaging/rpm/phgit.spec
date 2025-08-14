# Copyright (C) 2025 Pedro Henrique / phkaiser13
# SPDX-License-Identifier: Apache-2.0

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
%setup -q

%build
%cmake .
%cmake_build

%install
%cmake_install

%files
%license LICENSE.txt
%doc README.md
%{_bindir}/phgit
# Add other files like man pages, completion scripts, etc.

%changelog
* Mon Jul 29 2024 Pedro Henrique / phkaiser13 <phkaiser13@users.noreply.github.com> - @PACKAGE_VERSION@-1
- Initial RPM packaging
