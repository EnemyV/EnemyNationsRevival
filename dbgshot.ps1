# dbgshot.ps1 — token-cheap screenshot wrapper around screenshot.ps1.
#
# Thin alias that calls screenshot.ps1 with low-cost defaults (33% scale, JPEG
# q70). All the real work — window-role targeting, scaling, cropping, composite —
# lives in screenshot.ps1 / harness-common.ps1.
#
# Usage:
#   ./dbgshot.ps1                          # auto window, 33% JPEG -> d:\tmp\dbgshot.jpg
#   ./dbgshot.ps1 -Window map              # in-game Area Map child
#   ./dbgshot.ps1 -Scale 50                # bump quality
#   ./dbgshot.ps1 -Composite               # full stitched view
#   ./dbgshot.ps1 -Region 100,50,400,300   # crop (client px, pre-scale)
#   ./dbgshot.ps1 -Gray                    # grayscale (smaller still)
#   ./dbgshot.ps1 -Hwnd 12345 -Out d:\tmp\menu.jpg

[CmdletBinding()]
param(
    [string]$Out = 'd:\tmp\dbgshot.jpg',
    [string]$Window,
    [int]$Hwnd,
    [string]$ProcessName,
    [int]$Scale = 33,
    [int]$Jpeg = 70,
    [string]$Region,
    [switch]$Gray,
    [switch]$Composite,
    [switch]$Screen
)

$ErrorActionPreference = 'Stop'
$fwd = @{ Out = $Out; Scale = $Scale; Jpeg = $Jpeg }
if ($Window)     { $fwd.Window = $Window }
if ($Hwnd -gt 0) { $fwd.Hwnd = $Hwnd }
if ($ProcessName){ $fwd.ProcessName = $ProcessName }
if ($Region)     { $fwd.Region = $Region }
if ($Gray)       { $fwd.Gray = $true }
if ($Composite)  { $fwd.Composite = $true }
if ($Screen)     { $fwd.Screen = $true }

& (Join-Path $PSScriptRoot 'screenshot.ps1') @fwd
exit $LASTEXITCODE
