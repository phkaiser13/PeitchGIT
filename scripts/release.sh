#!/bin/bash
# Copyright (C) 2025 Pedro Henrique
# release.sh - Creates a distributable release archive of gitph.
#
# This script automates the process of building a release version, packaging
# all necessary binaries, modules, and plugins into a clean structure, and
# creating a compressed tarball for distribution.
#
# SPDX-License-Identifier: GPL-3.0-or-later

set -e

# --- Variables ---
VERSION=$(grep 'project(gitph' CMakeLists.txt | grep -oP 'VERSION \K[0-9.]+')
TARGET_OS=$(uname -s | tr '[:upper:]' '[:lower:]')
TARGET_ARCH=$(uname -m)
RELEASE_NAME="gitph-v${VERSION}-${TARGET_OS}-${TARGET_ARCH}"
BUILD_DIR="build-release"
RELEASE_DIR="release/${RELEASE_NAME}"

echo "--- [GITPH] Creating release version ${VERSION} ---"

# 1. Clean previous build and create a release build
echo "--> Performing a clean release build..."
rm -rf "${BUILD_DIR}"
cmake -S . -B "${BUILD_DIR}" -DCMAKE_BUILD_TYPE=Release
cmake --build "${BUILD_DIR}" --parallel

# 2. Create the release packaging directory
echo "--> Assembling release files in '${RELEASE_DIR}'..."
rm -rf "release"
mkdir -p "${RELEASE_DIR}"

# 3. Copy all necessary artifacts
cp -r "${BUILD_DIR}/bin/"* "${RELEASE_DIR}/"
cp -r "src/plugins" "${RELEASE_DIR}/"
cp "LICENSE" "${RELEASE_DIR}/"
# cp "README.md" "${RELEASE_DIR}/" # Uncomment if you have a README

# 4. Strip binaries to reduce size (optional)
echo "--> Stripping binaries to reduce size..."
strip "${RELEASE_DIR}/gitph"
strip "${RELEASE_DIR}/modules/"*

# 5. Create the compressed archive
echo "--> Creating tarball..."
(cd release && tar -czf "${RELEASE_NAME}.tar.gz" "${RELEASE_NAME}")

# 6. Final cleanup
rm -rf "${RELEASE_DIR}"

echo ""
echo "--- [GITPH] Release complete! ---"
echo "Archive created at: release/${RELEASE_NAME}.tar.gz"