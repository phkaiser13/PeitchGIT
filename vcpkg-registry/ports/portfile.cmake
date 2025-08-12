# Copyright (C) 2025 Pedro Henrique / phkaiser13
# SPDX-License-Identifier: Apache-2.0
#
# This file is the vcpkg portfile for gitph. It tells vcpkg how to
# download, configure, build, and install the tool.

include(vcpkg_common_functions)

# 1. Download the source code archive from the GitHub release.
#    TODO: Update the URL and SHA512 hash for each new release.
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO phkaiser13/peitchgit
    REF "v1.0.0" # This should match the version in vcpkg.json
    SHA512 "YOUR_SHA512_HASH_HERE" # Get this hash from the GitHub Actions build output or by running `vcpkg x-add-version gitph`
    HEAD_REF main
)

# 2. Configure the project using CMake.
#    vcpkg automatically passes the correct toolchain and dependency information.
vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        # If you had CMake options to disable tests, you would add them here.
        # -DBUILD_TESTING=OFF
)

# 3. Build the project.
vcpkg_cmake_build()

# 4. Install the project.
#    This runs the `install` target from your CMakeLists.txt and places the
#    files into the correct vcpkg package directory.
vcpkg_cmake_install()

# 5. Handle tools installation.
#    vcpkg is primarily for libraries. This step is crucial to copy the
#    executable from the `bin` directory to the `tools` directory so it can
#    be found by users.
file(INSTALL
    "${CURRENT_PACKAGES_DIR}/bin/gitph.exe"
    DESTINATION "${CURRENT_PACKAGES_DIR}/tools/gitph"
)
file(REMOVE "${CURRENT_PACKAGES_DIR}/bin/gitph.exe" "${CURRENT_PACKAGES_DIR}/debug/bin/gitph.exe")

# 6. Copy license file
file(INSTALL "${SOURCE_PATH}/LICENSE" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}" RENAME copyright)
