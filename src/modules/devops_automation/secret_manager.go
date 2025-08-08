/* Copyright (C) 2025 Pedro Henrique
 * secret_manager.go - Logic for interacting with secret management tools.
 *
 * This file provides functionality for reading secrets from a backend like
 * HashiCorp Vault. It demonstrates a different, more advanced pattern of
 * process execution compared to the terraform_runner.
 *
 * Instead of streaming output in real-time, it executes a command, captures
 * its structured output (JSON), and then parses that output to extract
 * specific pieces of data. This "execute-capture-parse" pattern is essential
 * for intelligent interaction with other CLI tools.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later */

package main

/*
#include "../../ipc/include/gitph_core_api.h"
*/

import (
	"C"
	"bytes"
	"encoding/json"
	"fmt"
	"os/exec"
)

// VaultSecretResponse defines the structure of the JSON output from the
// `vault read -format=json` command. We only care about the `data` field,
// which itself contains a map of the actual secret data.
type VaultSecretResponse struct {
	Data struct {
		Data map[string]interface{} `json:"data"`
	} `json:"data"`
}

// runCommandAndCapture is a generic helper function to execute a command and
// capture its stdout. This is a more advanced version of the runner used for
// Terraform, designed for when we need to process the output.
func runCommandAndCapture(commandName string, args ...string) (string, error) {
	// This function would live alongside `runTerraformCommand` and be called
	// by our handlers.
	logToCore(C.LOG_LEVEL_DEBUG, fmt.Sprintf("Capturing output from: %s %v", commandName, args))

	if _, err := exec.LookPath(commandName); err != nil {
		return "", fmt.Errorf("%s executable not found in PATH", commandName)
	}

	cmd := exec.Command(commandName, args...)

	// Use buffers to capture stdout and stderr from the child process.
	var stdout, stderr bytes.Buffer
	cmd.Stdout = &stdout
	cmd.Stderr = &stderr

	err := cmd.Run()
	if err != nil {
		// If the command fails, the stderr buffer likely contains the reason.
		return "", fmt.Errorf("command failed: %s", stderr.String())
	}

	return stdout.String(), nil
}

// handleVaultRead orchestrates the process of reading a secret from Vault.
func handleVaultRead(args []string) error {
	if len(args) != 1 {
		return fmt.Errorf("expected exactly one argument: <secret_path>")
	}
	secretPath := args[0]

	fmt.Printf("--- Reading secret from Vault: %s ---\n", secretPath)

	// 1. Execute the command and capture its JSON output.
	// We force JSON output for reliable parsing.
	jsonOutput, err := runCommandAndCapture("vault", "read", "-format=json", secretPath)
	if err != nil {
		return err // Propagate the error from the command execution.
	}

	// 2. Parse the captured JSON string.
	var secretResponse VaultSecretResponse
	if err := json.Unmarshal([]byte(jsonOutput), &secretResponse); err != nil {
		return fmt.Errorf("failed to parse JSON from vault: %w", err)
	}

	// 3. Display the results in a user-friendly format.
	secrets := secretResponse.Data.Data
	if len(secrets) == 0 {
		fmt.Println("No data found at the specified secret path.")
	} else {
		fmt.Println("Secrets found:")
		for key, value := range secrets {
			// Use a type assertion to handle different value types from JSON.
			if valStr, ok := value.(string); ok {
				fmt.Printf("  - %s: %s\n", key, valStr)
			} else {
				// For complex values (like nested objects), just indicate the type.
				fmt.Printf("  - %s: [complex value of type %T]\n", key, value)
			}
		}
	}
	fmt.Println("---------------------------------")

	return nil
}
