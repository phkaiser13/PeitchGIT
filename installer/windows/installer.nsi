; installer.nsi - Instalador NSIS funcional sem depender de EnvVarUpdate.nsh
; SPDX-License-Identifier: Apache-2.0

!include "MUI2.nsh"
!include "LogicLib.nsh"

!ifndef VERSION
  !define VERSION 0.0.0-dev
!endif

; Constants
!define WM_SETTINGCHANGE 0x001A

Name "phgit"
OutFile "phgit_installer_${VERSION}.exe"
InstallDir "$LOCALAPPDATA\phgit"
RequestExecutionLevel user

; Modern UI pages
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "PortugueseBR"

; -----------------------
; Section: Core Application
; -----------------------
Section "phgit Core" SecCore
    SetOutPath $INSTDIR
    DetailPrint "Installing phgit application files..."

    ; Ajuste esses caminhos conforme seu build output no momento da compilação do instalador
    File /r "..\..\build\bin\phgit.exe"
    File "..\..\build\bin\installer_helper.exe"

    ; Escreve o uninstaller
    WriteUninstaller "$INSTDIR\uninstall.exe"
SectionEnd

; -----------------------
; Section: Add to PATH
; -----------------------
Section "Add to PATH" SecPath
    SetOutPath $INSTDIR
    DetailPrint "Adicionando phgit ao PATH do usuário..."

    ; Chama a função que faz o append (não deduplica para simplicidade robusta)
    Push $INSTDIR
    Call AddToUserPath
SectionEnd

; -----------------------
; .onInstSuccess: executa helper de dependências
; -----------------------
Function .onInstSuccess
    DetailPrint "Verificando dependências (Git, Terraform, Vault)..."
    DetailPrint "O helper fará download/instalação adicional se necessário."

    ; Executa o helper e mostra o código de saída
    ExecWait '"$INSTDIR\installer_helper.exe"' $0

    ${If} $0 != 0
        MessageBox MB_OK|MB_ICONEXCLAMATION "O instalador de dependências retornou erro (código $0). phgit pode não funcionar corretamente."
    ${Else}
        MessageBox MB_OK|MB_ICONINFORMATION "phgit e dependências instaladas com sucesso!"
    ${EndIf}
FunctionEnd

; -----------------------
; Uninstall section
; -----------------------
Section "Uninstall"
    DetailPrint "Removendo phgit do PATH do usuário..."
    Push $INSTDIR
    Call RemoveFromUserPath

    DetailPrint "Removendo arquivos do aplicativo..."
    Delete "$INSTDIR\phgit.exe"
    Delete "$INSTDIR\installer_helper.exe"
    Delete "$INSTDIR\uninstall.exe"
    RMDir "$INSTDIR"

    DetailPrint "phgit desinstalado com sucesso."
SectionEnd

; -----------------------
; Função: AddToUserPath
;   Recebe na pilha: caminho a adicionar (ex.: $INSTDIR)
; -----------------------
Function AddToUserPath
    Exch $R0        ; $R0 = caminho a adicionar
    Push $R1
    Push $R2

    ; Lê PATH do HKCU
    ReadRegExpandStr $R1 HKCU "Environment" "Path"
    StrCmp $R1 "" +3
      StrCpy $R1 "$R1;$R0"
      Goto .write
    StrCpy $R1 "$R0"
  .write:
    WriteRegExpandStr HKCU "Environment" "Path" "$R1"

    ; Notifica o sistema para atualizar variáveis de ambiente
    System::Call 'user32::SendMessageTimeout(i -1, i ${WM_SETTINGCHANGE}, i 0, t "Environment", i 0x0002, i 1000, *i .r2)'

    Pop $R2
    Pop $R1
    Exch $R0
FunctionEnd

; -----------------------
; Função: RemoveFromUserPath
;   Recebe na pilha: caminho a remover (ex.: $INSTDIR)
;   Remove todas as ocorrências exatas do entry (não faz matching avançado)
; -----------------------
Function RemoveFromUserPath
    Exch $R0        ; $R0 = caminho a remover
    Push $R1
    Push $R2
    Push $R3
    Push $R4
    Push $R5
    Push $R6

    ReadRegExpandStr $R1 HKCU "Environment" "Path"
    StrCmp $R1 "" .done

    StrCpy $R2 ""    ; $R2 será o novo PATH reconstruído (sem entradas removidas)

  loop_start:
    StrLen $R3 $R1
    IntCmp $R3 0 loop_done 0 0

    ; encontrar próximo token (até ;)
    StrCpy $R4 ""      ; token temporário
    StrCpy $R5 0       ; índice
  find_char:
    IntCmp $R5 $R3 no_semi
    StrCpy $R6 $R1 1 $R5
    StrCmp $R6 ";" found_semi
    StrCpy $R4 "$R4$R6"
    IntOp $R5 $R5 + 1
    Goto find_char

  found_semi:
    IntOp $R6 $R5 + 1
    StrCpy $R1 $R1 -1 $R6
    Goto process_token

  no_semi:
    StrCpy $R4 $R1
    StrCpy $R1 ""
  process_token:
    StrCmp $R4 "$R0" skip_add
    StrCmp $R4 "$R0\" skip_add
    StrCmp $R2 "" add_first add_more
  add_first:
    StrCpy $R2 "$R4"
    Goto loop_start
  add_more:
    StrCpy $R2 "$R2;$R4"
    Goto loop_start
  skip_add:
    Goto loop_start

  loop_done:
    WriteRegExpandStr HKCU "Environment" "Path" "$R2"
    System::Call 'user32::SendMessageTimeout(i -1, i ${WM_SETTINGCHANGE}, i 0, t "Environment", i 0x0002, i 1000, *i .r2)'

  .done:
    Pop $R6
    Pop $R5
    Pop $R4
    Pop $R3
    Pop $R2
    Pop $R1
    Exch $R0
FunctionEnd
