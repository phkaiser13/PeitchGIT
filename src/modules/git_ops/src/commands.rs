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