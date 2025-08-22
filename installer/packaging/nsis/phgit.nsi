/* Copyright (C) 2025 Pedro Henrique / phkaiser13
*
* File: phgit.nsi
*
* This NSIS script is the primary installer for the phgit application. It is the
* single source of truth for all system integration tasks. Its responsibilities include:
*
* 1.  Extracting all application files, including the main executable (phgit.exe)
*     and the C++ post-installation assistant (phgit-installer.exe).
* 2.  Creating the uninstaller and the necessary registry entries for a clean
*     "Add/Remove Programs" experience.
* 3.  Optionally adding the application's binary directory to the system PATH.
* 4.  Optionally creating Start Menu and Desktop shortcuts.
*
* As a final step, it executes the C++ assistant. The assistant's sole purpose is
* to check for external dependencies (like Git) and interactively guide the user
* to install them if they are missing. This script handles all direct system
* modifications, while the C++ engine handles user guidance.
*
* SPDX-License-Identifier: Apache-2.0
*/

;--------------------------------
; Compiler Settings & Performance
;--------------------------------
SetCompressor /SOLID lzma
SetCompressorDictSize 32
Unicode true

;--------------------------------
; Product Information
;--------------------------------
!define PRODUCT_NAME "phgit"
!define PRODUCT_VERSION "1.0.0"
!define PRODUCT_PUBLISHER "Pedro Henrique / phkaiser13"
!define PRODUCT_WEBSITE "https://github.com/phkaiser13/phgit"
!define PRODUCT_UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"
!define USER_CONFIG_DIR "$APPDATA\${PRODUCT_NAME}"

;--------------------------------
; Installer Metadata
;--------------------------------
Name "${PRODUCT_NAME} ${PRODUCT_VERSION}"
OutFile "phgit-${PRODUCT_VERSION}-installer.exe"
InstallDir "$PROGRAMFILES\${PRODUCT_NAME}"
InstallDirRegKey HKLM "${PRODUCT_UNINST_KEY}" "InstallLocation"
RequestExecutionLevel admin
BrandingText " "

;--------------------------------
; Required Includes
;--------------------------------
!include "MUI2.nsh"
!include "LogicLib.nsh"
!include "WinMessages.nsh"
!include "FileFunc.nsh"
!include "x64.nsh"
!include "StrFunc.nsh"

;--------------------------------
; Modern UI Configuration
;--------------------------------
!define MUI_ABORTWARNING
!define MUI_UNABORTWARNING

; Finish page configuration
!define MUI_FINISHPAGE_RUN "$INSTDIR\bin\phgit.exe"
!define MUI_FINISHPAGE_RUN_TEXT "Run phgit now"
!define MUI_FINISHPAGE_SHOWREADME_NOTCHECKED
!define MUI_FINISHPAGE_SHOWREADME "${PRODUCT_WEBSITE}"
!define MUI_FINISHPAGE_SHOWREADME_TEXT "Visit project website"

;--------------------------------
; Installer Pages
;--------------------------------
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "LICENSE.txt"
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

;--------------------------------
; Uninstaller Pages
;--------------------------------
!insertmacro MUI_UNPAGE_WELCOME
Page custom un.CustomUninstallPage un.LeaveCustomPage
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH

;--------------------------------
; Languages
;--------------------------------
!insertmacro MUI_LANGUAGE "English"
!insertmacro MUI_LANGUAGE "PortugueseBR"

;--------------------------------
; Global Variables
;--------------------------------
Var /GLOBAL LogFile
Var /GLOBAL RemoveUserData
Var /GLOBAL PathAdded

;--------------------------------
; Utility Functions
;--------------------------------

; Writes a message to the log file with a timestamp.
Function WriteLog
    Exch $0
    Push $1
    Push $2
    Push $3
    Push $4
    Push $5
    Push $6
    Push $7
    ${GetTime} "" "L" $1 $2 $3 $4 $5 $6 $7
    FileOpen $1 "$LogFile" a
    ${If} $1 != ""
        FileWrite $1 "[$4-$3-$2 $5:$6:$7] $0$\r$\n"
        FileClose $1
    ${EndIf}
    Pop $7
    Pop $6
    Pop $5
    Pop $4
    Pop $3
    Pop $2
    Pop $1
    Pop $0
FunctionEnd

; Adds a directory to the system PATH environment variable if it's not already present.
Function AddToPath
    Exch $0
    Push $1
    Push $2
    Push $3
    ReadRegStr $1 HKLM "SYSTEM\CurrentControlSet\Control\Session Manager\Environment" "PATH"
    ${StrStr} $3 "$1" "$0"
    ${If} $3 == ""
        ${If} $1 == ""
            StrCpy $2 "$0"
        ${Else}
            StrCpy $2 "$1;$0"
        ${EndIf}
        WriteRegExpandStr HKLM "SYSTEM\CurrentControlSet\Control\Session Manager\Environment" "PATH" $2
        SendMessage ${HWND_BROADCAST} ${WM_WININICHANGE} 0 "STR:Environment" /TIMEOUT=5000
        StrCpy $PathAdded "1"
        Push "Added '$0' to system PATH."
        Call WriteLog
    ${Else}
        Push "'$0' already exists in system PATH. No changes made."
        Call WriteLog
    ${EndIf}
    Pop $3
    Pop $2
    Pop $1
    Pop $0
FunctionEnd

; Removes a directory from the system PATH environment variable.
Function un.RemoveFromPath
    Exch $0
    Push $1
    Push $2
    Push $3
    Push $4
    Push $5
    ReadRegStr $1 HKLM "SYSTEM\CurrentControlSet\Control\Session Manager\Environment" "PATH"
    StrCpy $2 $1
    ${StrRep} $2 $2 ";$0;" ";"
    ${StrRep} $2 $2 "$0;" ""
    ${StrRep} $2 $2 ";$0" ""
    ${StrRep} $2 $2 "$0" ""
    ${StrRep} $2 $2 ";;" ";"
    StrCpy $3 $2 1
    ${If} $3 == ";"
        StrCpy $2 $2 "" 1
    ${EndIf}
    StrLen $4 $2
    IntOp $5 $4 - 1
    StrCpy $3 $2 1 $5
    ${If} $3 == ";"
        StrCpy $2 $2 $5
    ${EndIf}
    ${If} $2 != $1
        WriteRegExpandStr HKLM "SYSTEM\CurrentControlSet\Control\Session Manager\Environment" "PATH" $2
        SendMessage ${HWND_BROADCAST} ${WM_WININICHANGE} 0 "STR:Environment" /TIMEOUT=5000
        Push "Removed '$0' from system PATH."
        Call un.WriteLog
    ${Else}
        Push "'$0' not found in system PATH or no changes needed."
        Call un.WriteLog
    ${EndIf}
    Pop $5
    Pop $4
    Pop $3
    Pop $2
    Pop $1
    Pop $0
FunctionEnd

;--------------------------------
; Installer Initialization
;--------------------------------
Function .onInit
    StrCpy $LogFile "$TEMP\phgit_installer.log"
    StrCpy $RemoveUserData "0"
    StrCpy $PathAdded "0"
    FileOpen $0 "$LogFile" w
    ${If} $0 != ""
        FileWrite $0 "phgit Installer Log - Session Started$\r$\n"
        FileClose $0
    ${EndIf}
    Push "Installer initialization completed."
    Call WriteLog
    !insertmacro MUI_LANGDLL_DISPLAY
    ReadRegStr $0 HKLM "${PRODUCT_UNINST_KEY}" "InstallLocation"
    ${If} $0 != ""
        MessageBox MB_YESNO|MB_ICONQUESTION \
            "phgit is already installed in '$0'.$\n$\nDo you want to continue and potentially overwrite the existing installation?" \
            IDYES continue IDNO abort
        abort:
            Abort
        continue:
    ${EndIf}
FunctionEnd

;--------------------------------
; Installation Sections
;--------------------------------

Section "phgit Core" SecCore
    SectionIn RO
    SetOutPath "$INSTDIR"
    Push "Starting installation of core phgit application."
    Call WriteLog
    CreateDirectory "$INSTDIR\bin"
    CreateDirectory "$INSTDIR\config"
    File /oname=bin\phgit.exe "phgit.exe"
    File /oname=bin\phgit-installer.exe "phgit-installer.exe"
    File /oname=config\config.json "config.json"
    File /oname=LICENSE.txt "LICENSE"
    File /oname=README.md "README.md"
    WriteUninstaller "$INSTDIR\uninstall.exe"
    WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "DisplayName" "${PRODUCT_NAME}"
    WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "DisplayVersion" "${PRODUCT_VERSION}"
    WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "Publisher" "${PRODUCT_PUBLISHER}"
    WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "URLInfoAbout" "${PRODUCT_WEBSITE}"
    WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "InstallLocation" "$INSTDIR"
    WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "UninstallString" '"$INSTDIR\uninstall.exe"'
    WriteRegDWORD HKLM "${PRODUCT_UNINST_KEY}" "NoModify" 1
    WriteRegDWORD HKLM "${PRODUCT_UNINST_KEY}" "NoRepair" 1
    ${GetSize} "$INSTDIR" "/S=0K" $0 $1 $2
    WriteRegDWORD HKLM "${PRODUCT_UNINST_KEY}" "EstimatedSize" "$0"
    Push "Core files extracted successfully."
    Call WriteLog

    ; --- CRITICAL STEP: Execute the C++ post-install assistant ---
    ; The C++ assistant will now run to check for external dependencies like Git.
    ; It will not install anything silently; it will guide the user if action is needed.
    DetailPrint "Running post-installation assistant to check for required dependencies..."
    DetailPrint "A console window will open to guide you through the process."
    Push "Executing C++ assistant: $INSTDIR\bin\phgit-installer.exe"
    Call WriteLog

    ExecWait '"$INSTDIR\bin\phgit-installer.exe"' $0
    ${If} $0 != 0
        Push "C++ assistant reported an issue or was closed with exit code: $0"
        Call WriteLog
        MessageBox MB_OK|MB_ICONINFORMATION "The dependency check finished. The core application is installed, but may require manual setup of its dependencies to function correctly."
    ${Else}
        Push "C++ assistant completed successfully."
        Call WriteLog
        DetailPrint "Dependency check complete."
    ${EndIf}
SectionEnd

Section "Add to System PATH" SecPath
    Push "Adding phgit to system PATH."
    Call WriteLog
    Push "$INSTDIR\bin"
    Call AddToPath
    WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "PathAdded" "1"
    DetailPrint "Added '$INSTDIR\bin' to system PATH."
SectionEnd

Section /o "Create Desktop Shortcut" SecDesktop
    CreateShortCut "$DESKTOP\phgit.lnk" "$INSTDIR\bin\phgit.exe" "" "$INSTDIR\bin\phgit.exe" 0
    Push "Desktop shortcut created."
    Call WriteLog
SectionEnd

Section "Create Start Menu Shortcuts" SecStartMenu
    CreateDirectory "$SMPROGRAMS\${PRODUCT_NAME}"
    CreateShortCut "$SMPROGRAMS\${PRODUCT_NAME}\phgit.lnk" "$INSTDIR\bin\phgit.exe"
    CreateShortCut "$SMPROGRAMS\${PRODUCT_NAME}\Uninstall ${PRODUCT_NAME}.lnk" "$INSTDIR\uninstall.exe"
    CreateShortCut "$SMPROGRAMS\${PRODUCT_NAME}\Documentation.lnk" "$INSTDIR\README.md"
    Push "Start Menu shortcuts created."
    Call WriteLog
SectionEnd

;--------------------------------
; Section Descriptions
;--------------------------------
!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
    !insertmacro MUI_DESCRIPTION_TEXT ${SecCore} "Installs the core phgit application and runs the dependency checker. (Required)"
    !insertmacro MUI_DESCRIPTION_TEXT ${SecPath} "Adds the application directory to the system PATH for easy command-line access."
    !insertmacro MUI_DESCRIPTION_TEXT ${SecDesktop} "Creates a shortcut to phgit on your desktop."
    !insertmacro MUI_DESCRIPTION_TEXT ${SecStartMenu} "Creates shortcuts in the Start Menu."
!insertmacro MUI_FUNCTION_DESCRIPTION_END

;--------------------------------
; Installation Callbacks
;--------------------------------
Function .onInstSuccess
    Push "Installation process finished successfully."
    Call WriteLog
    CopyFiles /SILENT "$LogFile" "$INSTDIR\installer_log.txt"
    DetailPrint "Installation completed."
    DetailPrint "Log file saved to: $INSTDIR\installer_log.txt"
FunctionEnd

Function .onInstFailed
    Push "Installation process failed."
    Call WriteLog
    MessageBox MB_OK|MB_ICONEXCLAMATION \
        "The installation failed. Please check the log file for details:$\n$LogFile"
FunctionEnd

;--------------------------------
; Custom Uninstaller Page
;--------------------------------
Var UninstDialog
Var UninstCheckbox

Function un.CustomUninstallPage
    nsDialogs::Create 1018
    Pop $UninstDialog
    ${If} $UninstDialog == error
        Abort
    ${EndIf}
    ${NSD_CreateLabel} 0 0 100% 30u \
        "The following phgit components will be removed:$\n$\n\
        • Application files and binaries$\n\
        • Registry entries and Start Menu shortcuts$\n\
        • System PATH entry (if added by installer)"
    ${NSD_CreateCheckBox} 0 120u 100% 15u \
        "&Remove all user configuration and data files"
    Pop $UninstCheckbox
    ${NSD_CreateLabel} 20u 140u 80% 20u \
        "Warning: This is irreversible and will delete all settings from:$\n${USER_CONFIG_DIR}"
    nsDialogs::Show
FunctionEnd

Function un.LeaveCustomPage
    ${NSD_GetState} $UninstCheckbox $RemoveUserData
FunctionEnd

;--------------------------------
; Uninstaller Section
;--------------------------------
Function un.onInit
    StrCpy $LogFile "$TEMP\phgit_uninstaller.log"
    FileOpen $0 "$LogFile" w
    ${If} $0 != ""
        FileWrite $0 "phgit Uninstaller Log - Session Started$\r$\n"
        FileClose $0
    ${EndIf}
    !insertmacro MUI_UNGETLANGUAGE
FunctionEnd

Section "Uninstall"
    ReadRegStr $0 HKLM "${PRODUCT_UNINST_KEY}" "PathAdded"
    ${If} $0 == "1"
        Push "$INSTDIR\bin"
        Call un.RemoveFromPath
        DetailPrint "Removed from system PATH."
    ${EndIf}
    RMDir /r "$SMPROGRAMS\${PRODUCT_NAME}"
    Delete "$DESKTOP\phgit.lnk"
    DetailPrint "Removing application files..."
    RMDir /r "$INSTDIR"
    ${If} $RemoveUserData == ${BST_CHECKED}
        DetailPrint "Removing user configuration files..."
        RMDir /r "${USER_CONFIG_DIR}"
    ${EndIf}
    DeleteRegKey HKLM "${PRODUCT_UNINST_KEY}"
    DetailPrint "Uninstallation completed."
SectionEnd

Function un.onUninstSuccess
    MessageBox MB_OK "phgit has been successfully removed from your system."
    MessageBox MB_YESNO "Would you like to view the uninstall log?" IDNO cleanup
        ExecShell "open" "$LogFile"
    cleanup:
FunctionEnd

;--------------------------------
; Version Info
;--------------------------------
VIProductVersion "${PRODUCT_VERSION}.0"
VIAddVersionKey "ProductName" "${PRODUCT_NAME}"
VIAddVersionKey "ProductVersion" "${PRODUCT_VERSION}"
VIAddVersionKey "CompanyName" "${PRODUCT_PUBLISHER}"
VIAddVersionKey "FileDescription" "${PRODUCT_NAME} Installer"
VIAddVersionKey "FileVersion" "${PRODUCT_VERSION}"
VIAddVersionKey "LegalCopyright" "Copyright (C) 2025 ${PRODUCT_PUBLISHER}"