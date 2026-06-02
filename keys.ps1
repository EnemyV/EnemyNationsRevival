# keys.ps1 — send keystrokes to the running Enemy Nations window.
#
# By default the target auto-resolves (in-game: Area Map child; menu/dialog: the
# active SDL window). Override with -Window when a specific window needs focus
# (e.g. -Window pick to type into the Pick-Your-Player dialog).
#
# Usage:
#   ./keys.ps1 -Key Enter                  # press named key once (auto target)
#   ./keys.ps1 -Window map -Key Down -Times 3
#   ./keys.ps1 -Text "Hello"               # type literal text
#   ./keys.ps1 -ListKeys                   # show the named-key table
#   ./keys.ps1 -Key Enter -Hwnd 12345      # target specific HWND
#
# Window roles: main/menu, map/area, radar, vehicles, buildings, research,
# pick/player — or any substring of a window title.
#
# Notes: uses PostMessage (no focus needed). For text, sends WM_KEYDOWN +
# WM_CHAR + WM_KEYUP per char. Modifier combos (Ctrl/Shift/Alt+key) NOT supported.
#
# Exit codes: 0 ok, 1 game not running / window not found, 2 send failed.

[CmdletBinding()]
param(
    [string]$Key,
    [string]$Text,
    [int]$Times = 1,
    [int]$DelayMs = 30,
    [string]$Window,
    [string]$ProcessName,
    [int]$Hwnd,
    [switch]$ListKeys
)

$ErrorActionPreference = 'Stop'

$VKMap = @{
    'Enter'     = 0x0D; 'Return'    = 0x0D
    'Escape'    = 0x1B; 'Esc'       = 0x1B
    'Tab'       = 0x09
    'Space'     = 0x20
    'Backspace' = 0x08; 'BS'        = 0x08
    'Delete'    = 0x2E; 'Del'       = 0x2E
    'Insert'    = 0x2D; 'Ins'       = 0x2D
    'Up'        = 0x26; 'Down'      = 0x28
    'Left'      = 0x25; 'Right'     = 0x27
    'Home'      = 0x24; 'End'       = 0x23
    'PageUp'    = 0x21; 'PageDown'  = 0x22
    'F1'  = 0x70; 'F2'  = 0x71; 'F3'  = 0x72; 'F4'  = 0x73
    'F5'  = 0x74; 'F6'  = 0x75; 'F7'  = 0x76; 'F8'  = 0x77
    'F9'  = 0x78; 'F10' = 0x79; 'F11' = 0x7A; 'F12' = 0x7B
}
foreach ($c in [char[]]'ABCDEFGHIJKLMNOPQRSTUVWXYZ') { $VKMap[$c.ToString()] = [int][char]$c }
foreach ($c in [char[]]'0123456789') { $VKMap[$c.ToString()] = [int][char]$c }

if ($ListKeys) {
    Write-Output 'Named keys recognized by -Key:'
    $VKMap.GetEnumerator() | Sort-Object Name | ForEach-Object {
        '  {0,-12} VK 0x{1:X2}' -f $_.Name, $_.Value
    }
    exit 0
}

if (-not $Key -and -not $Text) {
    Write-Error 'Pass -Key <name> or -Text "..." (or -ListKeys to see options)'
    exit 2
}

. (Join-Path $PSScriptRoot 'harness-common.ps1')

$info = Resolve-GameTarget -Hwnd $Hwnd -Window $Window -ProcessName $ProcessName
$hwndTarget = $info.Hwnd

$WM_KEYDOWN = 0x0100
$WM_KEYUP   = 0x0101
$WM_CHAR    = 0x0102

function Send-NamedKey {
    param([IntPtr]$Hwnd, [string]$Name, [int]$DelayMs)
    if (-not $VKMap.ContainsKey($Name)) {
        Write-Error "Unknown key: '$Name'. Run ./keys.ps1 -ListKeys for the table."
        return $false
    }
    $vk = [IntPtr]$VKMap[$Name]
    [void][GameWin32]::PostMessage($Hwnd, $WM_KEYDOWN, $vk, [IntPtr]0)
    Start-Sleep -Milliseconds $DelayMs
    [void][GameWin32]::PostMessage($Hwnd, $WM_KEYUP, $vk, [IntPtr]0)
    return $true
}

function Send-TextChar {
    param([IntPtr]$Hwnd, [char]$Ch, [int]$DelayMs)
    $upper = [char]::ToUpper($Ch)
    $vk = [IntPtr][int]$upper
    [void][GameWin32]::PostMessage($Hwnd, $WM_KEYDOWN, $vk, [IntPtr]0)
    [void][GameWin32]::PostMessage($Hwnd, $WM_CHAR, [IntPtr][int]$Ch, [IntPtr]0)
    Start-Sleep -Milliseconds $DelayMs
    [void][GameWin32]::PostMessage($Hwnd, $WM_KEYUP, $vk, [IntPtr]0)
}

for ($i = 0; $i -lt $Times; $i++) {
    if ($Key) {
        $ok = Send-NamedKey -Hwnd $hwndTarget -Name $Key -DelayMs $DelayMs
        if (-not $ok) { exit 2 }
    }
    if ($Text) {
        foreach ($c in $Text.ToCharArray()) {
            Send-TextChar -Hwnd $hwndTarget -Ch $c -DelayMs $DelayMs
        }
    }
    if ($i -lt ($Times - 1)) { Start-Sleep -Milliseconds $DelayMs }
}

$summary = @()
if ($Key)  { $summary += "Key=$Key" }
if ($Text) { $summary += ("Text=`"{0}`"" -f $Text) }
if ($Times -gt 1) { $summary += "Times=$Times" }
Write-Output ("Sent {0} to '{1}' [HWND {2}]" -f ($summary -join ' '), $info.Title, $hwndTarget.ToInt64())
exit 0
