/* Copyright (C) 2025 Pedro Henrique / phkaiser13
* File: process_executor.hpp
* This header defines the interface for the ProcessExecutor utility. This is a crucial
* component for a system installer, providing a robust, cross-platform way to execute
* external commands and capture their complete output. It abstracts the platform-specific
* complexities of process creation (CreateProcess on Windows, popen/fork on POSIX) and
* provides a simple, unified interface to run a command, wait for its completion, and
* retrieve its stdout, stderr, and exit code.
* SPDX-License-Identifier: Apache-2.0
*/

#pragma once

#include <string>
#include <vector>

namespace phgit_installer::utils {

    /**
     * @struct ProcessResult
     * @brief A data structure to hold all results from an executed process.
     */
    struct ProcessResult {
        std::string std_out;    // The standard output captured from the process.
        std::string std_err;    // The standard error captured from the process.
        int exit_code = -1;     // The exit code of the process.
    };

    /**
     * @class ProcessExecutor
     * @brief Provides a static method to execute external commands synchronously.
     */
    class ProcessExecutor {
    public:
        /**
         * @brief Executes an external command and waits for it to complete.
         *
         * This is a blocking call. It will not return until the child process has terminated.
         * It is designed to be simple and robust for the common installer use case of running
         * a command and checking its result.
         *
         * @param command The full command line to execute. For commands with spaces in the
         *                path, it's recommended to wrap the executable path in double quotes.
         * @return A ProcessResult struct containing the captured output and exit code.
         */
        static ProcessResult execute(const std::string& command);
    };

} // namespace phgit_installer::utils
