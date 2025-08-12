; Copyright (C) 2025 Pedro Henrique / phkaiser13
; SPDX-License-Identifier: Apache-2.0

!include "MUI2.nsh"
!include "LogicLib.nsh"
!include "WordFunc.nsh"
!include "EnvVarUpdate.nsh"

Name "gitph"
OutFile "gitph_installer_v${VERSION}.exe"
InstallDir "$LOCALAPPDATA\gitph"
RequestExecutionLevel user

!define GIT_INSTALLER_URL "https://github.com/git-for-windows/git/releases/download/v2.45.1.windows.1/Git-2.45.1-64-bit.exe"
!define GIT_INSTALLER_FILENAME "Git-Installer.exe"

Var IsGitInstalled

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "PortugueseBR"

Function .onInit
    nsExec::ExecToLog 'git --version'
    Pop $0
    ${If} $0 == 0
        StrCpy $IsGitInstalled "true"
    ${Else}
        StrCpy $IsGitInstalled "false"
    ${EndIf}
FunctionEnd

Section "gitph Core" SecCore
    SectionIn RO
    SetOutPath $INSTDIR
    
    ; Define o diretório de build a partir da raiz do repositório no runner do GitHub Actions
    File /r "build\bin\*"
    File "build\installer\installer_helper.exe"
    
    WriteUninstaller "$INSTDIR\uninstall.exe"
SectionEnd

Section "Add to PATH" SecPath
    ; Adiciona o diretório de instalação ao PATH do usuário
    ${EnvVarUpdate} $0 "PATH" "A" "HKCU" "$INSTDIR"
SectionEnd

Function .onInstSuccess
    ${If} $IsGitInstalled == "false"
        MessageBox MB_YESNO|MB_ICONQUESTION \
          "Git não foi encontrado. Ele é necessário para o gitph funcionar. Deseja baixar e instalar agora?" \
          IDYES download_git
        Goto end_git_check

    download_git:
        ExecWait '"$INSTDIR\installer_helper.exe" "${GIT_INSTALLER_URL}" "$PLUGINSDIR\${GIT_INSTALLER_FILENAME}"'
        ExecWait '"$PLUGINSDIR\${GIT_INSTALLER_FILENAME}" /VERYSILENT /NORESTART'

        nsExec::ExecToLog 'git --version'
        Pop $0
        ${If} $0 == 0
            MessageBox MB_OK|MB_ICONINFORMATION "Git foi instalado com sucesso!"
        ${Else}
            MessageBox MB_OK|MB_ICONEXCLAMATION "A instalação do Git pode não ter sido concluída com sucesso. O gitph pode não funcionar."
        ${EndIf}
    ${EndIf}
end_git_check:
FunctionEnd

Section "Uninstall"
    ; Remove do PATH
    ${EnvVarUpdate} $0 "PATH" "R" "HKCU" "$INSTDIR"

    ; Remove os arquivos
    Delete "$INSTDIR\gitph.exe"
    Delete "$INSTDIR\installer_helper.exe"
    RMDir /r "$INSTDIR\modules"
    
    Delete "$INSTDIR\uninstall.exe"
    RMDir "$INSTDIR"
SectionEnd
