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
use std::path::Path; // Import the Path type

// A type alias for the result of a command handler.
type CommandResult = Result<String, String>;

/// Handles the "status" command in the given repository path.
// CHANGED: Function now accepts the repository path explicitly.
pub fn handle_status(repo_path: &Path) -> CommandResult {
    log_to_core(GitphLogLevel::Info, "Handling 'status' command.");

    match git_wrapper::git_status(repo_path) {
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

/// Handles the "SND" (Send) command in the given repository path.
// CHANGED: Function now accepts the repository path explicitly.
pub fn handle_send(repo_path: &Path) -> CommandResult {
    log_to_core(GitphLogLevel::Info, "Handling 'SND' command.");

    // --- Step 1: Add all files ---
    println!("Staging all changes...");
    // CHANGED: Pass the repo_path to the wrapper function.
    if let Err(e) = git_wrapper::git_add(repo_path, ".") {
        let err_msg = format!("Failed to stage files (git add): {}", e);
        eprintln!("{}", err_msg);
        return Err(err_msg);
    }
    println!("Files staged successfully.");

    // --- Step 2: Commit changes ---
    let commit_message = "Automated commit from gitph";
    println!("Committing with message: '{}'...", commit_message);
    // CHANGED: Pass the repo_path to the wrapper function.
    if let Err(e) = git_wrapper::git_commit(repo_path, commit_message) {
        if e.contains("nothing to commit") {
            println!("Working tree clean. No new commit was created.");
            return Ok("No changes to send.".to_string());
        }
        let err_msg = format!("Failed to commit changes (git commit): {}", e);
        eprintln!("{}", err_msg);
        return Err(err_msg);
    }
    println!("Changes committed successfully.");

    // --- Step 3: Push changes ---
    let remote = "origin";
    let branch = "main";
    println!("Pushing to {} {}...", remote, branch);
    // CHANGED: Pass the repo_path to the wrapper function.
    if let Err(e) = git_wrapper::git_push(repo_path, remote, branch) {
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

    // Helper function to set up a temporary Git repository for testing.
    // It now returns the TempDir, whose path can be used by the tests.
    fn setup_test_repo() -> TempDir {
        let tmp_dir = tempfile::TempDir::new().expect("Failed to create temp dir");
        let repo_path_str = tmp_dir.path().to_str().unwrap();

        // Initialize a new git repository
        assert!(Command::new("git")
            .args(["init", repo_path_str])
            .status()
            .expect("Failed to init test repo")
            .success());

        // Configure user name and email to allow commits
        // Note: We use `-C <path>` to run the git command in the correct directory
        // without changing the process's CWD.
        assert!(Command::new("git")
            .args(["-C", repo_path_str, "config", "user.name", "Test User"])
            .status()
            .unwrap()
            .success());
        assert!(Command::new("git")
            .args(["-C", repo_path_str, "config", "user.email", "test@example.com"])
            .status()
            .unwrap()
            .success());

        // REMOVED: The dangerous call to `std::env::set_current_dir` is gone.

        tmp_dir
    }

    #[test]
    fn test_handle_status_on_clean_repo() {
        // Arrange: Create a clean git repository.
        let tmp_dir = setup_test_repo();
        let repo_path = tmp_dir.path();

        // Act: Run the status handler, passing the explicit path.
        let result = handle_status(repo_path);

        // Assert
        assert!(result.is_ok());
    }

    #[test]
    fn test_handle_send_with_changes() {
        // Arrange
        let tmp_dir = setup_test_repo();
        let repo_path = tmp_dir.path();
        // Create the file inside the temporary repository.
        fs::write(repo_path.join("new_file.txt"), "hello").expect("Failed to write test file");

        // Act: Run the send handler, passing the explicit path.
        // NOTE: For a real push to work, we'd need to set up a remote.
        // For this test, we'll assume the push will fail and we're just testing add/commit.
        // A more robust test would mock the push or create a local bare repo as a remote.
        let result = handle_send(repo_path);

        // Assert: For now, we expect it to fail on `git push` because no remote is configured.
        // The important part is that it gets past the `git add` and `git commit` steps.
        assert!(result.is_err());
        assert!(result.unwrap_err().contains("Failed to push changes"));

        // Further assert that a commit was actually created.
        let log_output =
            git_wrapper::execute_git_command(repo_path, &["log", "-1", "--oneline"], None).unwrap();
        assert!(log_output.contains("Automated commit from gitph"));
    }



    #[test]
    fn test_handle_send_with_no_changes() {
        // Arrange
        let tmp_dir = setup_test_repo();
        let repo_path = tmp_dir.path();

        // Act
        let result = handle_send(repo_path);

        // Assert
        assert!(result.is_ok());
        assert_eq!(result.unwrap(), "No changes to send.");
    }
}