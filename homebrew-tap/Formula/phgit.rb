# Copyright (C) 2025 Pedro Henrique / phkaiser13
# SPDX-License-Identifier: Apache-2.0

# Homebrew formula for phgit.
#   - @PHGIT_VERSION@ with the release tag (without leading 'v' if you prefer)
#   - @PHGIT_SHA256@ with the sha256 of the release tarball
#
# Keep comments in English and the code minimal/clear so the template is easy to update.

class Phgit < Formula
  desc "The Polyglot Assistant for Git & DevOps Workflows"
  homepage "https://github.com/phkaiser13/peitchgit"

  # here we have placeholders must be replaced by the release workflow:
  url "https://github.com/phkaiser13/peitchgit/archive/refs/tags/v@PHGIT_VERSION@.tar.gz"
  sha256 "@PHGIT_SHA256@"

  license "Apache-2.0"

  # Build-time dependencies
  depends_on "cmake" => :build
  depends_on "pkg-config" => :build
  depends_on "rust" => :build

  # Runtime dependencies
  depends_on "curl"
  depends_on "lua"
  depends_on "nlohmann-json"

  def install
    # 1) Configure with CMake (out-of-source build)
    system "cmake", "-S", ".", "-B", "build", *std_cmake_args

    # 2) Build in parallel
    system "cmake", "--build", "build", "--parallel"

    # 3) Install artifacts into Homebrew prefix
    system "cmake", "--install", "build"
  end

  test do
    # Basic smoke test: verify the binary responds to --version.
    # Keep the assertion generic to avoid brittle version string comparisons.
    assert_match "phgit version", shell_output("#{bin}/phgit --version")
  end
end
