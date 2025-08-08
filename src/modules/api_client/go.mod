// /* Copyright (C) 2025 Pedro Henrique
//         * go.mod - Go module definition for the api_client module.
//         *
//         * This file initializes a Go module. It serves two primary purposes:
//         * 1. It defines the module's path, which acts as a unique identifier for
//         *    the package and prevents import path conflicts. Here, we use a path
//         *    that mirrors our project structure.
//         * 2. It tracks the project's dependencies. As we add third-party packages
//         *    (e.g., for more advanced HTTP clients or authentication), the `go`
//         *    toolchain will automatically add them here.
//         *
//         * This is the foundational file for any Go project and is the first step
//         * in building our network client module.
//         *
//         * SPDX-License-Identifier: GPL-3.0-or-later */

        module gitph/modules/api_client

        go 1.18