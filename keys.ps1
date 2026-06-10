# keys.ps1 — send keystrokes to the running Enemy Nations window.
#
# By default the target auto-resolves (in-game: Area Map child; menu/dialog: the
# active SDL window). Override with -Window when a specific window needs focus
# (e.g. -Window pick to type into the Pick-Your-Player dialog).
#
# Usage:
#   ./keys.ps1 -Key Enter                  # press named key once (auto target)
#   ./keys.ps1 -Window map -Key Down -Times 3
#   ./keys.ps1 -Key "." -Window map        # in-game hotkey (rotate view CW)
#   ./keys.ps1 -Key "[" -Window map        # rotate a building being placed (CCW)
#   ./keys.ps1 -Key A -Ctrl                # Ctrl+A (modifier combo)
#   ./keys.ps1 -Text "Hello"               # type literal text
#   ./keys.ps1 -ListKeys                   # show the named-key table
#   ./keys.ps1 -Key Enter -Hwnd 12345      # target specific HWND
#
# Window roles: main/menu, map/area, radar, vehicles, buildings, research,
# pick/player — or any substring of a window title.
#
# Named keys include letters, digits, F-keys, nav keys AND OEM punctuation by the
# symbol itself: , . [ ] = - ; / ' \ ` (the in-game rotate/zoom hotkeys). SDL maps
# keydowns by hardware scancode, so the scancode is injected into lParam.
#
# Notes: uses PostMessage (no focus needed). For text, sends WM_KEYDOWN +
# WM_CHAR + WM_KEYUP per char. -Ctrl/-Shift/-Alt hold the modifier around the key.
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
    [switch]$Ctrl,
    [switch]$Shift,
    [switch]$Alt,
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
    # OEM punctuation — the in-game hotkeys use these (view rotate , / . and
    # building-placement rotate [ / ]; zoom = / -). Accept the symbol itself and
    # a descriptive name. SDL maps keydowns by scancode, so Send-NamedKey injects
    # the hardware scancode into lParam (below).
    ','  = 0xBC; 'Comma'        = 0xBC
    '.'  = 0xBE; 'Period'       = 0xBE
    '['  = 0xDB; 'LeftBracket'  = 0xDB
    ']'  = 0xDD; 'RightBracket' = 0xDD
    '='  = 0xBB; 'Plus'         = 0xBB; 'Equals' = 0xBB
    '-'  = 0xBD; 'Minus'        = 0xBD
    ';'  = 0xBA; '/'            = 0xBF; "'" = 0xDE; '\' = 0xDC; '`' = 0xC0
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
    $vkNum = $VKMap[$Name]
    $vk = [IntPtr]$vkNum
    # SDL's Windows backend maps WM_KEYDOWN to an SDL scancode using the hardware
    # scancode in lParam bits 16-23 — NOT the VK. With lParam=0 the scancode is 0
    # (SDL_SCANCODE_UNKNOWN) and game hotkeys that switch on keysym.scancode never
    # fire. Inject the scancode so OEM keys (, . [ ]) and the rest resolve correctly.
    $sc = [GameWin32]::MapVirtualKey([uint32]$vkNum, 0)   # MAPVK_VK_TO_VSC
    # Nav keys (arrows/Ins/Del/Home/End/PgUp/PgDn) are EXTENDED keys: without lParam
    # bit 24 SDL decodes their scancode as the NUMPAD equivalent (KP_4 not LEFT) and
    # the game's arrow-key pan never fires.
    $extVKs = @(0x25,0x26,0x27,0x28,0x2D,0x2E,0x24,0x23,0x21,0x22)
    $ext = if ($extVKs -contains $vkNum) { 0x01000000 } else { 0 }
    $lpDown = [IntPtr](([int64]$sc -shl 16) -bor $ext -bor 0x00000001)
    $lpUp   = [IntPtr](([int64]$sc -shl 16) -bor $ext -bor 0xC0000001)  # bits 30,31 = up transition
    [void][GameWin32]::PostMessage($Hwnd, $WM_KEYDOWN, $vk, $lpDown)
    Start-Sleep -Milliseconds $DelayMs
    [void][GameWin32]::PostMessage($Hwnd, $WM_KEYUP, $vk, $lpUp)
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

# Hold/release modifier keys (Ctrl/Shift/Alt) around the keystroke so combos like
# Ctrl+A reach SDL (which tracks modifier state from the WM_KEYDOWN/UP stream).
# Each entry: VK, hardware scancode.
$mods = @()
if ($Ctrl)  { $mods += ,@(0x11, 0x1D) }
if ($Shift) { $mods += ,@(0x10, 0x2A) }
if ($Alt)   { $mods += ,@(0x12, 0x38) }
function Push-Mods {
    foreach ($m in $mods) {
        $lp = [IntPtr](([int64]$m[1] -shl 16) -bor 0x00000001)
        [void][GameWin32]::PostMessage($hwndTarget, $WM_KEYDOWN, [IntPtr]$m[0], $lp)
    }
    if ($mods.Count) { Start-Sleep -Milliseconds $DelayMs }
}
function Pop-Mods {
    for ($k = $mods.Count - 1; $k -ge 0; $k--) {
        $m = $mods[$k]
        $lp = [IntPtr](([int64]$m[1] -shl 16) -bor 0xC0000001)
        [void][GameWin32]::PostMessage($hwndTarget, $WM_KEYUP, [IntPtr]$m[0], $lp)
    }
}

for ($i = 0; $i -lt $Times; $i++) {
    if ($Key) {
        Push-Mods
        $ok = Send-NamedKey -Hwnd $hwndTarget -Name $Key -DelayMs $DelayMs
        Pop-Mods
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
