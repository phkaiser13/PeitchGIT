; Copyright (C) 2025 Pedro Henrique / phkaiser13
; SPDX-License-Identifier: Apache-2.0

!define PRODUCT_NAME "phgit"
!define PRODUCT_VERSION "@PACKAGE_VERSION@"
!define PRODUCT_PUBLISHER "Pedro Henrique / phkaiser13"
!define PRODUCT_WEBSITE "@PACKAGE_HOMEPAGE@"
!define PRODUCT_UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"

# Modern UI 2
!include "MUI2.nsh"

# General
Name "${PRODUCT_NAME} ${PRODUCT_VERSION}"
OutFile "phgit-${PRODUCT_VERSION}-installer.exe"
InstallDir "$PROGRAMFILES\${PRODUCT_NAME}"
InstallDirRegKey HKLM "${PRODUCT_UNINST_KEY}" "InstallLocation"
RequestExecutionLevel admin ; Request admin privileges

# Interface Settings
!define MUI_ABORTWARNING
!define MUI_ICON "path\to\icon.ico"

# Pages
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

# Language
!insertmacro MUI_LANGUAGE "English"
!insertmacro MUI_LANGUAGE "PortugueseBR"

Section "phgit (required)" SecCore
    SectionIn RO ; Read-only section

    SetOutPath $INSTDIR
    
    ; This is where our C++ installer engine is executed
    ; It will handle dependency checks (Git) and downloads
    File "phgit-installer.exe"
    ExecWait '"$INSTDIR\phgit-installer.exe"' $0
    
    ; Check the return code from our C++ installer
    IntCmp $0 0 continue
    MessageBox MB_OK|MB_ICONSTOP "Installation failed. Please check phgit_installer.log for details."
    Abort
    
continue:
    ; After the C++ engine runs, we install the actual application files
    File "path\to\phgit.exe"
    ; ... other application files

    ; Write uninstaller information
    WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "DisplayName" "${PRODUCT_NAME}"
    WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "UninstallString" '"$INSTDIR\uninstall.exe"'
    WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "DisplayVersion" "${PRODUCT_VERSION}"
    WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "Publisher" "${PRODUCT_PUBLISHER}"
    
    WriteUninstaller "$INSTDIR\uninstall.exe"
SectionEnd

Section "Uninstall"
    ; Uninstall logic here
    Delete "$INSTDIR\phgit.exe"
    Delete "$INSTDIR\phgit-installer.exe"
    Delete "$INSTDIR\uninstall.exe"
    RMDir "$INSTDIR"
    
    DeleteRegKey HKLM "${PRODUCT_UNINST_KEY}"
SectionEnd
