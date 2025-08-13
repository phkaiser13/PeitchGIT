#!/bin/bash
# integration_test.sh - End-to-end test script for gitph.

set -e # Exit immediately if a command fails.
set -x # Print each command before executing it.

# --- Setup ---
BUILD_DIR="build"
GITPH_BIN="${BUILD_DIR}/bin/gitph"
TEST_REPO_DIR="test_repo"

# Ensure the project is built.
echo "--- Building project ---"
cmake -S . -B "${BUILD_DIR}"
cmake --build "${BUILD_DIR}" --parallel

# Clean up any previous test repository.
rm -rf "${TEST_REPO_DIR}"
mkdir "${TEST_REPO_DIR}"
cd "${TEST_REPO_DIR}"

# --- Test Case 1: 'status' command on a clean repo ---
echo "--- Testing 'status' on a clean repo ---"
git init
# Capture output and check for the expected message.
output=$("../${GITPH_BIN}" status)
if [[ ! "$output" == *"Working tree clean"* ]]; then
    echo "FAIL: 'status' command did not report a clean working tree."
    exit 1
fi
echo "PASS: 'status' command works correctly on a clean repo."

# --- Test Case 2: 'SND' command workflow ---
echo "--- Testing 'SND' command ---"
echo "test content" > new_file.txt

# Run the SND command.
"../${GITPH_BIN}" SND

# Verify that the commit was created.
git log --oneline | grep "Automated commit from gitph"
echo "PASS: 'SND' command created a commit."

# --- Cleanup ---
cd ..
rm -rf "${TEST_REPO_DIR}"

echo "--- All integration tests passed! ---"
