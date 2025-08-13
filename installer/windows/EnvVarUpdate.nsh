!ifndef ENVVARUPDATE_FUNCTION
!define ENVVARUPDATE_FUNCTION
!verbose push
!verbose 3
!include "LogicLib.nsh"
!include "WinMessages.nsh"
!include "StrFunc.nsh"

!macro _IncludeStrFunction StrFuncName
  !ifndef ${StrFuncName}_INCLUDED
    ${${StrFuncName}}
  !endif
  !ifndef Un${StrFuncName}_INCLUDED
    ${Un${StrFuncName}}
  !endif
  !define un.${StrFuncName} "${Un${StrFuncName}}"
!macroend

!insertmacro _IncludeStrFunction StrTok
!insertmacro _IncludeStrFunction StrStr
!insertmacro _IncludeStrFunction StrRep

!macro _EnvVarUpdateConstructor ResultVar EnvVarName Action Regloc PathString
  Push "${EnvVarName}"
  Push "${Action}"
  Push "${RegLoc}"
  Push "${PathString}"
  Call EnvVarUpdate
  Pop "${ResultVar}"
!macroend
!define EnvVarUpdate '!insertmacro "_EnvVarUpdateConstructor"'

!macro _unEnvVarUpdateConstructor ResultVar EnvVarName Action Regloc PathString
  Push "${EnvVarName}"
  Push "${Action}"
  Push "${RegLoc}"
  Push "${PathString}"
  Call un.EnvVarUpdate
  Pop "${ResultVar}"
!macroend
!define un.EnvVarUpdate '!insertmacro "_unEnvVarUpdateConstructor"'

Function EnvVarUpdate
  Exch $0
  Push $1
  Push $2
  Push $3
  Push $4
  Push $5
  Push $6
  Push $7
  Push $8
  Push $9
  Push $R0

  ; Parâmetros de input:
  ; $1 = EnvVarName
  ; $2 = Action ("A", "P" ou "R")
  ; $3 = RegLoc ("HKLM" ou "HKCU")
  ; $4 = PathString

  ; Validações básicas
  ${If} $1 == ""
    SetErrors
    DetailPrint "ERROR: EnvVarName is blank"
    Goto EnvVarUpdate_Restore_Vars
  ${EndIf}
  ${If} $2 != "A" ${AndIf} $2 != "P" ${AndIf} $2 != "R"
    SetErrors
    DetailPrint "ERROR: Invalid Action - must be A, P, or R"
    Goto EnvVarUpdate_Restore_Vars
  ${EndIf}

  ${If} $3 == HKLM
    ReadRegStr $5 'HKLM "SYSTEM\CurrentControlSet\Control\Session Manager\Environment"' $1
  ${ElseIf} $3 == HKCU
    ReadRegStr $5 'HKCU "Environment"' $1
  ${Else}
    SetErrors
    DetailPrint 'ERROR: RegLoc must be "HKLM" or "HKCU"'
    Goto EnvVarUpdate_Restore_Vars
  ${EndIf}

  ${If} $4 == ""
    SetErrors
    DetailPrint "ERROR: PathString is blank"
    Goto EnvVarUpdate_Restore_Vars
  ${EndIf}

  Push $6
  Push $7
  Push $8
  StrLen $7 $4
  StrLen $6 $5
  IntOp $8 $6 + $7
  ${If} $5 == "" ${OrIf} $8 >= ${NSIS_MAX_STRLEN}
    SetErrors
    DetailPrint "Current \$1 length (\$6) too long to modify; set manually if needed"
    Pop $8
    Pop $7
    Pop $6
    Goto EnvVarUpdate_Restore_Vars
  ${EndIf}
  Pop $8
  Pop $7
  Pop $6

  ; Se variável estiver vazia e ação for remover
  ${If} $5 == "" ${AndIf} $2 == "R"
    SetErrors
    DetailPrint "$1 is empty – nothing to remove"
    Goto EnvVarUpdate_Restore_Vars
  ${EndIf}

  ; Atualizando valor — limpeza de formatação
  StrCpy $0 $5
  ${If} $0 == ""
    StrCpy $0 $4
  ${ElseIf} $2 == "A"
    StrCpy $0 "$0;$4"
  ${ElseIf} $2 == "P"
    StrCpy $0 "$4;$0"
  ${Else}
    ; Ação = R, remove string
    ; Lógica de tokenização e remoção como no exemplo completo
    ... (estrutura de remoção aqui) ...
  ${EndIf}

  ; Grava de volta no registro
  ${If} $3 == HKLM
    WriteRegExpandStr 'HKLM "SYSTEM\CurrentControlSet\Control\Session Manager\Environment"' $1 $0
  ${ElseIf} $3 == HKCU
    WriteRegExpandStr 'HKCU "Environment"' $1 $0
  ${EndIf}

  SendMessage ${HWND_BROADCAST} ${WM_WININICHANGE} 0 "STR:Environment" /TIMEOUT=5000

EnvVarUpdate_Restore_Vars:
  ... ; código de restauração do stack
FunctionEnd

EnvVarUpdate UN
!insertmacro EnvVarUpdate ""
!insertmacro EnvVarUpdate "un."
!verbose pop
!endif
