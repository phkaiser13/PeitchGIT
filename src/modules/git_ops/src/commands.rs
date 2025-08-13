/* Copyright (C) 2025 Pedro Henrique / phkaiser13
 * commands.rs - Business logic for git_ops module commands.
 *
 * This module implements the high-level logic for the commands supported by
 * the `git_ops` module. It orchestrates calls to the `git_wrapper` to perform
 * sequences of Git operations that constitute a single `gitph` command.
 *
 * For example, the `handle_send` function implements the "SND" command by
 * calling `git_add`, `git_commit`, and `git_push` in sequence, handling errors
 * at each step. This separation of concerns (business logic here, execution
 * logic in the wrapper) makes the code cleaner, easier to test, and more
 * maintainable.
 *
 * SPDX-License-Identifier: Apache-2.0 */

use crate::git_wrapper; // Import our git wrapper module
use crate::log_to_core; // Import the logging helper from lib.rs
use crate::GitphLogLevel;

type CommandResult = Result<String, String>;

pub fn handle_status() -> CommandResult {
    log_to_core(GitphLogLevel::Info, "Handling 'status' command.");

    match git_wrapper::git_status(None) {
        Ok(output) => {
            if output.is_empty() {
                println!("Working tree clean. Nothing to commit.");
            } else {
                println!("Changes to be committed or untracked files:");
                println!("{}", output);
            }
            Ok("Status command executed successfully.".to_string())
        }
        Err(e) => {
            eprintln!("Error getting Git status: {}", e);
            Err(format!("Failed to get Git status: {}", e))
        }
    }
}

pub fn handle_send() -> CommandResult {
    log_to_core(GitphLogLevel::Info, "Handling 'SND' command.");

    println!("Staging all changes...");
    if let Err(e) = git_wrapper::git_add(None, ".") {
        let err_msg = format!("Failed to stage files (git add): {}", e);
        eprintln!("{}", err_msg);
        return Err(err_msg);
    }
    println!("Files staged successfully.");

    let commit_message = "Automated commit from gitph";
    println!("Committing with message: '{}'...", commit_message);
    if let Err(e) = git_wrapper::git_commit(None, commit_message) {
        if e.contains("nothing to commit") {
            println!("Working tree clean. No new commit was created.");
            return Ok("No changes to send.".to_string());
        }
        let err_msg = format!("Failed to commit changes (git commit): {}", e);
        eprintln!("{}", err_msg);
        return Err(err_msg);
    }
    println!("Changes committed successfully.");

    // This part will only be reached if a commit was successful.
    let remote = "origin";
    let branch = "main";
    println!("Pushing to {} {}...", remote, branch);
    if let Err(e) = git_wrapper::git_push(None, remote, branch) {
        let err_msg = format!("Failed to push changes (git push): {}", e);
        eprintln!("{}", err_msg);
        return Err(err_msg);
    }
    println!("Successfully pushed to remote.");

    Ok("SND command completed successfully.".to_string())
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::fs;
    use std::process::Command;
    use tempfile::TempDir;

    /// Helper to create a temporary, fully configured Git repository for testing.
    fn setup_test_repo() -> TempDir {
        let tmp_dir = TempDir::new().expect("Failed to create temp dir");
        let repo_path = tmp_dir.path().to_str().unwrap();

        assert!(Command::new("git")
            .args(["init", repo_path])
            .status()
            .expect("Failed to init test repo")
            .success());

        // Configure user name and email to allow commits within the test repo
        assert!(Command::new("git")
            .args(["-C", repo_path, "config", "user.name", "Test User"])
            .status().unwrap().success());
        assert!(Command::new("git")
            .args(["-C", repo_path, "config", "user.email", "test@example.com"])
            .status().unwrap().success());

        tmp_dir
    }

    /// A version of the `handle_send` command specifically for testing.
    /// It runs the add and commit logic but skips the `push` to avoid network dependency.
    fn handle_send_in_dir(repo_path: &str) -> CommandResult {
        // Step 1: Add
        git_wrapper::git_add(Some(repo_path), ".")?;

        // Step 2: Commit
        let commit_result = git_wrapper::git_commit(Some(repo_path), "Automated commit from gitph");
        if let Err(e) = commit_result {
            if e.contains("nothing to commit") {
                return Ok("No changes to send.".to_string());
            }
            return Err(e); // Propagate other commit errors
        }

        // We skip the push part for this unit test.
        Ok("SND (add/commit) command completed successfully.".to_string())
    }

    #[test]
    fn test_handle_status_on_clean_repo() {
        let tmp_dir = setup_test_repo();
        let repo_path = tmp_dir.path().to_str().unwrap();

        // Act: Run the status wrapper pointing to our clean test repo.
        let output = git_wrapper::git_status(Some(repo_path)).unwrap();

        // Assert: A clean repo should have an empty porcelain status.
        assert!(output.is_empty());
    }

    #[test]
    fn test_handle_send_with_changes() {
        // Arrange: Set up a repo and create a new file with content.
        let tmp_dir = setup_test_repo();
        let repo_path = tmp_dir.path().to_str().unwrap();
        fs::write(tmp_dir.path().join("new_file.txt"), "hello").expect("Failed to write test file");

        // Act: Run our test-specific send function.
        let result = handle_send_in_dir(repo_path);

        // Assert: The command should succeed.
        assert!(result.is_ok());

        // Further assert that a commit was actually created by checking the log.
        let log_output = git_wrapper::execute_git_command(Some(repo_path), &["log", "-1", "--oneline"], None).unwrap();
        assert!(log_output.contains("Automated commit from gitph"));
    }

    #[test]
    fn test_handle_send_with_no_changes() {
        // Arrange: Set up a clean repo.
        let tmp_dir = setup_test_repo();
        let repo_path = tmp_dir.path().to_str().unwrap();

        // Act: Run the send function in the clean directory.
        let result = handle_send_in_dir(repo_path);

        // Assert: The result should be Ok and contain the specific message.
        assert!(result.is_ok());
        assert_eq!(result.unwrap(), "No changes to send.");
    }
}