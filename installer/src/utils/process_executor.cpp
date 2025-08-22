/* Copyright (C) 2025 Pedro Henrique / phkaiser13
* File: process_executor.cpp
* This file provides the concrete, cross-platform implementation for the ProcessExecutor
* utility. For Windows, it uses the native WinAPI (CreateProcess, CreatePipe) to gain
* full control over process creation and I/O redirection, ensuring no console windows
* appear and capturing stdout/stderr streams independently. For POSIX systems, it uses
* the standard popen() function, redirecting stderr to stdout, and then intelligently
* assigns the captured output to the correct result field based on the process exit code.
* This ensures behavioral consistency across all supported platforms.
* SPDX-License-Identifier: Apache-2.0
*/

#include "utils/process_executor.hpp"
#include "spdlog/spdlog.h"

#include <array>
#include <memory>
#include <stdexcept>
#include <vector> // Required for the Windows implementation command line buffer

#if defined(_WIN32) || defined(_WIN64)
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
#else
    #include <cstdio>
    #include <sys/wait.h>
#endif

namespace phgit_installer::utils {

#if defined(_WIN32) || defined(_WIN64)
    // Windows-specific implementation using the WinAPI for robust process control.
    ProcessResult ProcessExecutor::execute(const std::string& command) {
        ProcessResult result;
        HANDLE h_stdout_rd = NULL, h_stdout_wr = NULL;
        HANDLE h_stderr_rd = NULL, h_stderr_wr = NULL;

        SECURITY_ATTRIBUTES sa;
        sa.nLength = sizeof(SECURITY_ATTRIBUTES);
        sa.bInheritHandle = TRUE;
        sa.lpSecurityDescriptor = NULL;

        // Create anonymous pipes to capture stdout and stderr from the child process.
        if (!CreatePipe(&h_stdout_rd, &h_stdout_wr, &sa, 0) ||
            !CreatePipe(&h_stderr_rd, &h_stderr_wr, &sa, 0)) {
            result.exit_code = -1;
            result.std_err = "Failed to create pipes for process I/O redirection.";
            spdlog::error(result.std_err);
            return result;
        }

        // Ensure the read handles for the pipes are not inherited by the child process.
        SetHandleInformation(h_stdout_rd, HANDLE_FLAG_INHERIT, 0);
        SetHandleInformation(h_stderr_rd, HANDLE_FLAG_INHERIT, 0);

        PROCESS_INFORMATION pi;
        STARTUPINFOA si;
        ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));
        ZeroMemory(&si, sizeof(STARTUPINFOA));
        si.cb = sizeof(STARTUPINFOA);
        si.hStdOutput = h_stdout_wr;
        si.hStdError = h_stderr_wr;
        si.dwFlags |= STARTF_USESTDHANDLES;

        // CreateProcess requires a mutable C-style string for the command line.
        std::vector<char> cmd_vec(command.begin(), command.end());
        cmd_vec.push_back('\0');

        // Create the child process. CREATE_NO_WINDOW prevents a console from flashing.
        if (!CreateProcessA(NULL, cmd_vec.data(), NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
            result.exit_code = -1;
            result.std_err = "CreateProcess failed. Error code: " + std::to_string(GetLastError());
            spdlog::error(result.std_err);
            CloseHandle(h_stdout_rd); CloseHandle(h_stdout_wr);
            CloseHandle(h_stderr_rd); CloseHandle(h_stderr_wr);
            return result;
        }

        // The parent process must close the write ends of the pipes immediately
        // so that ReadFile will eventually return when the child closes its handles.
        CloseHandle(h_stdout_wr);
        CloseHandle(h_stderr_wr);

        // Lambda to read all available data from a pipe into a string.
        auto read_from_pipe = [](HANDLE pipe, std::string& out) {
            std::array<char, 256> buffer;
            DWORD bytes_read;
            while (ReadFile(pipe, buffer.data(), buffer.size(), &bytes_read, NULL) && bytes_read != 0) {
                out.append(buffer.data(), bytes_read);
            }
        };

        // Read stdout and stderr streams from the child process.
        read_from_pipe(h_stdout_rd, result.std_out);
        read_from_pipe(h_stderr_rd, result.std_err);

        // Wait indefinitely for the child process to terminate.
        WaitForSingleObject(pi.hProcess, INFINITE);

        // Get the process exit code.
        DWORD exit_code_dw;
        GetExitCodeProcess(pi.hProcess, &exit_code_dw);
        result.exit_code = static_cast<int>(exit_code_dw);

        // Clean up all remaining handles.
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        CloseHandle(h_stdout_rd);
        CloseHandle(h_stderr_rd);

        return result;
    }
#else // POSIX implementation (Linux, macOS, etc.)
    ProcessResult ProcessExecutor::execute(const std::string& command) {
        ProcessResult result;

        // To capture both stdout and stderr with a single popen call, we redirect
        // stderr to stdout at the shell level (2>&1).
        std::string cmd_with_redirect = command + " 2>&1";
        std::string combined_output;

        std::array<char, 128> buffer;
        // Use a unique_ptr for RAII to ensure pclose is always called.
        std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd_with_redirect.c_str(), "r"), pclose);

        if (!pipe) {
            result.exit_code = -1;
            result.std_err = "popen() failed!";
            spdlog::error(result.std_err);
            return result;
        }

        // Read the combined output stream into a single string.
        while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
            combined_output += buffer.data();
        }

        // pclose() returns the termination status of the command's shell.
        // We must release the raw pointer from the unique_ptr before calling pclose.
        int status = pclose(pipe.release());

        if (WIFEXITED(status)) {
            result.exit_code = WEXITSTATUS(status);
        } else {
            result.exit_code = -1; // Indicate abnormal termination (e.g., signal).
        }

        // --- CRITICAL LOGIC FIX ---
        // The original implementation put all output into std_out, hiding errors.
        // This corrected logic inspects the exit code to determine if the output
        // represents a success (stdout) or a failure (stderr). This makes the
        // function's behavior consistent with the Windows implementation.
        if (result.exit_code != 0) {
            result.std_err = combined_output;
        } else {
            result.std_out = combined_output;
        }

        return result;
    }
#endif

} // namespace phgit_installer::utils