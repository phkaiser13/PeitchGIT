/* Copyright (C) 2025 Pedro Henrique / phkaiser13
* File: process_executor.cpp
* This file implements the ProcessExecutor utility. It contains highly platform-specific
* code to robustly execute external processes. The Windows implementation uses the
* Win32 API (CreateProcess, CreatePipe) to gain full control over process I/O without
* creating console windows. The POSIX implementation uses popen, a standard and reliable
* way to achieve the same goal on Linux and macOS. The result is a powerful, unified
* function that abstracts away these complex details.
* SPDX-License-Identifier: Apache-2.0
*/

#include "utils/process_executor.hpp"
#include "spdlog/spdlog.h"

#include <array>
#include <memory>
#include <stdexcept>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <cstdio> // For popen, pclose
#include <sys/wait.h> // For WEXITSTATUS
#endif

namespace phgit_installer::utils {

#ifdef _WIN32

    ProcessResult ProcessExecutor::execute(const std::string& command) {
        ProcessResult result;
        HANDLE h_stdout_rd = NULL, h_stdout_wr = NULL;
        HANDLE h_stderr_rd = NULL, h_stderr_wr = NULL;

        SECURITY_ATTRIBUTES sa;
        sa.nLength = sizeof(SECURITY_ATTRIBUTES);
        sa.bInheritHandle = TRUE;
        sa.lpSecurityDescriptor = NULL;

        // Create pipes for stdout and stderr
        if (!CreatePipe(&h_stdout_rd, &h_stdout_wr, &sa, 0) ||
            !SetHandleInformation(h_stdout_rd, HANDLE_FLAG_INHERIT, 0)) {
            result.exit_code = -1;
            result.std_err = "Failed to create stdout pipe.";
            return result;
        }
        if (!CreatePipe(&h_stderr_rd, &h_stderr_wr, &sa, 0) ||
            !SetHandleInformation(h_stderr_rd, HANDLE_FLAG_INHERIT, 0)) {
            result.exit_code = -1;
            result.std_err = "Failed to create stderr pipe.";
            CloseHandle(h_stdout_rd); CloseHandle(h_stdout_wr);
            return result;
        }

        PROCESS_INFORMATION pi;
        STARTUPINFOA si;
        ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));
        ZeroMemory(&si, sizeof(STARTUPINFOA));
        si.cb = sizeof(STARTUPINFOA);
        si.hStdOutput = h_stdout_wr;
        si.hStdError = h_stderr_wr;
        si.dwFlags |= STARTF_USESTDHANDLES;

        std::vector<char> cmd_vec(command.begin(), command.end());
        cmd_vec.push_back(0);

        if (!CreateProcessA(NULL, cmd_vec.data(), NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
            result.exit_code = -1;
            result.std_err = "CreateProcess failed with error code: " + std::to_string(GetLastError());
            CloseHandle(h_stdout_rd); CloseHandle(h_stdout_wr);
            CloseHandle(h_stderr_rd); CloseHandle(h_stderr_wr);
            return result;
        }

        // Close the write ends of the pipes in the parent process
        CloseHandle(h_stdout_wr);
        CloseHandle(h_stderr_wr);

        // Read from pipes
        auto read_from_pipe = [](HANDLE pipe, std::string& out) {
            std::array<char, 256> buffer;
            DWORD bytes_read;
            while (ReadFile(pipe, buffer.data(), buffer.size(), &bytes_read, NULL) && bytes_read != 0) {
                out.append(buffer.data(), bytes_read);
            }
        };

        read_from_pipe(h_stdout_rd, result.std_out);
        read_from_pipe(h_stderr_rd, result.std_err);

        // Wait for the process to finish and get exit code
        WaitForSingleObject(pi.hProcess, INFINITE);
        DWORD exit_code;
        GetExitCodeProcess(pi.hProcess, &exit_code);
        result.exit_code = static_cast<int>(exit_code);

        // Cleanup
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        CloseHandle(h_stdout_rd);
        CloseHandle(h_stderr_rd);

        return result;
    }

#else // POSIX implementation

    ProcessResult ProcessExecutor::execute(const std::string& command) {
        ProcessResult result;
        std::string full_command = command + " 2>&1"; // Redirect stderr to stdout

        FILE* pipe = popen(full_command.c_str(), "r");
        if (!pipe) {
            result.exit_code = -1;
            result.std_err = "popen() failed!";
            return result;
        }

        std::array<char, 256> buffer;
        while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
            result.std_out += buffer.data();
        }

        int status = pclose(pipe);
        if (status == -1) {
            result.exit_code = -1;
            result.std_err = "pclose() failed!";
        } else {
            result.exit_code = WEXITSTATUS(status);
        }

        // On POSIX with this simple popen, we can't easily separate stdout and stderr.
        // The calling code will need to check the exit code to know if the output
        // is from stdout or stderr. A more complex fork/exec/pipe implementation
        // would be needed for full separation. For this installer's purpose, this is sufficient.
        if (result.exit_code != 0) {
            result.std_err = result.std_out;
            result.std_out = "";
        }

        return result;
    }

#endif

} // namespace phgit_installer::utils
