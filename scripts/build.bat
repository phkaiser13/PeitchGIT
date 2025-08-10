@echo off
REM Copyright (C) 2025 Pedro Henrique / phkaiser13
REM build.bat - Build script for gitph on Windows.
REM
REM This script orchestrates the build process by calling CMake to configure
REM the project and then to compile all C/C++/Go/Rust components.
REM
REM SPDX-License-Identifier: GPL-3.0-or-later

REM --- Variables ---
SET BUILD_DIR=build

REM --- Main Logic ---
echo --- [GITPH] Configuring project with CMake ---
REM Generate the build system in the build/ directory.
cmake -S . -B "%BUILD_DIR%"
IF %ERRORLEVEL% NEQ 0 (
    echo CMake configuration failed.
    exit /b %ERRORLEVEL%
)

echo.
echo --- [GITPH] Compiling all modules and core application ---
REM Run the actual build process (compiles C/C++, Go, Rust).
cmake --build "%BUILD_DIR%" --parallel
IF %ERRORLEVEL% NEQ 0 (
    echo Build failed.
    exit /b %ERRORLEVEL%
)

echo.
echo --- [GITPH] Build complete! ---
echo Executable is available at: %BUILD_DIR%\bin\gitph.exe
echo Modules are in: %BUILD_DIR%\bin\modules\