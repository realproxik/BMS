<#
install_chromium.ps1

Downloads a Chromium snapshot for Windows and extracts it into build/dependencies/chromuim.
Usage:
  powershell -ExecutionPolicy Bypass -File scripts\install_chromium.ps1
  powershell -ExecutionPolicy Bypass -File scripts\install_chromium.ps1 -Force
#>

param(
    [string]$TargetDir = 'build/dependencies/chromuim',
    [switch]$Force
)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$fullTarget = Join-Path $root $TargetDir
New-Item -ItemType Directory -Force -Path $fullTarget | Out-Null

Write-Host "Fetching latest Chromium revision (Windows)..."
$lastUrl = 'https://www.googleapis.com/download/storage/v1/b/chromium-browser-snapshots/o/Win%2FLAST_CHANGE?alt=media'
$rev = (Invoke-RestMethod -Uri $lastUrl -UseBasicParsing).Trim()
Write-Host "Latest revision: $rev"

$downloadUrl = "https://www.googleapis.com/download/storage/v1/b/chromium-browser-snapshots/o/Win%2F$rev%2Fchrome-win.zip?alt=media"
$zipPath = Join-Path $fullTarget 'chrome-win.zip'
if (-Not (Test-Path $zipPath) -or $Force) {
    Write-Host "Downloading Chromium revision $rev..."
    Invoke-WebRequest -Uri $downloadUrl -OutFile $zipPath -UseBasicParsing
} else {
    Write-Host "Found existing $zipPath (use -Force to re-download)."
}

Write-Host "Extracting to $fullTarget..."
Expand-Archive -LiteralPath $zipPath -DestinationPath $fullTarget -Force

Write-Host "Chromium extracted to $fullTarget"
