// /* Copyright (C) 2025 Pedro Henrique
//         * go.mod - Go module definition for the ci_cd_manager module.
//         *
//         * This file initializes a new, independent Go module dedicated to parsing
//         * CI/CD workflow files. It is distinct from the `api_client` module,
//         * demonstrating how our project can contain multiple, isolated Go components,
//         * each compiled into its own shared library.
//         *
//         * This module will contain the logic for reading and understanding YAML files
//         * from providers like GitHub Actions.
//         *
//         * SPDX-License-Identifier: GPL-3.0-or-later */

module gitph/modules/ci_cd_manager

go 1.18

require gopkg.in/yaml.v3 v3.0.1
