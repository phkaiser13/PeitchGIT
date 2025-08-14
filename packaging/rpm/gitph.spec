# Copyright (C) 2025 Pedro Henrique / phkaiser13
# SPDX-License-Identifier: Apache-2.0

Name:       gitph
# O workflow irá substituir a versão
Version:    ${GITPH_VERSION}
Release:    1%{?dist}
Summary:    The Polyglot Assistant for Git & DevOps Workflows

License:    Apache-2.0
URL:        https://github.com/phkaiser13/peitchgit
Source0:    https://github.com/phkaiser13/peitchgit/archive/refs/tags/v%{version}.tar.gz

# Dependências de build
BuildRequires:  gcc-c++
BuildRequires:  cmake
BuildRequires:  pkgconfig(lua)
BuildRequires:  pkgconfig(libcurl)
BuildRequires:  nlohmann-json-devel
BuildRequires:  rust
BuildRequires:  cargo

# Dependências de execução
Requires:       libcurl
Requires:       lua

%description
A modern, extensible, and high-performance command-line tool designed to
unify and streamline your development lifecycle.

%prep
%autosetup -n peitchgit-%{version}

%build
%cmake -DCMAKE_BUILD_TYPE=Release
%cmake_build

%install
%cmake_install

%files
%license LICENSE
%{_bindir}/gitph
%dir %{_datadir}/gitph
%{_datadir}/gitph/modules/
%{_datadir}/gitph/plugins/

%changelog
# O workflow pode adicionar entradas aqui
* Mon Aug 12 2025 Pedro Henrique <seu-email@example.com> - ${GITPH_VERSION}-1
- Release inicial do pacote RPM.
