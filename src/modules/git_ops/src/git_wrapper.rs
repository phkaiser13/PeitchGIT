/* Copyright (C) 2025 Pedro Henrique / phkaiser13
 * git_wrapper.rs - A safe wrapper for executing Git commands.
 *
 * This module provides a clean and safe interface for interacting with the
 * system's `git` command-line tool. It abstracts the complexities of the
 * `std::process::Command` API, providing dedicated functions for common Git
 * operations.
 *
 * Key design principles:
 * - Centralization: All direct interactions with the `git` executable happen
 *   here. This makes it easy to update command arguments or change execution
 *   logic in one place.
 * - Error Handling: Functions return a `Result<String, String>`, which is an
 *   idiomatic Rust way to handle operations that can fail. An `Ok(String)`
 *   contains the successful output (stdout), while an `Err(String)` contains
 *   the error message (stderr).
 * - Reusability: Provides simple, reusable functions like `git_status()` or
 *   `git_add()` that can be composed into more complex workflows in the
 *   `commands.rs` module.
 *
 * SPDX-License-Identifier: Apache-2.0 */

use std::process::{Command, Stdio};
use std::io::{Write};

/// A type alias for the result of a Git command execution.
/// On success, returns stdout. On failure, returns stderr.
pub type GitResult = Result<String, String>;

/// Executes a raw `git` command with the given arguments.
/// This is the core function that all other wrapper functions will use.
///
/// # Arguments
/// * `args` - A slice of strings representing the arguments to pass to `git`.
///            For example: `&["status", "--porcelain"]`.
/// * `stdin_data` - Optional data to be piped to the command's standard input.
///
/// # Returns
/// A `GitResult` containing either the standard output or standard error.


///FIX: execute_git_command must be a public function for rust tests.
pub fn execute_git_command(args: &[&str], stdin_data: Option<&str>) -> GitResult {
    // Log the command being executed for debugging purposes.
    super::log_to_core(super::GitphLogLevel::Debug, &format!("Executing: git {}", args.join(" ")));

    let mut command = Command::new("git");
    command.args(args);

    // If there is data for stdin, configure the pipe.
    if stdin_data.is_some() {
        command.stdin(Stdio::piped());
    }

    // Configure stdout and stderr to be captured.
    command.stdout(Stdio::piped());
    command.stderr(Stdio::piped());

    // Spawn the child process.
    let mut child = match command.spawn() {
        Ok(child) => child,
        Err(e) => {
            let error_msg = format!("Failed to spawn git command. Is Git installed and in your PATH? Error: {}", e);
            super::log_to_core(super::GitphLogLevel::Fatal, &error_msg);
            return Err(error_msg);
        }
    };

    // Write to stdin if data was provided.
    if let Some(data) = stdin_data {
        if let Some(mut stdin) = child.stdin.take() {
            if let Err(e) = stdin.write_all(data.as_bytes()) {
                let error_msg = format!("Failed to write to git stdin: {}", e);
                super::log_to_core(super::GitphLogLevel::Error, &error_msg);
                return Err(error_msg);
            }
        }
    }

    // Wait for the command to finish and capture its output.
    let output = match child.wait_with_output() {
        Ok(output) => output,
        Err(e) => {
            let error_msg = format!("Failed to wait for git command to finish: {}", e);
            super::log_to_core(super::GitphLogLevel::Error, &error_msg);
            return Err(error_msg);
        }
    };

    // Check the exit status of the command.
    if output.status.success() {
        // Command was successful, return stdout.
        Ok(String::from_utf8_lossy(&output.stdout).to_string())
    } else {
        // Command failed, return stderr.
        Err(String::from_utf8_lossy(&output.stderr).to_string())
    }
}

// --- Public Wrapper Functions ---

/// Runs `git status --porcelain` for a machine-readable status.
pub fn git_status() -> GitResult {
    execute_git_command(&["status", "--porcelain"], None)
}

/// Runs `git add <pathspec>`.
///
/// # Arguments
/// * `pathspec` - The path to add (e.g., "." for all files).
pub fn git_add(pathspec: &str) -> GitResult {
    execute_git_command(&["add", pathspec], None)
}

/// Runs `git commit -m "<message>"`.
///
/// # Arguments
/// * `message` - The commit message.
pub fn git_commit(message: &str) -> GitResult {
    execute_git_command(&["commit", "-m", message], None)
}

/// Runs `git push <remote> <branch>`.
///
/// # Arguments
/// * `remote` - The name of the remote repository (e.g., "origin").
/// * `branch` - The name of the branch to push.
pub fn git_push(remote: &str, branch: &str) -> GitResult {
    execute_git_command(&["push", remote, branch], None)
}