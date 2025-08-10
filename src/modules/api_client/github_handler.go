/* Copyright (C) 2025 Pedro Henrique / phkaiser13
 * github_handler.go - Business logic for GitHub API interactions.
 *
 * This file defines a generic ApiProvider interface and its concrete
 * implementation for the GitHub API. This architecture decouples the FFI
 * bridge from the specifics of any single provider, making the module
 * extensible to support others like GitLab or Bitbucket in the future.
 *
 * It leverages Go's standard library for HTTP requests and JSON parsing.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later */

package main

import (
	"encoding/json"
	"fmt"
	"net/http"
	"strings"
)

// --- Generic API Provider Contracts ---

// RepoInfo is a generic, provider-agnostic representation of a repository.
type RepoInfo struct {
	FullName      string
	Description   string
	Stars         int
	Forks         int
	URL           string
	DefaultBranch string
}

// ApiProvider defines the contract for any Git API service.
type ApiProvider interface {
	FetchRepoInfo(user, repo string) (*RepoInfo, error)
}

// --- GitHub API Implementation ---

// GitHubRepoResponse matches the structure of the JSON from the GitHub API.
// The `json:"..."` tags are essential for the decoder.
type GitHubRepoResponse struct {
	FullName      string  `json:"full_name"`
	Description   *string `json:"description"` // Pointer to handle null descriptions
	Stars         int     `json:"stargazers_count"`
	Forks         int     `json:"forks_count"`
	URL           string  `json:"html_url"`
	DefaultBranch string  `json:"default_branch"`
}

const gitHubAPIEndpoint = "https://api.github.com/repos/%s/%s"

// GitHubHandler is the concrete implementation for the ApiProvider interface.
type GitHubHandler struct {
	// In a real-world scenario, this might hold an auth token or a custom
	// http.Client.
}

// NewGitHubHandler creates a new instance of the GitHubHandler.
func NewGitHubHandler() *GitHubHandler {
	return &GitHubHandler{}
}

// FetchRepoInfo performs the API call to GitHub and maps the response to our
// generic RepoInfo struct.
func (h *GitHubHandler) FetchRepoInfo(user, repo string) (*RepoInfo, error) {
	url := fmt.Sprintf(gitHubAPIEndpoint, user, repo)
	// --- CORREÇÃO: Usando a constante Go em vez do tipo C ---
	logToCore(logLevelDebug, fmt.Sprintf("Querying GitHub API: %s", url))

	resp, err := http.Get(url)
	if err != nil {
		return nil, fmt.Errorf("failed to make request to GitHub API: %w", err)
	}
	defer resp.Body.Close()

	if resp.StatusCode != http.StatusOK {
		return nil, fmt.Errorf("repository not found or API error (status: %s)", resp.Status)
	}

	var apiResponse GitHubRepoResponse
	if err := json.NewDecoder(resp.Body).Decode(&apiResponse); err != nil {
		return nil, fmt.Errorf("failed to decode JSON response from GitHub: %w", err)
	}

	// Map the provider-specific response to our generic struct.
	repoInfo := &RepoInfo{
		FullName:      apiResponse.FullName,
		Stars:         apiResponse.Stars,
		Forks:         apiResponse.Forks,
		URL:           apiResponse.URL,
		DefaultBranch: apiResponse.DefaultBranch,
	}
	if apiResponse.Description != nil {
		repoInfo.Description = *apiResponse.Description
	} else {
		repoInfo.Description = "(No description provided)"
	}

	return repoInfo, nil
}

// --- Command Logic ---

// setRepository is the main logic function for the "srp" command. It is
// now provider-agnostic.
func setRepository(provider ApiProvider, args []string) error {
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

	// 3. Call the API logic via the interface
	repoInfo, err := provider.FetchRepoInfo(user, repo)
	if err != nil {
		return err // Propagate the error up
	}

	// 4. Display the results to the user in a clean format.
	fmt.Println("--- Repository Found ---")
	fmt.Printf("Name:          %s\n", repoInfo.FullName)
	fmt.Printf("Description:   %s\n", repoInfo.Description)
	fmt.Printf("Default Branch: %s\n", repoInfo.DefaultBranch)
	fmt.Printf("Stars:         %d\n", repoInfo.Stars)
	fmt.Printf("Forks:         %d\n", repoInfo.Forks)
	fmt.Printf("URL:           %s\n", repoInfo.URL)
	fmt.Println("------------------------")

	// --- CORREÇÃO: Usando a constante Go em vez do tipo C ---
	logToCore(logLevelInfo, fmt.Sprintf("Successfully fetched info for %s", repoInfo.FullName))
	return nil
}
