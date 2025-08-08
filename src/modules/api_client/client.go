/* Copyright (C) 2025 Pedro Henrique
 * client.go - FFI bridge and entry point for the api_client Go module.
 *
 * This file is the core of the C-Go interaction for the api_client module.
 * It uses `cgo` to implement the `gitph_core_api.h` contract, allowing this
 * Go package to be compiled as a C shared library (.so/.dll) and be loaded
 * by the main C application.
 *
 * It handles the complex tasks of data type translation between Go and C,
 * manages the lifecycle of the module, and dispatches incoming commands to
 * the appropriate Go functions that contain the actual business logic.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later */

package main

/*
// These includes are processed by cgo.
#include <stdlib.h>
#include "../../ipc/include/gitph_core_api.h"

// FIX 1: Create a C helper function to call the function pointer.
// REASON: Calling function pointers from Go directly can be syntactically messy.
// This C helper encapsulates the logic of safely calling the function pointer,
// making the Go code cleaner. It receives the function pointer (fn) and its
// arguments, and then executes the call within C. This resolves the "undefined name"
// error because we are no longer incorrectly exporting a Go function for this task.
static inline void call_log_fn_wrapper(PFN_log fn, GitphLogLevel level, const char* module, const char* msg) {
    // Check if the function pointer is not null before calling it.
    if (fn != NULL) {
        fn(level, module, msg);
    }
}
*/
import "C" // This import enables cgo.

import (
	"fmt"
	"unsafe"
)

// --- Module Info Globals ---
// We define these as globals to ensure they have a stable memory address
// that can be safely returned to C. C.CString allocates memory that C must
// manage, so we do it once and let it live for the duration of the app.
var (
	moduleName        = C.CString("api_client")
	moduleVersion     = C.CString("1.0.0")
	moduleDescription = C.CString("Client for interacting with Git provider APIs (GitHub, etc.)")
	supportedCommands = []*C.char{
		C.CString("srp"), // Set Repository
		nil,              // Null terminator for the C array
	}
	moduleInfo = C.GitphModuleInfo{
		name:        moduleName,
		version:     moduleVersion,
		description: moduleDescription,
		commands:    &supportedCommands[0],
	}
)

// coreContext holds the function pointers passed from the C core.
var coreContext *C.GitphCoreContext

// logToCore is a helper function to safely call the logger from the C core.
func logToCore(level C.GitphLogLevel, message string) {
	if coreContext == nil || coreContext.log == nil {
		// Fallback if logger is not available
		fmt.Printf("LOG CORE (fallback): %s\n", message)
		return
	}
	moduleNameC := C.CString("API_CLIENT")
	messageC := C.CString(message)
	// Defer the free to ensure it's called even if the function panics.
	defer C.free(unsafe.Pointer(moduleNameC))
	defer C.free(unsafe.Pointer(messageC))

	// FIX 2: Call the new C wrapper function.
	// REASON: We now call the simple, well-defined C function from our preamble,
	// passing the function pointer `coreContext.log` as an argument. This is the
	// correct and clean way to interact with the C function pointer.
	C.call_log_fn_wrapper(coreContext.log, level, moduleNameC, messageC)
}

// FIX 3: The incorrect Go implementation of `call_log_fn` has been removed.
// REASON: It was the source of the logical error and is no longer needed,
// as its role is now correctly handled by the C helper `call_log_fn_wrapper`.

// --- C-compatible API Implementation ---

//export module_get_info
func module_get_info() *C.GitphModuleInfo {
	return &moduleInfo
}

//export module_init
func module_init(context *C.GitphCoreContext) C.GitphStatus {
	if context == nil {
		return C.GITPH_ERROR_INIT_FAILED
	}
	coreContext = context
	logToCore(C.LOG_LEVEL_INFO, "api_client module initialized successfully.")
	return C.GITPH_SUCCESS
}

//export module_exec
func module_exec(argc C.int, argv **C.char) C.GitphStatus {
	// Convert C's argc/argv to a Go slice of strings for idiomatic handling.
	// This requires unsafe pointer arithmetic.
	argsSlice := (*[1 << 30]*C.char)(unsafe.Pointer(argv))[:argc:argc]
	args := make([]string, int(argc)) // Use int(argc) for better Go compatibility
	for i, s := range argsSlice {
		args[i] = C.GoString(s)
	}

	if len(args) == 0 {
		logToCore(C.LOG_LEVEL_ERROR, "Execution called with no arguments.")
		return C.GITPH_ERROR_INVALID_ARGS
	}

	command := args[0]
	logToCore(C.LOG_LEVEL_DEBUG, fmt.Sprintf("Executing command: %s", command))

	var err error
	switch command {
	case "srp":
		if len(args) < 2 {
			err = fmt.Errorf("command 'srp' requires at least one argument")
		} else {
			// Delegate to the handler function which contains the actual logic.
			// We will create this file and function next.
			err = handleSetRepository(args[1:])
		}
	default:
		err = fmt.Errorf("unknown command '%s' for api_client module", command)
	}

	if err != nil {
		logToCore(C.LOG_LEVEL_ERROR, fmt.Sprintf("Command failed: %v", err))
		return C.GITPH_ERROR_EXEC_FAILED
	}

	return C.GITPH_SUCCESS
}

//export module_cleanup
func module_cleanup() {
	logToCore(C.LOG_LEVEL_INFO, "api_client module cleaned up.")
	// In a real application, we might close persistent network connections here.
	// We also free the C strings we allocated for the module info.
	C.free(unsafe.Pointer(moduleName))
	C.free(unsafe.Pointer(moduleVersion))
	C.free(unsafe.Pointer(moduleDescription))
	for _, s := range supportedCommands {
		if s != nil {
			C.free(unsafe.Pointer(s))
		}
	}
}

// A main function is required for a `c-shared` library build, even if empty.
func main() {}

// Dummy handler function to make the code compilable.
func handleSetRepository(args []string) error {
	if len(args) == 0 {
		return fmt.Errorf("no repository provided")
	}
	logToCore(C.LOG_LEVEL_INFO, fmt.Sprintf("Repository set to: %s", args[0]))
	return nil
}
