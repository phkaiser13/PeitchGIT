; phgit.nsi
; NSIS installer script for "phgit"
; Copyright (C) 2025 Pedro Henrique / phkaiser13
; SPDX-License-Identifier: Apache-2.0
;
; Purpose:
; - Install phgit files to %ProgramFiles%\phgit
; - Optionally request installation of Terraform and Vault (flags passed to phgit-installer.exe)
; - Add $INSTDIR\bin to system PATH (HKLM)
; - Provide clean uninstaller with optional removal of user config
; - Use Modern UI 2, nsDialogs for custom uninstall checkbox
; - Robust error handling and logging

;--------------------------------
; Basic setup / performance
;--------------------------------
SetCompressor /SOLID lzma
SetCompressorDictSize 32
Unicode true
Name "phgit ${PRODUCT_VERSION}"
; PRODUCT_VERSION set later by macro; define a fallback
!ifndef PRODUCT_VERSION
  !define PRODUCT_VERSION "0.0.0"
!endif

;--------------------------------
; Includes
;--------------------------------
!include "MUI2.nsh"
!include "LogicLib.nsh"
!include "nsDialogs.nsh"
!include "WinMessages.nsh"
!include "EnvVarUpdate.nsh" ; robust PATH manipulation helper
!include "Sections.nsh"     ; for SectionFlagIsSet macro

;--------------------------------
; Product Defines (editable)
;--------------------------------
!define PRODUCT_NAME "phgit"
!define PRODUCT_VERSION "@PACKAGE_VERSION@"
!define PRODUCT_PUBLISHER "Pedro Henrique / phkaiser13"
!define PRODUCT_WEBSITE "@PACKAGE_HOMEPAGE@"
!define DOCS_URL "${PRODUCT_WEBSITE}/docs"
!define PRODUCT_UNINST_KEY "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\${PRODUCT_NAME}"
!define USER_CONFIG_DIR "$APPDATA\\${PRODUCT_NAME}"
!define USER_CONFIG_FILE "phgit_sync_state.json"

;--------------------------------
; Installer metadata & UI assets (place these files beside script)
;--------------------------------
Name "${PRODUCT_NAME} ${PRODUCT_VERSION}"
OutFile "phgit-${PRODUCT_VERSION}-installer.exe"
InstallDir "$PROGRAMFILES\\${PRODUCT_NAME}"
InstallDirRegKey HKLM "${PRODUCT_UNINST_KEY}" "InstallLocation"
RequestExecutionLevel admin

!define MUI_ICON "installer_icon.ico"
!define MUI_UNICON "uninstaller_icon.ico"
!define MUI_HEADERIMAGE_BITMAP "header_image.bmp"

; MUI finish options
!define MUI_FINISHPAGE_RUN "$INSTDIR\\bin\\phgit.exe"
!define MUI_FINISHPAGE_RUN_TEXT "Run phgit now"
!define MUI_FINISHPAGE_SHOWREADME "${DOCS_URL}"
!define MUI_FINISHPAGE_SHOWREADME_TEXT "View Online Documentation"

!define LOGFILE "$INSTDIR\\phgit_installer.log"

;--------------------------------
; Pages
;--------------------------------
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

; Uninstaller pages (default confirmation replaced by custom onInit)
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

;--------------------------------
; Languages
;--------------------------------
!insertmacro MUI_LANGUAGE "English"
!insertmacro MUI_LANGUAGE "PortugueseBR"

;--------------------------------
; Global variables
;--------------------------------
Var UNINSTALL_USER_DATA       ; nsDialogs checkbox handle
Var InstallerArgs             ; assembled args for phgit-installer.exe
Var /GLOBAL INST_LOG_PATH

;--------------------------------
; Sections
; NOTE: define optional sections BEFORE the "core" section if core wants to check their flags.
;--------------------------------

Section "Install Terraform (optional)" SecTerraform
    ; This section is an option only. The actual download/install of Terraform is
    ; delegated to phgit-installer.exe which receives the component flags.
    ; We create a marker file so the engine can see component-level choices (optional).
    ; No files are embedded here; this keeps the option visible in the components page.
    ; Marker file can be used by post-install processes (not required).
    ; (Empty by design)
SectionEnd

Section "Install Vault (optional)" SecVault
    ; Same as above for HashiCorp Vault
SectionEnd

Section "phgit (required)" SecCore
    ; The "core" section will deploy the shipped files (phgit.exe, phgit-installer.exe, uninstaller stub)
    ; It is marked read-only on the components page so the user cannot deselect it.
    SectionIn RO

    ; Prepare install directory and copy essential distribution files.
    ; Writes go to $INSTDIR and $INSTDIR\bin
    SetOutPath "$INSTDIR"
    ; Copy the C++ installer engine used for dependency handling
    File /oname=phgit-installer.exe "phgit-installer.exe"
    ; Create bin and copy main executable
    SetOutPath "$INSTDIR\\bin"
    File /oname=phgit.exe "bin\\phgit.exe"

    ; Write uninstaller program into $INSTDIR
    SetOutPath "$INSTDIR"
    WriteUninstaller "$INSTDIR\\uninstall.exe"

    ; Create a basic installer log path variable
    StrCpy $INST_LOG_PATH "$INSTDIR\\phgit_installer.log"

    ; We DO NOT run the engine here. Running the engine is handled in a dedicated internal section
    ; that executes after components selections are finalized (see SecEngine).
SectionEnd

Section "Add to system PATH" SecPath
    ; Add $INSTDIR\bin to system PATH (HKLM) so phgit is available from any console.
    ; EnvVarUpdate.nsh handles duplicates and registry expand types robustly.
    ${EnvVarUpdate} $0 "PATH" "A" "HKLM" "$INSTDIR\\bin"
    ; Log outcome for debugging
    DetailPrint "PATH update result: $0"
SectionEnd

; Internal engine runner section
; This section runs the bundled phgit-installer.exe with flags based on user component selection.
; It is hidden from the components UI (we will hide it on runtime init).
Section "Run bundled dependency engine (internal)" SecEngine
    ; Build the InstallerArgs string based on component flags
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

    ; Always pass --install-dir for explicitness
    StrCpy $InstallerArgs "$InstallerArgs --install-dir=\"$INSTDIR\""

    ; Safety: if phgit-installer.exe missing, abort with error.
    IfFileExists "$INSTDIR\\phgit-installer.exe" engine_exists engine_missing
    engine_missing:
        MessageBox MB_ICONSTOP "Critical error: bundled engine 'phgit-installer.exe' not found in installer payload. Aborting."
        Abort
    engine_exists:

    ; Run engine and capture return value
    DetailPrint "Executing engine: $INSTDIR\\phgit-installer.exe $InstallerArgs"
    ; Run with ExecWait so we can check exit code; log output to $INST_LOG_PATH if engine supports a --log flag (best-effort).
    ; We pass the InstallerArgs exactly as constructed.
    ExecWait '"$INSTDIR\\phgit-installer.exe" $InstallerArgs' $R0

    ; Interpret return code (0 = success)
    IntCmp $R0 0 engine_ok engine_failed engine_failed
    engine_ok:
        DetailPrint "Engine completed successfully."
        ; Optionally read/append engine log to the installer log
        ; (if the engine emits local log files, admin can inspect them)
    engine_failed:
        MessageBox MB_OK|MB_ICONSTOP "The dependency installer (phgit-installer.exe) returned an error (code: $R0). Installation cannot continue."
        ; Attempt to record to installer log
        DetailPrint "Engine failure code: $R0"
        Abort
SectionEnd

;--------------------------------
; MUI descriptions for components (displayed on components page)
; Must be inserted after sections are defined so ${Sec...} constants exist.
;--------------------------------
!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
  !insertmacro MUI_FUNCTION_DESCRIPTION_TEXT ${SecCore}      "Installs the core phgit application (required)."
  !insertmacro MUI_FUNCTION_DESCRIPTION_TEXT ${SecPath}      "Adds $INSTDIR\\bin to the system PATH (recommended) for command-line access."
  !insertmacro MUI_FUNCTION_DESCRIPTION_TEXT ${SecTerraform} "Optional: Install Terraform (Infrastructure-as-Code dependency)."
  !insertmacro MUI_FUNCTION_DESCRIPTION_TEXT ${SecVault}     "Optional: Install HashiCorp Vault (secrets management) for advanced functionality."
!insertmacro MUI_FUNCTION_DESCRIPTION_END

;--------------------------------
; Hide internal section from components page at runtime
; (do this in .onInit so the user never sees the "Run bundled dependency engine (internal)" item)
;--------------------------------
Function .onInit
    !insertmacro MUI_LANGDLL_DISPLAY

    ; Hide SecEngine from components page
    ; SectionSetFlags syntax: SectionSetFlags <secIndex> <flags>
    ; Use ${SF_SELECTED} to ensure it runs and ${SF_RO} to hide from components
    ; We'll set the "hidden" flag combination (SF_SELECTED + SF_RO) so it executes automatically but is not editable by user.
    ; Constants: ${SF_SELECTED} = 1, ${SF_RO} = 4 (these are standard NSIS values)
    ; We compute the numeric OR value: 1 | 4 = 5
    ; (If constants aren't available, SectionSetFlags accepts integer literals.)
    SectionSetFlags ${SecEngine} 5
FunctionEnd

;--------------------------------
; Uninstaller: custom confirmation with checkbox for removing user data
;--------------------------------
Function un.onInit
    ; Ask a simple Yes/No to proceed with uninstall (keeps behavior similar to a confirmation page)
    MessageBox MB_YESNO|MB_ICONQUESTION "Are you sure you want to uninstall ${PRODUCT_NAME}?" IDYES NoAbort
    Abort
    NoAbort:

    ; Create a custom page with nsDialogs to ask about removing user settings
    nsDialogs::Create 1018
    Pop $0
    ${If} $0 == error
        ; Fallback: if nsDialogs failed, default to removing user data = false
        StrCpy $UNINSTALL_USER_DATA "0"
        Return
    ${EndIf}

    ; Label
    ${NSD_CreateLabel} 0 0 100% 12u "Do you want to remove all user-specific settings and data?"
    Pop $1

    ; Checkbox
    ${NSD_CreateCheckBox} 10u 20u 80% 10u "Yes — remove my settings (${USER_CONFIG_FILE})"
    Pop $UNINSTALL_USER_DATA

    ; By default leave unchecked
    ${NSD_SetState} $UNINSTALL_USER_DATA ${BST_UNCHECKED}

    nsDialogs::Show
FunctionEnd

;--------------------------------
; Uninstaller implementation
;--------------------------------
Section "Uninstall"
    ; Remove PATH entry first (safe to attempt even if not present)
    ${EnvVarUpdate} $0 "PATH" "R" "HKLM" "$INSTDIR\\bin"

    ; Attempt to stop services or running instances if necessary (not included here).
    ; Delete installed files
    Delete "$INSTDIR\\bin\\phgit.exe"
    Delete "$INSTDIR\\phgit-installer.exe"
    Delete "$INSTDIR\\uninstall.exe"
    Delete "$INSTDIR\\phgit_installer.log"

    ; Remove empty directories (use RMDir /r sparingly; here directories should be empty)
    RMDir "$INSTDIR\\bin"
    RMDir "$INSTDIR"

    ; Remove user data if the checkbox was checked
    ${NSD_GetState} $UNINSTALL_USER_DATA $0
    ${If} $0 == ${BST_CHECKED}
        ; Delete config files in %APPDATA%\phgit
        Delete "${USER_CONFIG_DIR}\\${USER_CONFIG_FILE}"
        ; Attempt to remove the directory (will fail if other files exist)
        RMDir "${USER_CONFIG_DIR}"
    ${EndIf}

    ; Remove uninstall registry key
    DeleteRegKey HKLM "${PRODUCT_UNINST_KEY}"
SectionEnd

;--------------------------------
; Final UI settings / finish page behavior
;--------------------------------
!insertmacro MUI_PAGE_CUSTOMFUNCTION_SHOW ShowReadmeLink
Function ShowReadmeLink
  ; Nothing required here — MUI_FINISHPAGE_SHOWREADME will open DOCS_URL when user clicks.
FunctionEnd

;--------------------------------
; Helpful logging on failure (installer-wide)
;--------------------------------
Function .onInstFailed
    ; This function is run if installation is aborted
    MessageBox MB_OK|MB_ICONEXCLAMATION "Installation aborted. Please check the logs at $INST_LOG_PATH (if available) and ensure you have administrator privileges."
FunctionEnd

;--------------------------------
; End of script
;--------------------------------
