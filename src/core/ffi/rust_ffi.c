/*
 * Copyright (C) 2025 Pedro Henrique / phkaiser13
 *
 * File: src/core/ffi/rust_ffi.c
 *
 * [
 * This file implements the Foreign Function Interface (FFI) layer for Peitch.
 * Its core responsibility is to dynamically load shared libraries generated from
 * the Rust modules, resolve the exported function symbols, execute them with the
 * appropriate parameters (a JSON configuration string), and manage the lifecycle
 * of the library handles.
 *
 * Architecture:
 * The design is platform-agnostic, using preprocessor directives to select the
 * correct dynamic linking API for the target operating system.
 * - On POSIX-compliant systems (Linux, macOS), it uses the `dlopen`, `dlsym`,
 * `dlerror`, and `dlclose` functions from `<dlfcn.h>`.
 * - On Windows, it uses `LoadLibrary`, `GetProcAddress`, and `FreeLibrary` from
 * `<windows.h>`.
 *
 * A central private function, `ffi_call_rust_module`, encapsulates this platform-specific
 * logic. It takes the library name and the JSON payload as input, handles the
 * entire process of loading, symbol resolution, function execution, and error
 * reporting, and then unloads the library. This approach provides a high degree
 * of robustness and simplifies the public-facing API.
 *
 * Public functions like `ffi_call_preview_module` are thin wrappers around this
 * central function, providing a clean, command-specific entry point that the
 * `cli_parser` can use without needing to know the underlying FFI implementation
 * details. This design is highly scalable for adding new Rust modules in the future.
 *
 * Error Handling:
 * Error handling is paramount. The code meticulously checks the return values of
 * each dynamic linking function call. If a library cannot be loaded or a function
 * symbol cannot be found, a descriptive error message is printed to `stderr`, and
 * a non-zero status code is returned, ensuring that failures are immediately visible
 * and diagnosable.
 * ]
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "rust_ffi.h"
#include <stdio.h>
#include <stdlib.h>

// Platform-specific includes for dynamic library loading.
#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

// Define the name of the exported function that all Rust modules must implement.
// This standardization is the core of our FFI contract.
#define RUST_FUNCTION_NAME "invoke"
// Define the expected signature of the Rust function.
typedef int (*rust_function_t)(const char *);

/**
 * @brief A generic, internal function to call any Rust module.
 *
 * @param library_name The name of the shared library file (e.g., "libk8s_preview.so").
 * @param json_config The JSON configuration string to pass to the Rust function.
 * @return An integer status code: 0 on success, non-zero on failure.
 *
 * Pre-conditions:
 * - `library_name` and `json_config` must be valid, null-terminated strings.
 *
 * Post-conditions:
 * - The specified Rust library is dynamically loaded, the `invoke` function is called,
 * and the library is then unloaded.
 * - Errors at any stage (loading, symbol resolution) are printed to stderr.
 *
 * This function encapsulates all platform-specific FFI logic, providing a single,
 * reliable implementation for interacting with our Rust modules.
 */
static int ffi_call_rust_module(const char *library_name,
				const char *json_config)
{
#ifdef _WIN32
	// Windows-specific dynamic library loading logic.
	HINSTANCE handle = LoadLibrary(library_name);
	if (!handle)
	{
		fprintf(stderr, "FFI Error: Could not load library %s. Error code: %lu\n",
			library_name, GetLastError());
		return -1;
	}

	rust_function_t rust_function =
		(rust_function_t)GetProcAddress(handle, RUST_FUNCTION_NAME);
	if (!rust_function)
	{
		fprintf(stderr,
			"FFI Error: Could not find function '%s' in library %s. Error code: %lu\n",
			RUST_FUNCTION_NAME, library_name, GetLastError());
		FreeLibrary(handle);
		return -1;
	}
#else
	// POSIX-specific dynamic library loading logic.
	void *handle = dlopen(library_name, RTLD_LAZY);
	if (!handle)
	{
		fprintf(stderr, "FFI Error: Could not load library %s. Reason: %s\n",
			library_name, dlerror());
		return -1;
	}

	// Clear any existing error conditions before calling dlsym.
	dlerror();

	rust_function_t rust_function =
		(rust_function_t)dlsym(handle, RUST_FUNCTION_NAME);
	const char *dlsym_error = dlerror();
	if (dlsym_error)
	{
		fprintf(stderr,
			"FFI Error: Could not find function '%s' in library %s. Reason: %s\n",
			RUST_FUNCTION_NAME, library_name, dlsym_error);
		dlclose(handle);
		return -1;
	}
#endif

	// At this point, we have a valid function pointer. Execute the Rust code.
	int status = rust_function(json_config);

// Clean up: unload the library.
#ifdef _WIN32
	FreeLibrary(handle);
#else
	dlclose(handle);
#endif

	return status;
}

/*
================================================================================
 Public API Functions
================================================================================
*/

/**
 * See header file `rust_ffi.h` for function documentation.
 */
int ffi_call_preview_module(const char *json_config)
{
	// Define platform-specific library names. The build system (e.g., Cargo, CMake)
	// must produce artifacts with these names.
#ifdef _WIN32
	const char *library_name = "k8s_preview.dll";
#elif __APPLE__
	const char *library_name = "libk8s_preview.dylib";
#else
	const char *library_name = "./libk8s_preview.so"; // Assuming the library is in the current directory for Linux.
#endif

	printf("C Core: Attempting to call Rust module '%s' via FFI.\n",
	       library_name);
	return ffi_call_rust_module(library_name, json_config);
}

// Implementations for other modules like `ffi_call_release_module` would follow
// the exact same pattern, just specifying a different library name. This demonstrates
// the scalability of the FFI design.