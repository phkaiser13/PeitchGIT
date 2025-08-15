/* Copyright (C) 2025 Pedro Henrique / phkaiser13
* File: phgit.nsi
* This NSIS (Nullsoft Scriptable Install System) script is responsible for creating a robust
* Windows installer for the 'phgit' application. It handles the installation of application
* files, creation of registry keys, modification of the system's PATH environment variable,
* and ensures a clean and complete uninstallation process. This version includes a components
* page to select optional dependencies (Terraform, Vault), custom branding, and a finish
* page with post-install actions.
* SPDX-License-Identifier: Apache-2.0 */

;--------------------------------
; Includes
;--------------------------------

!include "MUI2.nsh"
!include "LogicLib.nsh"
!include "Sections.nsh" ; Required for SectionFlagIsSet macro
!include "EnvVarUpdate.nsh" ; Include for robust PATH manipulation

;--------------------------------
; Product Defines
;--------------------------------

!define PRODUCT_NAME "phgit"
!define PRODUCT_VERSION "@PACKAGE_VERSION@"
!define PRODUCT_PUBLISHER "Pedro Henrique / phkaiser13"
!define PRODUCT_WEBSITE "@PACKAGE_HOMEPAGE@"
!define DOCS_URL "${PRODUCT_WEBSITE}/docs"
!define PRODUCT_UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"
!define USER_CONFIG_DIR "$APPDATA\${PRODUCT_NAME}"
!define USER_CONFIG_FILE "phgit_sync_state.json"

;--------------------------------
; General Installer Settings
;--------------------------------

Name "${PRODUCT_NAME} ${PRODUCT_VERSION}"
OutFile "phgit-${PRODUCT_VERSION}-installer.exe"
InstallDir "$PROGRAMFILES\${PRODUCT_NAME}"
InstallDirRegKey HKLM "${PRODUCT_UNINST_KEY}" "InstallLocation"
RequestExecutionLevel admin ; Request admin privileges for HKLM writes and PATH modification

;--------------------------------
; Modern UI 2 Interface Settings
;--------------------------------

!define MUI_ABORTWARNING

; --- Custom Visuals ---
; Create these files and place them in the same directory as this script.
!define MUI_ICON "installer_icon.ico"
!define MUI_UNICON "uninstaller_icon.ico"
!define MUI_HEADERIMAGE
!define MUI_HEADERIMAGE_BITMAP "header_image.bmp" ; Must be a BMP file

; --- Finish Page Options ---
!define MUI_FINISHPAGE_RUN "$INSTDIR\bin\phgit.exe"
!define MUI_FINISHPAGE_RUN_TEXT "Run phgit now"
!define MUI_FINISHPAGE_SHOWREADME "${DOCS_URL}"
!define MUI_FINISHPAGE_SHOWREADME_TEXT "View Online Documentation"

;--------------------------------
; Global Variables
;--------------------------------

; Variable to hold the user's choice for removing settings during uninstall
Var UNINSTALL_USER_DATA
; Variable to hold the command-line arguments for the C++ installer engine
Var InstallerArgs

;--------------------------------
; Installer Pages
;--------------------------------

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

;--------------------------------
; Uninstaller Pages
;--------------------------------

; Custom function to show a checkbox on the uninstall confirmation page
Function un.onInit
    !insertmacro MUI_UNGETLANGUAGE
    MessageBox MB_YESNO|MB_ICONQUESTION \
        "Are you sure you want to completely remove ${PRODUCT_NAME} and all of its components?" \
        IDYES NoAbort
        Abort
    NoAbort:
    
    ; Create a custom page to ask about removing user data
    nsDialogs::Create 1018
    Pop $0

    ${NSD_CreateLabel} 0 0 100% 12u "Do you want to remove all user-specific settings and data?"
    ${NSD_CreateCheckBox} 10u 20u 80% 10u "Yes, remove my settings (${USER_CONFIG_FILE})."
    Pop $UNINSTALL_USER_DATA
    
    nsDialogs::Show
FunctionEnd

!insertmacro MUI_UNPAGE_INSTFILES

;--------------------------------
; Languages
;--------------------------------

!insertmacro MUI_LANGUAGE "English"
!insertmacro MUI_LANGUAGE "PortugueseBR"

;--------------------------------
; Installer Sections
;--------------------------------

; Use a function to set descriptions for the components page
Function .onInit
    !insertmacro MUI_LANGDLL_DISPLAY
    !insertmacro MUI_DESCRIPTION_BEGIN
        !insertmacro MUI_DESCRIPTION_TEXT ${SecCore} "Installs the core phgit application."
        !insertmacro MUI_DESCRIPTION_TEXT ${SecPath} "Adds the application directory to the system PATH for easy command-line access."
        !insertmacro MUI_DESCRIPTION_TEXT ${SecTerraform} "Automatically downloads and installs Terraform, a dependency for IaC features."
        !insertmacro MUI_DESCRIPTION_TEXT ${SecVault} "Automatically downloads and installs HashiCorp Vault, a dependency for secret management."
    !insertmacro MUI_DESCRIPTION_END
FunctionEnd

Section "phgit (required)" SecCore
    SectionIn RO ; This section is read-only and cannot be deselected

    ; Build the command-line arguments for the C++ installer engine based on component selection
    StrCpy $InstallerArgs ""
    !insertmacro SectionFlagIsSet ${SecTerraform}
    Pop $0
    ${If} $0 == 1
        StrCpy $InstallerArgs "$InstallerArgs --install-terraform"
    ${EndIf}
    !insertmacro SectionFlagIsSet ${SecVault}
    Pop $0
    ${If} $0 == 1
        StrCpy $InstallerArgs "$InstallerArgs --install-vault"
    ${EndIf}

    SetOutPath $INSTDIR
    
    ; The C++ installer engine handles dependency checks and downloads.
    ; We pass the user's choices as command-line arguments.
    File "phgit-installer.exe"
    DetailPrint "Running core installer with arguments: $InstallerArgs"
    ExecWait '"$INSTDIR\phgit-installer.exe" $InstallerArgs' $0
    
    ; Check the return code from the C++ installer engine.
    IntCmp $0 0 continue_install
    MessageBox MB_OK|MB_ICONSTOP "Core installation failed. Please check phgit_installer.log for details."
    Abort
    
continue_install:
    ; After the core engine succeeds, install the actual application files into a 'bin' subdirectory.
    SetOutPath "$INSTDIR\bin"
    File "bin\phgit.exe"

    ; Write uninstaller information to the registry.
    SetOutPath $INSTDIR
    WriteUninstaller "$INSTDIR\uninstall.exe"
    
    WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "DisplayName" "${PRODUCT_NAME}"
    WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "UninstallString" '"$INSTDIR\uninstall.exe"'
    WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "QuietUninstallString" '"$INSTDIR\uninstall.exe" /S'
    WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "InstallLocation" "$INSTDIR"
    WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "DisplayIcon" '"$INSTDIR\bin\phgit.exe"'
    WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "DisplayVersion" "${PRODUCT_VERSION}"
    WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "Publisher" "${PRODUCT_PUBLISHER}"
SectionEnd

Section "Add to system PATH" SecPath
    ${EnvVarUpdate} $0 "PATH" "A" "HKLM" "$INSTDIR\bin"
SectionEnd

; --- Optional Dependencies ---
; These sections are empty. Their only purpose is to provide a checkbox on the
; components page. The actual installation logic is handled by the C++ installer
; engine based on the command-line flags built in the main section.

Section "Install Terraform" SecTerraform
SectionEnd

Section "Install Vault" SecVault
SectionEnd

;--------------------------------
; Uninstaller Section
;--------------------------------

Section "Uninstall"
    ; Step 1: Remove the application directory from the system PATH.
    ${EnvVarUpdate} $0 "PATH" "R" "HKLM" "$INSTDIR\bin"

    ; Step 2: Delete the installed files.
    Delete "$INSTDIR\bin\phgit.exe"
    Delete "$INSTDIR\phgit-installer.exe"
    Delete "$INSTDIR\uninstall.exe"
    
    ; Step 3: Remove the directories.
    RMDir "$INSTDIR\bin"
    RMDir "$INSTDIR"

    ; Step 4: Check if the user chose to remove their settings.
    ${NSD_GetState} $UNINSTALL_USER_DATA $0
    ${If} $0 == ${BST_CHECKED}
        Delete "${USER_CONFIG_DIR}\${USER_CONFIG_FILE}"
        RMDir "${USER_CONFIG_DIR}"
    ${EndIf}

    ; Step 5: Remove the uninstallation key from the registry.
    DeleteRegKey HKLM "${PRODUCT_UNINST_KEY}"
SectionEnd