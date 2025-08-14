# Copyright (C) 2025 Pedro Henrique / phkaiser13
# SPDX-License-Identifier: Apache-2.0

class Gitph < Formula
  desc "The Polyglot Assistant for Git & DevOps Workflows"
  homepage "https://github.com/phkaiser13/peitchgit"
  
  # Estas variáveis serão atualizadas pelo workflow de release
  url "https://github.com/phkaiser13/peitchgit/archive/refs/tags/v${GITPH_VERSION}.tar.gz"
  sha256 "${GITPH_SHA256}" 
  
  license "Apache-2.0"

  # Dependências de build
  depends_on "cmake" => :build
  depends_on "pkg-config" => :build
  depends_on "rust" => :build
  
  # Dependências de execução
  depends_on "curl"
  depends_on "lua"
  depends_on "nlohmann-json"

  def install
    # 1. Configuração padrão do CMake
    system "cmake", "-S", ".", "-B", "build", *std_cmake_args

    # 2. Compilação do projeto
    system "cmake", "--build", "build", "--parallel"

    # 3. Instalação dos artefatos
    system "cmake", "--install", "build"
  end

  test do
    # Verifica se o comando --version funciona
    assert_match "gitph version", shell_output("#{bin}/gitph --version")
  end
end
