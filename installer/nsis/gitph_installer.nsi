; Copyright (C) 2025 Pedro Henrique / phkaiser13
; gitph_installer.nsi - NSIS script for creating a Windows installer.
;
; This script orchestrates the creation of a user-friendly, native Windows
; installer for gitph. It provides a standard GUI wizard and handles file
; extraction, uninstaller creation, and Start Menu shortcuts.
;
; Its most critical task is to execute our custom C-based dependency checker
; (`installer_main.exe`) silently in the background. This combines the robust
; environment setup logic written in C with a professional GUI wrapper.
;
; SPDX-License-Identifier: GPL-3.0-or-later

!include "MUI2.nsh"

; --- General Installer Attributes ---
Name "gitph"
OutFile "gitph-installer.exe"
InstallDir "$LOCALAPPDATA\gitph"
RequestExecutionLevel user ; No admin rights needed, as we install to user profile.

; --- Modern UI Interface Settings ---
!define MUI_ABORTWARNING
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "..\..\LICENSE.txt" ; Assuming a LICENSE.txt file exists
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "English"

; --- Main Installation Section ---
Section "Install gitph (required)" SecInstall
  SetOutPath $INSTDIR

  ; Pack all necessary application files. The paths are relative to this script.
  File "..\..\build\bin\gitph.exe"
  File /r "..\..\build\modules"

  ; Pack our C-based dependency checker.
  File "..\..\build\installer\installer_main.exe"

  ; --- Execute the dependency checker and environment setup logic ---
  ; We use nsExec to run it silently in the background. The user only sees
  ; the main installer's progress bar.
  nsExec::ExecToLog '"$INSTDIR\installer_main.exe"'
  Pop $0 ; Get the return code from our C program.

  ; $0 will be "0" on success. Any other value indicates failure.
  ; (e.g., Git not found and user chose not to download).
  StrCmp $0 "0" continue_install abort_install

continue_install:
  ; The C program handled PATH setup. Now we just create shortcuts and the uninstaller.
  ; Clean up the temporary installer logic executable.
  Delete "$INSTDIR\installer_main.exe"

  ; --- Create Uninstaller ---
  WriteUninstaller "$INSTDIR\uninstall.exe"

  ; --- Registry keys for "Add/Remove Programs" ---
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\gitph" "DisplayName" "gitph (Git Helper)"
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\gitph" "UninstallString" '"$INSTDIR\uninstall.exe"'
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\gitph" "DisplayIcon" '"$INSTDIR\gitph.exe"'
  WriteRegDWORD HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\gitph" "NoModify" 1
  WriteRegDWORD HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\gitph" "NoRepair" 1

  Goto end_install

abort_install:
  ; Our C program failed. Inform the user and abort.
  MessageBox MB_OK|MB_ICONEXCLAMATION "Installation aborted. Please ensure Git is installed and try again."
  Abort

end_install:
SectionEnd

; --- Uninstallation Section ---
Section "Uninstall"
  ; TODO: The uninstaller needs logic to cleanly remove the PATH entry.
  ; This is a complex task that involves reading the registry, parsing the
  ; PATH string, removing our specific entry, and writing it back.
  ; For now, we just remove the files.

  Delete "$INSTDIR\gitph.exe"
  Delete "$INSTDIR\uninstall.exe"
  RMDir /r "$INSTDIR\modules"
  RMDir "$INSTDIR"

  ; Remove registry keys
  DeleteRegKey HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\gitph"
SectionEnd