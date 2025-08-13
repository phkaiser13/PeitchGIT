/* Copyright (C) 2025 Pedro Henrique / phkaiser13
* File: iplatform_installer.hpp
* This header file defines the abstract base class (interface) for all platform-specific
* installers. By programming to this interface, the main application logic can remain
* completely decoupled from the concrete implementation details of how installation is
* performed on any given OS (Linux, Windows, macOS). This is a cornerstone of the
* installer's modular and extensible design.
* SPDX-License-Identifier: Apache-2.0
*/

#pragma once

namespace phgit_installer::platform {

    /**
     * @class IPlatformInstaller
     * @brief An interface representing the contract for any platform-specific installer.
     * It guarantees that every installer will have a `run_installation` method.
     */
    class IPlatformInstaller {
    public:
        // Virtual destructor is essential for proper cleanup of derived classes.
        virtual ~IPlatformInstaller() = default;

        /**
         * @brief The primary method to execute the installation process for a platform.
         * This is a pure virtual function, making this class abstract and forcing
         * derived classes to provide their own implementation.
         */
        virtual void run_installation() = 0;
    };

} // namespace phgit_installer::platform
