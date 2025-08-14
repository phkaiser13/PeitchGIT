/* Copyright (C) 2025 Pedro Henrique / phkaiser13
* File: process_executor.cpp
* This file provides the concrete, cross-platform implementation for the ProcessExecutor
* utility. For Windows, it uses the native WinAPI (CreateProcess, CreatePipe) to gain
* full control over process creation and I/O redirection, ensuring no console windows
* appear. For POSIX systems, it uses the standard and reliable popen() function to
* execute commands and capture their output streams, providing a robust solution for
* interacting with system tools.
* SPDX-License-Identifier: Apache-2.0
*/

#include "utils/process_executor.hpp"
#include "spdlog/spdlog.h"

#include <array>
#include <memory>
#include <stdexcept>

#if defined(_WIN32) || defined(_WIN64)
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
#else
    #include <cstdio>
    #include <sys/wait.h>
#endif

namespace phgit_installer::utils {

#if defined(_WIN32) || defined(_WIN64)
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
            !CreatePipe(&h_stderr_rd, &h_stderr_wr, &sa, 0)) {
            result.exit_code = -1;
            result.std_err = "Failed to create pipes.";
            return result;
        }

        // Ensure the read handles are not inherited
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

        std::vector<char> cmd_vec(command.begin(), command.end());
        cmd_vec.push_back(0);

        if (!CreateProcessA(NULL, cmd_vec.data(), NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
            result.exit_code = -1;
            result.std_err = "CreateProcess failed. Error code: " + std::to_string(GetLastError());
            CloseHandle(h_stdout_rd); CloseHandle(h_stdout_wr);
            CloseHandle(h_stderr_rd); CloseHandle(h_stderr_wr);
            return result;
        }

        // Parent process must close the write ends of the pipes immediately
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

        // Wait for the process to finish
        WaitForSingleObject(pi.hProcess, INFINITE);

        DWORD exit_code_dw;
        GetExitCodeProcess(pi.hProcess, &exit_code_dw);
        result.exit_code = static_cast<int>(exit_code_dw);

        // Clean up all handles
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        CloseHandle(h_stdout_rd);
        CloseHandle(h_stderr_rd);

        return result;
    }
#else // POSIX implementation
    ProcessResult ProcessExecutor::execute(const std::string& command) {
        ProcessResult result;
        
        // We combine stdout and stderr for simplicity with popen, as it's the most
        // portable and reliable method without resorting to a full fork/exec model.
        std::string cmd_with_redirect = command + " 2>&1";
        
        std::array<char, 128> buffer;
        std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd_with_redirect.c_str(), "r"), pclose);
        
        if (!pipe) {
            result.exit_code = -1;
            result.std_err = "popen() failed!";
            return result;
        }
        
        while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
            result.std_out += buffer.data();
        }
        
        int status = pclose(pipe.release()); // Release ownership before manual pclose
        if (WIFEXITED(status)) {
            result.exit_code = WEXITSTATUS(status);
        } else {
            result.exit_code = -1; // Indicate abnormal termination
        }
        
        return result;
    }
#endif

} // namespace phgit_installer::utils
