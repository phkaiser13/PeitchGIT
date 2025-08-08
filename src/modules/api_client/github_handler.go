/* Copyright (C) 2025 Pedro Henrique
 * github_handler.go - Business logic for GitHub API interactions.
 *
 * This file contains the concrete implementation for fetching data from the
 * public GitHub API. It is kept separate from the FFI bridge (`client.go`)
 * to maintain a clean separation of concerns.
 *
 * It leverages Go's powerful standard library for making HTTP requests and
 * for parsing JSON responses into strongly-typed structs, which makes the
 * code robust and easy to understand.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later */

package main

import (
	"encoding/json"
	"fmt"
	"net/http"
	"strings"
)

// GitHubRepo defines the structure of the data we want to extract from the
// GitHub API's JSON response. The `json:"..."` tags are essential for the
// json decoder to map the incoming fields to our struct fields.
type GitHubRepo struct {
	FullName        string  `json:"full_name"`
	Description     *string `json:"description"` // Pointer to handle null descriptions
	Stars           int     `json:"stargazers_count"`
	Forks           int     `json:"forks_count"`
	URL             string  `json:"html_url"`
	DefaultBranch   string  `json:"default_branch"`
}

const gitHubAPIEndpoint = "https://api.github.com/repos/%s/%s"

// fetchGitHubRepoInfo performs the actual API call to GitHub.
func fetchGitHubRepoInfo(user, repo string) (*GitHubRepo, error) {
	// Construct the full API URL.
	url := fmt.Sprintf(gitHubAPIEndpoint, user, repo)
	logToCore(C.LOG_LEVEL_DEBUG, fmt.Sprintf("Querying GitHub API: %s", url))

	// Perform the HTTP GET request.
	resp, err := http.Get(url)
	if err != nil {
		return nil, fmt.Errorf("failed to make request to GitHub API: %w", err)
	}
	// `defer` is a powerful Go feature that guarantees this line will be
	// executed before the function returns, ensuring we don't leak resources.
	defer resp.Body.Close()

	// Check for non-successful status codes (e.g., 404 Not Found).
	if resp.StatusCode != http.StatusOK {
		return nil, fmt.Errorf("repository not found or API error (status: %s)", resp.Status)
	}

	// Decode the JSON response directly into our struct.
	var repoInfo GitHubRepo
	if err := json.NewDecoder(resp.Body).Decode(&repoInfo); err != nil {
		return nil, fmt.Errorf("failed to decode JSON response from GitHub: %w", err)
	}

	return &repoInfo, nil
}

// handleSetRepository is the main logic function for the "srp" command.
func handleSetRepository(args []string) error {
	// 1. Validate input
	if len(args) != 1 {
		return fmt.Errorf("expected exactly one argument: <user/repo>")
	}
	repoPath := args[0]

	// 2. Parse the input string
	parts := strings.SplitN(repoPath, "/", 2)
	if len(parts) != 2 || parts[0] == "" || parts[1] == "" {
		return fmt.Errorf("invalid format. Please use <user/repo>")
	}
	user, repo := parts[0], parts[1]

	// 3. Call the API logic
	repoInfo, err := fetchGitHubRepoInfo(user, repo)
	if err != nil {
		return err // Propagate the error up
	}

	// 4. Display the results to the user in a clean format.
	// This output is what the user sees in their terminal.
	fmt.Println("--- Repository Found on GitHub ---")
	fmt.Printf("Name:          %s\n", repoInfo.FullName)
	if repoInfo.Description != nil {
		fmt.Printf("Description:   %s\n", *repoInfo.Description)
	} else {
		fmt.Printf("Description:   (No description provided)\n")
	}
	fmt.Printf("Default Branch: %s\n", repoInfo.DefaultBranch)
	fmt.Printf("Stars:         %d\n", repoInfo.Stars)
	fmt.Printf("Forks:         %d\n", repoInfo.Forks)
	fmt.Printf("URL:           %s\n", repoInfo.URL)
	fmt.Println("----------------------------------")

	// In a real application, we would now store this information in a
	// global state or configuration file for other commands to use.
	logToCore(C.LOG_LEVEL_INFO, fmt.Sprintf("Successfully fetched info for %s", repoInfo.FullName))

	return nil
}