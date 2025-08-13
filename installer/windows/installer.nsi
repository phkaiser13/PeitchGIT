; Copyright (C) 2025 Pedro Henrique / phkaiser13
; File: installer.nsi
; This script builds the Windows installer for phgit using the NSIS toolkit.
; It defines the installer's appearance, pages, and file installation logic.
; Its primary role is to unpack the application files and execute the C++
; installer_helper, which handles the complex task of managing dependencies
; like Git, Terraform, and Vault.
;
; SPDX-License-Identifier: Apache-2.0

!include "MUI2.nsh"
!include "LogicLib.nsh"
!include "EnvVarUpdate.nsh"

; --- Basic Installer Information ---
Name "phgit"
OutFile "phgit_installer_v1.0.0.exe" ; It's good practice to have a version here
InstallDir "$LOCALAPPDATA/phgit"
RequestExecutionLevel user ; No admin rights needed for user-local installation

; --- Modern UI Pages Configuration ---
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "PortugueseBR"

; --- Section 1: Core Application Files ---
; This section installs the main application executable and the dependency helper.
Section "phgit Core" SecCore
    SectionIn RO ; Read-only section, always installed.
    SetOutPath $INSTDIR

    ; DetailPrint messages are shown to the user during installation.
    DetailPrint "Installing phgit application files..."
    
    ; NOTE: The path should point to where your build system (e.g., CMake)
    ; outputs the final binaries. We assume a 'build/bin' directory.
    File /r "..\..\build\bin\phgit.exe" ; Main application executable
    File "..\..\build\bin\installer_helper.exe" ; Our C++ dependency helper
    
    ; Create the uninstaller.
    WriteUninstaller "$INSTDIR\uninstall.exe"
SectionEnd

; --- Section 2: Add to User's PATH ---
; This makes 'phgit' command available in cmd and PowerShell.
Section "Add to PATH" SecPath
    SetOutPath $INSTDIR
    DetailPrint "Adding phgit to user's PATH environment variable..."
    ${EnvVarUpdate} $0 "PATH" "A" "HKCU" "$INSTDIR" ; "A"ppend to the user's (HKCU) PATH
SectionEnd

; --- Post-Installation: Run Dependency Helper ---
; This function is automatically called by MUI after the file sections are complete.
Function .onInstSuccess
    DetailPrint "Verifying required dependencies (Git, Terraform, Vault)..."
    DetailPrint "This may take a few minutes and will download missing software."

    ; Execute our C++ helper and wait for it to complete.
    ; The helper will show its own progress bar in the console window.
    ; The exit code of the helper is pushed to the stack ($0).
    ExecWait '"$INSTDIR\installer_helper.exe"' $0

    ; Check the exit code. 0 means success. Any other value indicates an error.
    ${If} $0 != 0
        MessageBox MB_OK|MB_ICONEXCLAMATION "The dependency installer reported an error (exit code $0). phgit might not function correctly. Please review the installation log shown in the window above."
    ${Else}
        MessageBox MB_OK|MB_ICONINFORMATION "phgit and all its dependencies have been installed successfully!"
    ${EndIf}
FunctionEnd

; --- Uninstaller Section ---
Section "Uninstall"
    DetailPrint "Removing phgit from user's PATH..."
    ${EnvVarUpdate} $0 "PATH" "R" "HKCU" "$INSTDIR" ; "R"emove from the user's PATH

    DetailPrint "Deleting application files..."
    Delete "$INSTDIR\phgit.exe"
    Delete "$INSTDIR\installer_helper.exe"
    ; If phgit creates other files/folders, add them here.
    ; RMDir /r "$INSTDIR\some_other_folder"
    
    Delete "$INSTDIR\uninstall.exe"
    RMDir "$INSTDIR"
    
    DetailPrint "phgit has been successfully uninstalled."
SectionEnd
