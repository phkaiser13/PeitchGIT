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
 * here. This makes it easy to update command arguments or change execution
 * logic in one place.
 * - Error Handling: Functions return a `Result<String, String>`, which is an
 * idiomatic Rust way to handle operations that can fail. An `Ok(String)`
 * contains the successful output (stdout), while an `Err(String)` contains
 * the error message (stderr).
 * - Reusability: Provides simple, reusable functions like `git_status()` or
 * `git_add()` that can be composed into more complex workflows in the
 * `commands.rs` module.
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
/// * `repo_path` - An optional path to the repository directory.
/// * `args` - A slice of strings representing the arguments to pass to `git`.
/// * `stdin_data` - Optional data to be piped to the command's standard input.
///
/// # Returns
/// A `GitResult` containing either the standard output or standard error.
pub fn execute_git_command(repo_path: Option<&str>, args: &[&str], stdin_data: Option<&str>) -> GitResult {
    let mut command_log = "Executing: git".to_string();
    let mut command = Command::new("git");

    if let Some(path) = repo_path {
        command.arg("-C").arg(path);
        command_log.push_str(&format!(" -C {}", path));
    }
    command_log.push_str(&format!(" {}", args.join(" ")));

    super::log_to_core(super::GitphLogLevel::Debug, &command_log);
    command.args(args);

    if stdin_data.is_some() {
        command.stdin(Stdio::piped());
    }

    command.stdout(Stdio::piped());
    command.stderr(Stdio::piped());

    let mut child = match command.spawn() {
        Ok(child) => child,
        Err(e) => {
            let error_msg = format!("Failed to spawn git command. Is Git installed and in your PATH? Error: {}", e);
            super::log_to_core(super::GitphLogLevel::Fatal, &error_msg);
            return Err(error_msg);
        }
    };

    if let Some(data) = stdin_data {
        if let Some(mut stdin) = child.stdin.take() {
            if let Err(e) = stdin.write_all(data.as_bytes()) {
                let error_msg = format!("Failed to write to git stdin: {}", e);
                super::log_to_core(super::GitphLogLevel::Error, &error_msg);
                return Err(error_msg);
            }
        }
    }

    let output = match child.wait_with_output() {
        Ok(output) => output,
        Err(e) => {
            let error_msg = format!("Failed to wait for git command to finish: {}", e);
            super::log_to_core(super::GitphLogLevel::Error, &error_msg);
            return Err(error_msg);
        }
    };

    if output.status.success() {
        Ok(String::from_utf8_lossy(&output.stdout).to_string())
    } else {
        Err(String::from_utf8_lossy(&output.stderr).to_string())
    }
}

// --- Public Wrapper Functions ---

pub fn git_status(repo_path: Option<&str>) -> GitResult {
    execute_git_command(repo_path, &["status", "--porcelain"], None)
}

pub fn git_add(repo_path: Option<&str>, pathspec: &str) -> GitResult {
    execute_git_command(repo_path, &["add", pathspec], None)
}

pub fn git_commit(repo_path: Option<&str>, message: &str) -> GitResult {
    execute_git_command(repo_path, &["commit", "-m", message], None)
}

pub fn git_push(repo_path: Option<&str>, remote: &str, branch: &str) -> GitResult {
    execute_git_command(repo_path, &["push", remote, branch], None)
}