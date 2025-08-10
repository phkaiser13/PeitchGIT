/* Copyright (C) 2025 Pedro Henrique / phkaiser13
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
 * SPDX-License-Identifier: Apache-2.0 */

package main

/*
#include <stdlib.h>
#include "../../ipc/include/gitph_core_api.h"

// C-wrapper to safely call the function pointer from Go.
static inline void call_log_fn_wrapper(void (*fn)(GitphLogLevel, const char*, const char*), GitphLogLevel level, const char* module, const char* msg) {
    if (fn != NULL) {
        fn(level, module, msg);
    }
}
*/
import "C"
import (
	"fmt"
	"unsafe"
)

// --- ALTERAÇÃO 1: Constantes Go para os níveis de log ---
// Mapeamos os enums C para constantes Go. Isso permite que outros arquivos .go
// no mesmo pacote usem esses valores sem precisar importar "C" diretamente.
const (
	logLevelDebug = C.LOG_LEVEL_DEBUG
	logLevelInfo  = C.LOG_LEVEL_INFO
	logLevelWarn  = C.LOG_LEVEL_WARN
	logLevelError = C.LOG_LEVEL_ERROR
	logLevelFatal = C.LOG_LEVEL_FATAL
)

// --- Module Info Globals ---
// These are defined as globals to ensure they have a stable memory address
// that can be safely returned to C. C.CString allocates memory that C must
// manage, so we do it once and let it live for the duration of the app.
var (
	moduleName        = C.CString("api_client")
	moduleVersion     = C.CString("1.1.0") // Version bumped for refactor
	moduleDescription = C.CString("Client for interacting with Git provider APIs (e.g., GitHub)")
	supportedCommands = []*C.char{
		C.CString("srp"), // Set RePository
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

// --- ALTERAÇÃO 2: A assinatura de logToCore agora usa `int` nativo do Go ---
// Isso abstrai o tipo C, tornando a função segura para ser chamada de qualquer lugar no pacote Go.
func logToCore(level int, message string) {
	if coreContext == nil || coreContext.log == nil {
		fmt.Printf("LOG CORE (fallback): %s\n", message)
		return
	}
	moduleNameC := C.CString("API_CLIENT")
	messageC := C.CString(message)
	defer C.free(unsafe.Pointer(moduleNameC))
	defer C.free(unsafe.Pointer(messageC))

	// A conversão para o tipo C acontece aqui, de forma segura, dentro deste arquivo.
	C.call_log_fn_wrapper(coreContext.log, C.GitphLogLevel(level), moduleNameC, messageC)
}

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
	// --- ALTERAÇÃO 3: Usar a nova constante Go ---
	logToCore(logLevelInfo, "api_client module initialized successfully.")
	return C.GITPH_SUCCESS
}

//export module_exec
func module_exec(argc C.int, argv **C.char) C.GitphStatus {
	// Convert C's argc/argv to a Go slice of strings for idiomatic handling.
	args := cArgsToSlice(argc, argv)
	if len(args) == 0 {
		// --- ALTERAÇÃO 4: Usar a nova constante Go ---
		logToCore(logLevelError, "Execution called with no command.")
		return C.GITPH_ERROR_INVALID_ARGS
	}

	command := args[0]
	commandArgs := args[1:]
	// --- ALTERAÇÃO 5: Usar a nova constante Go ---
	logToCore(logLevelDebug, fmt.Sprintf("Dispatching command: %s", command))

	var err error
	switch command {
	case "srp":
		// For now, we hardcode the GitHub provider.
		// In the future, this could be determined by a config file.
		var provider ApiProvider = NewGitHubHandler()
		err = setRepository(provider, commandArgs)
	default:
		err = fmt.Errorf("unknown command '%s' for api_client module", command)
	}

	if err != nil {
		// --- ALTERAÇÃO 6: Usar a nova constante Go ---
		logToCore(logLevelError, fmt.Sprintf("Command failed: %v", err))
		return C.GITPH_ERROR_EXEC_FAILED
	}

	return C.GITPH_SUCCESS
}

//export module_cleanup
func module_cleanup() {
	// --- ALTERAÇÃO 7: Usar a nova constante Go ---
	logToCore(logLevelInfo, "api_client module cleaned up.")
	// Free the C strings we allocated for the module info.
	C.free(unsafe.Pointer(moduleName))
	C.free(unsafe.Pointer(moduleVersion))
	C.free(unsafe.Pointer(moduleDescription))
	for _, s := range supportedCommands {
		if s != nil {
			C.free(unsafe.Pointer(s))
		}
	}
}

// cArgsToSlice converts C-style argc/argv to a Go slice of strings.
func cArgsToSlice(argc C.int, argv **C.char) []string {
	// This unsafe conversion creates a Go slice header that points to the C array.
	argsSlice := (*[1 << 30]*C.char)(unsafe.Pointer(argv))[:argc:argc]
	args := make([]string, int(argc))
	for i, s := range argsSlice {
		args[i] = C.GoString(s)
	}
	return args
}

// A main function is required for a `c-shared` library build, even if empty.
func main() {}
