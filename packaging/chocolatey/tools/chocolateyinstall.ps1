# Copyright (C) 2025 Pedro Henrique / phkaiser13
# SPDX-License-Identifier: Apache-2.0

$ErrorActionPreference = 'Stop'

# --- Parâmetros do Pacote ---
# Estas variáveis serão substituídas pelo seu workflow de CI/CD
$version = '${GITPH_VERSION}' # Ex: 1.0.0
$installerUrl = "https://github.com/phkaiser13/peitchgit/releases/download/v$($version)/gitph-${version}-installer.exe"
$checksum = '${CHECKSUM_SHA256}' # O workflow irá injetar o hash SHA256 do instalador aqui
$checksumType = 'sha256'
$toolsDir = "$(Get-ToolsLocation)"

$packageArgs = @{
  packageName    = $env:ChocolateyPackageName
  fileType       = 'exe'
  url            = $installerUrl
  
  # --- Argumentos silenciosos para o seu instalador NSIS ---
  silentArgs     = "/S /D=`"$toolsDir`""

  validExitCodes = @(0)
  
  checksum       = $checksum
  checksumType   = $checksumType
}

# Baixa e executa o instalador
Install-ChocolateyPackage @packageArgs
