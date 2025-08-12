# Copyright (C) 2025 Pedro Henrique / phkaiser13
# SPDX-License-Identifier: Apache-2.0
#
# This script is executed by Chocolatey when a user runs `choco install gitph`.
# Its job is to download the native .exe installer from your GitHub Release
# and run it silently.

$ErrorActionPreference = 'Stop'

# --- Package Parameters ---
# TODO: Update this for each new release.
$version = 'v1.0.0' 
$installerUrl = "https://github.com/phkaiser13/peitchgit/releases/download/$version/gitph_installer_$version.exe"
$checksum = '' # Optional: You can add an SHA256 checksum for the installer here.
$checksumType = 'sha256'

$packageArgs = @{
  packageName    = $env:ChocolateyPackageName
  unzipLocation  = $toolsDir
  fileType       = 'exe'
  url            = $installerUrl
  
  # --- Silent arguments for your NSIS installer ---
  # These are passed to the .exe to make it run without any user interaction.
  # /S runs the installer silently.
  # /D specifies the installation directory (optional, but good practice).
  silentArgs     = "/S /D=$toolsDir"

  # NSIS installers typically return 0 for success.
  validExitCodes = @(0)
  
  checksum       = $checksum
  checksumType   = $checksumType
}

# Download and run the installer.
# This Chocolatey helper function handles the download, checksum verification,
# and execution of the installer with the silent arguments.
Install-ChocolateyPackage @packageArgs
