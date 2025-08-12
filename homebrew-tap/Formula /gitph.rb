# Copyright (C) 2025 Pedro Henrique / phkaiser13
# SPDX-License-Identifier: Apache-2.0
#
# This is the Homebrew formula for the gitph command-line tool.
# Formulas are Ruby scripts that tell Homebrew how to install software.
# For more information, see: https://docs.brew.sh/Formula-Cookbook

class Gitph < Formula
  # -- METADATA --
  desc "The Polyglot Assistant for Git & DevOps Workflows"
  homepage "https://github.com/phkaiser13/peitchgit"
  
  # URL to the source code archive for a specific release.
  # Homebrew will automatically download and verify this.
  # TODO: Update the URL and sha256 hash for every new release.
  # To get the sha256, run: curl -L <URL> | shasum -a 256
  url "https://github.com/phkaiser13/peitchgit/archive/refs/tags/v1.0.0.tar.gz"
  sha256 "YOUR_SHA256_HASH_HERE" 
  
  license "Apache-2.0"

  # -- DEPENDENCIES --
  # These are the build-time dependencies required by CMake.
  # Homebrew will ensure these are installed before trying to build gitph.
  depends_on "cmake" => :build
  depends_on "pkg-config" => :build
  depends_on "curl"
  depends_on "lua"
  depends_on "nlohmann-json"

  def install
    # This block contains the build instructions. Homebrew creates a temporary,
    # sandboxed environment to run these commands.

    # 1. Standard CMake configuration step.
    #    `std_cmake_args` provides standard arguments like `CMAKE_INSTALL_PREFIX`.
    system "cmake", "-S", ".", "-B", "build", *std_cmake_args

    # 2. Build the project.
    system "cmake", "--build", "build"

    # 3. Install the built artifacts.
    #    This runs the `install` target defined in your CMakeLists.txt.
    #    It will copy the executable to `#{prefix}/bin` and modules to `#{prefix}/bin/modules`.
    system "cmake", "--install", "build"
  end

  test do
    # This block defines a simple test to run after installation to verify
    # that the executable is working correctly.
    # We just check if the `--version` command runs without error.
    assert_match "gitph version", shell_output("#{bin}/gitph --version")
  end
end
