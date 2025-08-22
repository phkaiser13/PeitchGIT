# Copyright (C) 2025 Pedro Henrique / phkaiser13
# SPDX-License-Identifier: Apache-2.0
#
# Portfile for phgit (template).
# NOTE: Replace placeholders @PHGIT_VERSION@ and @PHGIT_SHA512@ using your release workflow.

include(vcpkg_common_functions)

# 1. Download the source from GitHub.
#    The workflow must replace REF and SHA512 with concrete values.
vcpkg_from_github(
        OUT_SOURCE_PATH SOURCE_PATH
        REPO phkaiser13/peitchgit
        # The release tag. Example: "v1.2.3"
        REF "v@PHGIT_VERSION@"
        # SHA512 of the release archive tarball (computed by the workflow)
        SHA512 "@PHGIT_SHA512@"
        HEAD_REF main
)

# 2. Configure the project using CMake.
vcpkg_cmake_configure(
        SOURCE_PATH "${SOURCE_PATH}"
        OPTIONS
        # Disable tests; not needed for the shipped package.
        -DBUILD_TESTING=OFF
)

# 3. Build the project.
vcpkg_cmake_build()

# 4. Install the built artifacts.
vcpkg_cmake_install()

# 5. Move the executable into the tools folder for the port.
file(INSTALL
        "${CURRENT_PACKAGES_DIR}/bin/phgit${VCPKG_HOST_EXECUTABLE_SUFFIX}"
        DESTINATION "${CURRENT_PACKAGES_DIR}/tools/phgit"
)
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/bin" "${CURRENT_PACKAGES_DIR}/debug")

# 6. Install license/copyright.
file(INSTALL "${SOURCE_PATH}/LICENSE" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}" RENAME copyright)
