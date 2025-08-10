; Copyright (C) 2025 Pedro Henrique / phkaiser13
; gitph_installer.nsi - NSIS script for creating a Windows installer.
;
; Versão Profissional:
; - Detecção de dependência (Git) nativa no NSIS.
; - Download e execução automática do instalador do Git.
; - Manipulação segura da variável de ambiente PATH.
; - Página de componentes para escolhas do usuário.
; - Criação de atalhos e melhorias de UX.
;
; SPDX-License-Identifier: Apache-2.0

!include "MUI2.nsh"
!include "LogicLib.nsh"
!include "WordFunc.nsh" ; Necessário para EnvVarUpdate
!include "EnvVarUpdate.nsh" ; Biblioteca para manipular o PATH

; --- Atributos Gerais ---
Name "gitph"
OutFile "gitph-installer-pro.exe"
InstallDir "$LOCALAPPDATA\gitph"
RequestExecutionLevel user ; Perfeito para instalação no perfil do usuário.

; --- Ícones e Branding ---
!define MUI_ICON "..\..\path\to\your\app_icon.ico"
!define MUI_UNICON "..\..\path\to\your\uninst_icon.ico"

; --- Constantes ---
!define APP_NAME "gitph"
!define GIT_URL "https://github.com/git-for-windows/git/releases/download/v2.45.1.windows.1/Git-2.45.1-64-bit.exe"
!define GIT_INSTALLER_NAME "Git-Installer.exe"

; --- Variáveis Globais ---
Var IsGitInstalled

; --- Páginas da Interface Moderna ---
!define MUI_ABORTWARNING
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "..\..\LICENSE.txt"
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "English"

; --- Seção de Descrições (para a página de componentes) ---
LangString DESC_SecInstall ${LANG_ENGLISH} "Installs the main application files for gitph."
LangString DESC_SecPath ${LANG_ENGLISH} "Add the gitph directory to the user's PATH to run 'gitph' from any terminal."
LangString DESC_SecDesktopShortcut ${LANG_ENGLISH} "Create a shortcut on the Desktop."

; --- Função de Inicialização (.onInit) ---
; Executada antes de qualquer página ser exibida.
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

; --- Seção Principal de Instalação ---
Section "gitph Core Files" SecInstall
  SectionIn RO ; Seção obrigatória, não pode ser desmarcada.
  SetOutPath $INSTDIR

  ; Empacota os arquivos da aplicação
  File "..\..\build\bin\gitph.exe"
  File /r "..\..\build\modules"

  ; Cria o desinstalador
  WriteUninstaller "$INSTDIR\uninstall.exe"

  ; --- Chaves de Registro para "Adicionar/Remover Programas" ---
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "DisplayName" "${APP_NAME} (Git Helper)"
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "UninstallString" '"$INSTDIR\uninstall.exe"'
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "InstallLocation" "$INSTDIR"
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "DisplayIcon" '"$INSTDIR\gitph.exe"'
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "Publisher" "Pedro Henrique / phkaiser13"
  ; Adicione URL de ajuda, etc. se desejar
  WriteRegDWORD HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "NoModify" 1
  WriteRegDWORD HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "NoRepair" 1
SectionEnd

; --- Seção Opcional: Adicionar ao PATH ---
Section "Add to PATH" SecPath
  ; Adiciona o diretório ao PATH do usuário
  ${EnvVarUpdate} $0 "PATH" "A" "HKCU" "$INSTDIR"
SectionEnd

; --- Seção Opcional: Atalho na Área de Trabalho ---
Section "Desktop Shortcut" SecDesktopShortcut
  CreateShortCut "$DESKTOP\${APP_NAME}.lnk" "$INSTDIR\gitph.exe"
SectionEnd

; --- Função de Instalação customizada para checar o Git ---
Function .onInstSuccess
  ; Esta função é chamada após a cópia dos arquivos.
  ; Aqui checamos se o Git precisa ser instalado.
  ${If} $IsGitInstalled == "false"
    ; Pergunta ao usuário se ele quer baixar o Git.
    MessageBox MB_YESNO|MB_ICONQUESTION \
      "Git is not found on your system. It is required for gitph to work.$\n$\nDo you want to download and install it now?" \
      IDYES download_git
    
    ; Se o usuário disser não, avisa e termina.
    MessageBox MB_OK|MB_ICONEXCLAMATION "Installation of gitph is complete, but it will not function without Git."
    Goto end_git_check

  download_git:
    ; Usa o plugin INetC para baixar o instalador do Git com UI integrada.
    INetC::get "${GIT_URL}" "$PLUGINSDIR\${GIT_INSTALLER_NAME}" /end
    Pop $0 ; Pega o resultado do download ('OK' ou erro)
    ${If} $0 != "OK"
      MessageBox MB_OK|MB_ICONERROR "Failed to download the Git installer. Please install it manually."
      Goto end_git_check
    ${EndIf}

    ; Executa o instalador do Git e espera ele terminar.
    MessageBox MB_OK|MB_ICONINFORMATION "Download complete. The Git installer will now start. Please follow its instructions."
    ExecWait '"$PLUGINSDIR\${GIT_INSTALLER_NAME}"'
    
    ; Após a instalação, podemos re-verificar se o Git agora existe.
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


; --- Seção de Desinstalação ---
Section "Uninstall"
  ; Remove os arquivos
  Delete "$INSTDIR\gitph.exe"
  Delete "$INSTDIR\uninstall.exe"
  RMDir /r "$INSTDIR\modules"
  
  ; Remove o atalho da área de trabalho, se existir
  Delete "$DESKTOP\${APP_NAME}.lnk"

  ; Remove o diretório do PATH do usuário de forma segura
  ${EnvVarUpdate} $0 "PATH" "R" "HKCU" "$INSTDIR"

  ; Remove o diretório principal (se estiver vazio)
  RMDir "$INSTDIR"

  ; Remove as chaves de registro
  DeleteRegKey HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}"
SectionEnd
