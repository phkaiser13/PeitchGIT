/* Copyright (C) 2025 Pedro Henrique
 * parser.go - YAML workflow parser for the ci_cd_manager module.
 *
 * This Go program is designed to be compiled as a C shared library. Its sole
 * purpose is to parse a given YAML workflow file (e.g., a GitHub Actions
 * .yml file) and serialize the structured data into a JSON string.
 *
 * This JSON string is then returned to the C/C++ caller, acting as a stable,
 * language-agnostic data interchange format. This decouples the Go parser
 * from the C++ visualizer, requiring them only to agree on the JSON schema.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later */

package main

/*
#include <stdlib.h>
*/
import "C"

import (
	"encoding/json"
	"fmt"
	"io/ioutil"

	"gopkg.in/yaml.v3"
)

// --- Data Structures for YAML and JSON ---
// These structs define the schema for both parsing the YAML and generating
// the JSON. They represent a simplified view of a GitHub Actions workflow.

type WorkflowStep struct {
	Name string `yaml:"name" json:"name"`
	Run  string `yaml:"run"  json:"run"`
}

type WorkflowJob struct {
	Name   string         `yaml:"-"    json:"name"` // Name is the key in the YAML map
	RunsOn string         `yaml:"runs-on" json:"runs_on"`
	Steps  []WorkflowStep `yaml:"steps"   json:"steps"`
}

type Workflow struct {
	Name string                 `yaml:"name" json:"name"`
	Jobs map[string]WorkflowJob `yaml:"jobs" json:"jobs"`
}

// ParseWorkflowToJSON reads the YAML file at the given path, parses it,
// and returns a JSON string representing the workflow.
//
// CONTRACT: The returned `*C.char` is allocated by Go and MUST be freed
// by the C/C++ caller using `free()`. A NULL return value indicates an error.
//
//export ParseWorkflowToJSON
func ParseWorkflowToJSON(filepath *C.char) *C.char {
	// Convert C string to Go string
	goFilepath := C.GoString(filepath)

	// Read the YAML file content
	yamlFile, err := ioutil.ReadFile(goFilepath)
	if err != nil {
		// In a real app, we'd log this error back to the core.
		fmt.Printf("Error reading file: %v\n", err)
		return nil
	}

	// Unmarshal the YAML into our Go structs
	var workflow Workflow
	err = yaml.Unmarshal(yamlFile, &workflow)
	if err != nil {
		fmt.Printf("Error parsing YAML: %v\n", err)
		return nil
	}

	// The job name is the key in the map, so we need to manually assign it.
	for name, job := range workflow.Jobs {
		job.Name = name
		workflow.Jobs[name] = job
	}

	// Marshal the Go struct into a JSON byte slice
	jsonBytes, err := json.MarshalIndent(workflow, "", "  ") // Pretty-print the JSON
	if err != nil {
		fmt.Printf("Error marshalling to JSON: %v\n", err)
		return nil
	}

	// Convert the Go byte slice to a C string and return it.
	// The caller is now responsible for this memory.
	return C.CString(string(jsonBytes))
}

// A main function is required for a `c-shared` library build.
func main() {}
