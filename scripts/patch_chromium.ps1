<#
patch_chromium.ps1

Creates a placeholder patch directory and copies BMS-specific stubs into Chromium tree.
This is a helper to track changes; applying real patches to Chromium requires careful manual review.
#>
param(
    [string]$ChromiumRoot = 'build/dependencies/chromuim',
    [string]$PatchDir = 'patches'
)

$root = Split-Path -Parent $PSScriptRoot
$fullChromium = Join-Path $root $ChromiumRoot
$fullPatch = Join-Path $root $PatchDir

New-Item -ItemType Directory -Force -Path $fullPatch | Out-Null

# Create a placeholder patch file
$patchFile = Join-Path $fullPatch 'bms_changes.patch'
Set-Content -Path $patchFile -Value "# Placeholder patch for converting Chromium into BMS\n# Review and implement carefully.\n" -Force

Write-Host "Created placeholder patch at $patchFile"
if (Test-Path $fullChromium) {
    Write-Host "Chromium tree detected at $fullChromium"
    Write-Host "Copying placeholder files (no destructive changes)."
    Copy-Item -Path $patchFile -Destination $fullChromium -Force
    Write-Host "Patch placeholder copied to Chromium tree. Inspect and apply manually."
} else {
    Write-Host "Chromium tree not found at $fullChromium — run install_chromium.ps1 first."
}
