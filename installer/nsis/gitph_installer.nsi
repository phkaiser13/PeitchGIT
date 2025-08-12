; Copyright (C) 2025 Pedro Henrique / phkaiser13
; SPDX-License-Identifier: Apache-2.0

!include "MUI2.nsh"
!include "LogicLib.nsh"

Name "gitph"
OutFile "gitph_installer_v${VERSION}.exe"
InstallDir "$LOCALAPPDATA\gitph"
RequestExecutionLevel user

!define GIT_INSTALLER_URL "https://github.com/git-for-windows/git/releases/download/v2.45.1.windows.1/Git-2.45.1-64-bit.exe"
!define GIT_INSTALLER_FILENAME "Git-Installer.exe"

Var IsGitInstalled

Function .onInit
    ; Checa se o Git está instalado de forma silenciosa.
    nsExec::ExecToLog 'git --version'
    Pop $0 ; Pega o código de retorno
    ${If} $0 == 0
        StrCpy $IsGitInstalled "true"
    ${Else}
        StrCpy $IsGitInstalled "false"
    ${EndIf}
FunctionEnd

Section "gitph Core" SecInstall
    SectionIn RO
    SetOutPath $INSTDIR
    ; Empacota os arquivos principais do gitph
    File "build\\bin\\gitph.exe"
    File /r "build\\bin\\modules"
    ; Empacota o nosso helper de instalação
    File "build\\installer\\installer_helper.exe"
    WriteUninstaller "$INSTDIR\uninstall.exe"
SectionEnd

; ... (outras seções como PATH, atalhos, etc.) ...

Function .onInstSuccess
    ${If} $IsGitInstalled == "false"
        MessageBox MB_YESNO|MB_ICONQUESTION \
          "Git is not found. It is required for gitph to work. Do you want to download and install it now?" \
          IDYES download_git
        Goto end_git_check

    download_git:
        ; Extrai e executa o nosso downloader em C para uma experiência de usuário superior
        SetOutPath $PLUGINSDIR
        
        ; Executa o downloader profissional em C que você criou para baixar o Git
        ExecWait '"$INSTDIR\installer_helper.exe" "${GIT_INSTALLER_URL}" "$PLUGINSDIR\${GIT_INSTALLER_FILENAME}"'
        
        ; Após o download, executa o instalador do Git de forma silenciosa
        ExecWait '"$PLUGINSDIR\${GIT_INSTALLER_FILENAME}" /VERYSILENT /NORESTART'

        ; Re-verifica se a instalação do Git foi bem-sucedida
        nsExec::ExecToLog 'git --version'
        Pop $0
        ${If} $0 == 0
            MessageBox MB_OK|MB_ICONINFORMATION "Git was successfully installed!"
        ${Else}
            MessageBox MB_OK|MB_ICONEXCLAMATION "Git installation may not have completed successfully. gitph might not work."
        ${EndIf}
    ${EndIf}
end_git_check:
FunctionEnd
