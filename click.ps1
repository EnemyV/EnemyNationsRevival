# click.ps1 — send a mouse click to the running Enemy Nations window.
#
# Coordinates are CLIENT coords of the TARGET window. Take a screenshot of the
# same window first (./screenshot.ps1 -Window <role>) so the coords match.
#
# By default the target auto-resolves: in-game it's the "Area Map" child window
# (where map clicks actually land — events route by SDL windowID); on the menu /
# a dialog it's the single active SDL window. Override with -Window.
#
# Usage:
#   ./click.ps1 -X 320 -Y 240                # auto target, left click
#   ./click.ps1 -Window map -X 800 -Y 600    # click the Area Map child
#   ./click.ps1 -Window radar -X 100 -Y 100  # click the radar window
#   ./click.ps1 -X 100 -Y 50 -Right          # right click
#   ./click.ps1 -X 100 -Y 50 -Right -Ctrl    # Ctrl+RMB (e.g. rotate a building being placed)
#   ./click.ps1 -X 100 -Y 50 -Shift          # Shift+click (e.g. add to selection)
#   ./click.ps1 -X 100 -Y 50 -Double         # double-click
#   ./click.ps1 -X 100 -Y 50 -NoMove         # skip the WM_MOUSEMOVE before click
#   ./click.ps1 -X 100 -Y 50 -Hwnd 12345     # target a specific HWND
#
# Drags (hold the button from X,Y to ToX,ToY with interpolated moves):
#   ./click.ps1 -Window map -X 600 -Y 400 -ToX 900 -ToY 600           # LMB drag = selection box
#   ./click.ps1 -Window map -X 600 -Y 400 -ToX 900 -ToY 600 -Right    # RMB drag = line move (2+ units)
#   ./click.ps1 -Window map -X 800 -Y 500 -ToX 500 -ToY 500 -Middle   # MMB drag = pan the map
#
# Window roles: main/menu, map/area, radar, vehicles, buildings, research,
# pick/player — or any substring of a window title.
#
# Notes: uses PostMessage, so the window does NOT need focus. SDL reads the
# WM_*BUTTON* / WM_MOUSEMOVE messages into its event queue.
#
# Exit codes: 0 ok, 1 game not running / window not found, 2 click failed.

[CmdletBinding()]
param(
    [Parameter(Mandatory)] [int]$X,
    [Parameter(Mandatory)] [int]$Y,
    [string]$Window,
    [string]$ProcessName,
    [int]$Hwnd,
    [switch]$Right,
    [switch]$Middle,
    [switch]$Double,
    [switch]$Ctrl,
    [switch]$Shift,
    [switch]$NoMove,
    [switch]$NoCursor,
    [int]$DelayMs = 40,
    # Drag: hold the button from (X,Y) to (ToX,ToY) with interpolated mouse moves.
    [int]$ToX = [int]::MinValue,
    [int]$ToY = [int]::MinValue,
    [int]$Steps = 12,
    [int]$StepDelayMs = 20
)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'harness-common.ps1')

$info = Resolve-GameTarget -Hwnd $Hwnd -Window $Window -ProcessName $ProcessName
$hwndTarget = $info.Hwnd

# Warn (don't fail) if the point is outside the target's client area — a common
# cause of "the click did nothing".
if ($X -lt 0 -or $Y -lt 0 -or $X -ge $info.CW -or $Y -ge $info.CH) {
    Write-Warning ("({0},{1}) is outside '{2}' client area {3}x{4}" -f $X, $Y, $info.Title, $info.CW, $info.CH)
}

$WM_MOUSEMOVE   = 0x0200
$WM_LBUTTONDOWN = 0x0201; $WM_LBUTTONUP = 0x0202
$WM_RBUTTONDOWN = 0x0204; $WM_RBUTTONUP = 0x0205
$WM_MBUTTONDOWN = 0x0207; $WM_MBUTTONUP = 0x0208
$MK_LBUTTON = 0x0001; $MK_RBUTTON = 0x0002; $MK_MBUTTON = 0x0010
$MK_SHIFT = 0x0004; $MK_CONTROL = 0x0008

if ($Right) {
    $downMsg = $WM_RBUTTONDOWN; $upMsg = $WM_RBUTTONUP; $mkButton = $MK_RBUTTON; $btnName = 'right'
} elseif ($Middle) {
    $downMsg = $WM_MBUTTONDOWN; $upMsg = $WM_MBUTTONUP; $mkButton = $MK_MBUTTON; $btnName = 'middle'
} else {
    $downMsg = $WM_LBUTTONDOWN; $upMsg = $WM_LBUTTONUP; $mkButton = $MK_LBUTTON; $btnName = 'left'
}

# Modifier bits OR'd into every mouse message's wParam — SDL reads MK_CONTROL/
# MK_SHIFT from the button message (e.g. Ctrl+RMB rotates a building being placed).
$mkMods = 0
if ($Ctrl)  { $mkMods = $mkMods -bor $MK_CONTROL }
if ($Shift) { $mkMods = $mkMods -bor $MK_SHIFT }
$btnName = (@(if($Ctrl){'Ctrl'}; if($Shift){'Shift'}) + $btnName) -join '+'

# Pack X (low word) and Y (high word) into LPARAM
$lparam = [IntPtr](($Y -shl 16) -bor ($X -band 0xFFFF))

# Physically place the OS cursor at the target. SDL derives a button event's
# position from the last known mouse position, not the message's lParam — so if
# the real cursor sits elsewhere (screen unlocked), the click lands there instead
# of our target. Moving the real cursor first makes SDL agree. Skip with -NoCursor.
if (-not $NoCursor) {
    $sp = New-Object GameWin32+POINT; $sp.X = $X; $sp.Y = $Y
    [void][GameWin32]::ClientToScreen($hwndTarget, [ref]$sp)
    [void][GameWin32]::SetCursorPos($sp.X, $sp.Y)
    Start-Sleep -Milliseconds $DelayMs
}

if (-not $NoMove) {
    [void][GameWin32]::PostMessage($hwndTarget, $WM_MOUSEMOVE, [IntPtr]$mkMods, $lparam)
    Start-Sleep -Milliseconds $DelayMs
}

$isDrag = ($ToX -ne [int]::MinValue) -and ($ToY -ne [int]::MinValue)

if ($isDrag) {
    # press at the start point, walk interpolated WM_MOUSEMOVEs (button bit held
    # in wParam) to the end point, release there. The physical cursor is moved
    # along too — SDL takes a button event's position from the live cursor.
    [void][GameWin32]::PostMessage($hwndTarget, $downMsg, [IntPtr]($mkButton -bor $mkMods), $lparam)
    Start-Sleep -Milliseconds $DelayMs

    for ($i = 1; $i -le $Steps; $i++) {
        $px = [int]($X + ($ToX - $X) * $i / $Steps)
        $py = [int]($Y + ($ToY - $Y) * $i / $Steps)
        if (-not $NoCursor) {
            $dp = New-Object GameWin32+POINT; $dp.X = $px; $dp.Y = $py
            [void][GameWin32]::ClientToScreen($hwndTarget, [ref]$dp)
            [void][GameWin32]::SetCursorPos($dp.X, $dp.Y)
        }
        $lpStep = [IntPtr](($py -shl 16) -bor ($px -band 0xFFFF))
        [void][GameWin32]::PostMessage($hwndTarget, $WM_MOUSEMOVE, [IntPtr]($mkButton -bor $mkMods), $lpStep)
        Start-Sleep -Milliseconds $StepDelayMs
    }

    $lpEnd = [IntPtr](($ToY -shl 16) -bor ($ToX -band 0xFFFF))
    [void][GameWin32]::PostMessage($hwndTarget, $upMsg, [IntPtr]$mkMods, $lpEnd)

    Write-Output ("Sent {0} drag ({1},{2})->({3},{4}) to '{5}' [HWND {6}]" -f `
        $btnName, $X, $Y, $ToX, $ToY, $info.Title, $hwndTarget.ToInt64())
    exit 0
}

[void][GameWin32]::PostMessage($hwndTarget, $downMsg, [IntPtr]($mkButton -bor $mkMods), $lparam)
Start-Sleep -Milliseconds $DelayMs
[void][GameWin32]::PostMessage($hwndTarget, $upMsg, [IntPtr]$mkMods, $lparam)

if ($Double) {
    Start-Sleep -Milliseconds $DelayMs
    [void][GameWin32]::PostMessage($hwndTarget, $downMsg, [IntPtr]($mkButton -bor $mkMods), $lparam)
    Start-Sleep -Milliseconds $DelayMs
    [void][GameWin32]::PostMessage($hwndTarget, $upMsg, [IntPtr]$mkMods, $lparam)
}

$kind = if ($Double) { "$btnName double-click" } else { "$btnName click" }
Write-Output ("Sent {0} at ({1},{2}) to '{3}' [HWND {4}]" -f $kind, $X, $Y, $info.Title, $hwndTarget.ToInt64())
exit 0
