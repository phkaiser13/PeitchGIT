# Copyright (C) 2025 Pedro Henrique
# Makefile - A user-friendly convenience wrapper for CMake.
#
# This Makefile does not contain any build logic itself. It serves as a facade,
# providing simple, memorable commands that translate into the more verbose
# commands required to operate CMake correctly.
#
# This improves the developer experience, especially on POSIX systems.
#
# SPDX-License-Identifier: GPL-3.0-or-later

# --- Variables ---
CMAKE_BUILD_DIR := build
EXECUTABLE_NAME := gitph
EXECUTABLE_PATH := ${CMAKE_BUILD_DIR}/bin/${EXECUTABLE_NAME}

# Phony targets do not represent files.
.PHONY: all build configure clean run install help

# --- Targets ---

# Default target
all: build

# Configure and build the entire project.
build: ${EXECUTABLE_PATH}

# The main build dependency chain.
${EXECUTABLE_PATH}: ${CMAKE_BUILD_DIR}/Makefile
	@echo "--- Building Project ---"
	@${CMAKE_COMMAND} --build ${CMAKE_BUILD_DIR}

# Run CMake to configure the project and generate the native build system.
${CMAKE_BUILD_DIR}/Makefile: CMakeLists.txt
	@echo "--- Configuring Project with CMake ---"
	@${CMAKE_COMMAND} -S . -B ${CMAKE_BUILD_DIR}

# A separate configure target for explicit configuration.
configure: ${CMAKE_BUILD_DIR}/Makefile

# Clean up all build artifacts.
clean:
	@echo "--- Cleaning Build Artifacts ---"
	@rm -rf ${CMAKE_BUILD_DIR}

# Build and run the application.
run: build
	@echo "--- Running gitph ---"
	@${EXECUTABLE_PATH}

# Install the application using the rules defined in CMake.
install: build
	@echo "--- Installing gitph ---"
	@${CMAKE_COMMAND} --install ${CMAKE_BUILD_DIR}

# Display help information.
help:
	@echo "gitph Makefile Wrapper"
	@echo "----------------------"
	@echo "Usage: make [target]"
	@echo ""
	@echo "Targets:"
	@echo "  all        (Default) Configure and build the entire project."
	@echo "  build      Build the project if already configured."
	@echo "  configure  Run CMake to generate the build system."
	@echo "  run        Build and execute the main application."
	@echo "  install    Install the application to the configured directory."
	@echo "  clean      Remove all build artifacts."
	@echo "  help       Show this help message."