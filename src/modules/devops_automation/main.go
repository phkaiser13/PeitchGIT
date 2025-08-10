/* Copyright (C) 2025 Pedro Henrique / phkaiser13
 * main.go - Unified FFI bridge and dispatcher for the devops_automation module.
 *
 * This file is the single entry point for the C core to interact with all
 * functionalities of the devops_automation module. It has been refactored to
 * handle commands for multiple tools (Terraform, Vault, etc.).
 *
 * It implements the gitph FFI contract and contains a central dispatcher that
 * routes commands to the appropriate handler function based on the command name.
 * It also provides a toolbox of helper functions for different process
 * execution strategies (streaming vs. capturing).
 *
 * SPDX-License-Identifier: Apache-2.0 */

package main

/*
#include <stdlib.h>
#include "../../ipc/include/gitph_core_api.h"
*/
import "C"

import (
	"bytes"
	"encoding/json"
	"fmt"
	"os"
	"os/exec"
	"unsafe"
)

// --- FFI Boilerplate & Module Info ---

var (
	moduleName        = C.CString("devops_automation")
	moduleVersion     = C.CString("1.0.1") // Version bumped to reflect changes
	moduleDescription = C.CString("Wrapper for DevOps tools like Terraform and Vault.")
	// The list of commands is now updated to be comprehensive.
	supportedCommands = []*C.char{
		C.CString("tf-plan"),
		C.CString("tf-apply"),
		C.CString("vault-read"),
		nil, // Null terminator
	}
	moduleInfo = C.GitphModuleInfo{
		name:        moduleName,
		version:     moduleVersion,
		description: moduleDescription,
		commands:    &supportedCommands[0],
	}
)

var coreContext *C.GitphCoreContext

func logToCore(level C.GitphLogLevel, message string) {
	fmt.Printf("[%s] %s\n", "DEVOPS_AUTOMATION", message)
}

//export module_get_info
func module_get_info() *C.GitphModuleInfo { return &moduleInfo }

//export module_init
func module_init(context *C.GitphCoreContext) C.GitphStatus {
	coreContext = context
	logToCore(C.LOG_LEVEL_INFO, "devops_automation module initialized.")
	return C.GITPH_SUCCESS
}

//export module_cleanup
func module_cleanup() {
	logToCore(C.LOG_LEVEL_INFO, "devops_automation module cleaned up.")
	C.free(unsafe.Pointer(moduleName))
	C.free(unsafe.Pointer(moduleVersion))
	C.free(unsafe.Pointer(moduleDescription))
	for _, s := range supportedCommands {
		if s != nil {
			C.free(unsafe.Pointer(s))
		}
	}
}

// --- Process Execution Toolbox ---

// runCommandWithStreaming executes a command and streams its I/O in real-time.
// Ideal for long-running, interactive commands.
func runCommandWithStreaming(commandName string, args ...string) error {
	logToCore(C.LOG_LEVEL_DEBUG, fmt.Sprintf("Streaming: %s %v", commandName, args))
	if _, err := exec.LookPath(commandName); err != nil {
		return fmt.Errorf("%s executable not found in PATH", commandName)
	}
	cmd := exec.Command(commandName, args...)
	cmd.Stdout = os.Stdout
	cmd.Stderr = os.Stderr
	cmd.Stdin = os.Stdin
	if err := cmd.Run(); err != nil {
		return fmt.Errorf("%s command failed", commandName)
	}
	return nil
}

// runCommandAndCapture executes a command and captures its stdout for parsing.
// Ideal for commands that return structured data.
func runCommandAndCapture(commandName string, args ...string) (string, error) {
	logToCore(C.LOG_LEVEL_DEBUG, fmt.Sprintf("Capturing: %s %v", commandName, args))
	if _, err := exec.LookPath(commandName); err != nil {
		return "", fmt.Errorf("%s executable not found in PATH", commandName)
	}
	cmd := exec.Command(commandName, args...)
	var stdout, stderr bytes.Buffer
	cmd.Stdout = &stdout
	cmd.Stderr = &stderr
	if err := cmd.Run(); err != nil {
		return "", fmt.Errorf("command failed: %s", stderr.String())
	}
	return stdout.String(), nil
}

// --- Command Handlers ---

// Terraform Handlers
func handleTerraformPlan(args []string) error {
	fmt.Println("--- Running Terraform Plan ---")
	return runCommandWithStreaming("terraform", append([]string{"plan"}, args...)...)
}

func handleTerraformApply(args []string) error {
	fmt.Println("--- Running Terraform Apply ---")
	return runCommandWithStreaming("terraform", append([]string{"apply"}, args...)...)
}

// Vault Handlers
type VaultSecretResponse struct {
	Data struct {
		Data map[string]interface{} `json:"data"`
	} `json:"data"`
}

func handleVaultRead(args []string) error {
	if len(args) != 1 {
		return fmt.Errorf("expected exactly one argument: <secret_path>")
	}
	secretPath := args[0]
	fmt.Printf("--- Reading secret from Vault: %s ---\n", secretPath)
	jsonOutput, err := runCommandAndCapture("vault", "read", "-format=json", secretPath)
	if err != nil {
		return err
	}
	var secretResponse VaultSecretResponse
	if err := json.Unmarshal([]byte(jsonOutput), &secretResponse); err != nil {
		return fmt.Errorf("failed to parse JSON from vault: %w", err)
	}
	secrets := secretResponse.Data.Data
	if len(secrets) == 0 {
		fmt.Println("No data found at the specified secret path.")
	} else {
		fmt.Println("Secrets found:")
		for key, value := range secrets {
			fmt.Printf("  - %s: %v\n", key, value)
		}
	}
	fmt.Println("---------------------------------")
	return nil
}

// --- Main Dispatcher ---

//export module_exec
func module_exec(argc C.int, argv **C.char) C.GitphStatus {
	argsSlice := (*[1 << 30]*C.char)(unsafe.Pointer(argv))[:argc:argc]
	args := make([]string, argc)
	for i, s := range argsSlice {
		args[i] = C.GoString(s)
	}
	if len(args) == 0 {
		return C.GITPH_ERROR_INVALID_ARGS
	}

	command := args[0]
	userArgs := args[1:]
	var err error

	switch command {
	case "tf-plan":
		err = handleTerraformPlan(userArgs)
	case "tf-apply":
		err = handleTerraformApply(userArgs)
	case "vault-read":
		err = handleVaultRead(userArgs)
	default:
		err = fmt.Errorf("unknown command '%s' for devops_automation module", command)
	}

	if err != nil {
		logToCore(C.LOG_LEVEL_ERROR, fmt.Sprintf("Command failed: %v", err))
		fmt.Fprintf(os.Stderr, "\n[gitph ERROR] %v\n", err)
		return C.GITPH_ERROR_EXEC_FAILED
	}
	return C.GITPH_SUCCESS
}

func main() {}