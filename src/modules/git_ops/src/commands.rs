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

// A type alias for the result of a command handler.
// On success, returns an Ok with a success message. On failure, an Err with an error message.
type CommandResult = Result<String, String>;

/// Handles the "status" command.
///
/// This is a simple command that directly maps to a single git_wrapper function.
/// It fetches the status and prints it to the console.
pub fn handle_status() -> CommandResult {
    log_to_core(GitphLogLevel::Info, "Handling 'status' command.");

    match git_wrapper::git_status() {
        Ok(output) => {
            if output.is_empty() {
                println!("Working tree clean. Nothing to commit.");
            } else {
                // The output of `git status --porcelain` is designed for scripts.
                // We can simply print it for a clear, concise status view.
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

/// Handles the "SND" (Send) command.
///
/// This is a more complex command that orchestrates multiple Git operations:
/// 1. Add all changes.
/// 2. Commit with a predefined message.
/// 3. Push to the default remote and branch (e.g., origin main).
///
/// NOTE: This is a simplified version. A real-world implementation would need
/// to prompt the user for a commit message and intelligently determine the
/// remote and branch.
pub fn handle_send() -> CommandResult {
    log_to_core(GitphLogLevel::Info, "Handling 'SND' command.");

    // --- Step 1: Add all files ---
    println!("Staging all changes...");
    if let Err(e) = git_wrapper::git_add(".") {
        let err_msg = format!("Failed to stage files (git add): {}", e);
        eprintln!("{}", err_msg);
        return Err(err_msg);
    }
    println!("Files staged successfully.");

    // --- Step 2: Commit changes ---
    // TODO: Prompt user for a real commit message.
    let commit_message = "Automated commit from gitph";
    println!("Committing with message: '{}'...", commit_message);
    if let Err(e) = git_wrapper::git_commit(commit_message) {
        // A common case for commit failure is having nothing to commit.
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
    // TODO: Intelligently determine the remote and branch.
    let remote = "origin";
    let branch = "main"; // This is a common default, but `master` is also used.
    println!("Pushing to {} {}...", remote, branch);
    if let Err(e) = git_wrapper::git_push(remote, branch) {
        let err_msg = format!("Failed to push changes (git push): {}", e);
        eprintln!("{}", err_msg);
        return Err(err_msg);
    }
    println!("Successfully pushed to remote.");

    Ok("SND command completed successfully.".to_string())
}

// ADDDING: FOR TESTING

#[cfg(test)]
mod tests {
    use super::*; // Import functions from the outer module.
    use std::fs;
    use std::process::Command;

    // Helper function to set up a temporary Git repository for testing.
    fn setup_test_repo() -> tempfile::TempDir {
        let tmp_dir = tempfile::TempDir::new().expect("Failed to create temp dir");
        let repo_path = tmp_dir.path().to_str().unwrap();

        // Initialize a new git repository
        assert!(Command::new("git")
            .args(["init", repo_path])
            .status()
            .expect("Failed to init test repo")
            .success());

        // Configure user name and email to allow commits
        assert!(Command::new("git")
            .args(["-C", repo_path, "config", "user.name", "Test User"])
            .status().unwrap().success());
        assert!(Command::new("git")
            .args(["-C", repo_path, "config", "user.email", "test@example.com"])
            .status().unwrap().success());


        // Change the current directory to the new repo for the duration of the test.
        // This makes running git commands simpler.
        assert!(std::env::set_current_dir(repo_path).is_ok());

        tmp_dir
    }

    #[test]
    fn test_handle_status_on_clean_repo() {
        // Arrange: Create a clean git repository.
        let _tmp_dir = setup_test_repo(); // The directory is cleaned up when _tmp_dir goes out of scope.

        // Act: Run the status handler.
        let result = handle_status();

        // Assert: Check that the operation was successful.
        // In a real test, we would capture stdout to check the "Working tree clean" message.
        assert!(result.is_ok());
    }

    #[test]
    fn test_handle_send_with_changes() {
        // Arrange: Set up a repo and create a new file.
        let _tmp_dir = setup_test_repo();
        fs::write("new_file.txt", "hello").expect("Failed to write test file");

        // Act: Run the send handler.
        let result = handle_send();

        // Assert: The command should succeed.
        assert!(result.is_ok());

        // Further assert that a commit was actually created.
        let log_output = git_wrapper::execute_git_command(&["log", "-1", "--oneline"], None).unwrap();
        assert!(log_output.contains("Automated commit from gitph"));
    }

    #[test]
    fn test_handle_send_with_no_changes() {
        // Arrange: Set up a clean repo.
        let _tmp_dir = setup_test_repo();

        // Act
        let result = handle_send();

        // Assert
        assert!(result.is_ok());
        assert_eq!(result.unwrap(), "No changes to send.");
    }
}
