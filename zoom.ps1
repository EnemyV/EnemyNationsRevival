# zoom.ps1 — send mouse-wheel zoom to the running Enemy Nations window.
#
# The game zooms toward the wheel position; this aims at the target window's
# CENTRE so the view stays put (off-centre wheels edge-scroll the map). Uses
# PostMessage (no focus needed), same target resolution as click.ps1.
#
# Usage:
#   ./zoom.ps1 -Out 4               # zoom OUT 4 wheel notches
#   ./zoom.ps1 -In 3                # zoom IN 3 notches
#   ./zoom.ps1 -Out 8 -Window map   # target the Area Map child explicitly
#
# Window roles: main/menu, map/area, radar, ... (see click.ps1). Default: map.
# Exit codes: 0 ok, 1 game not running / window not found.

[CmdletBinding()]
param(
    [int]$Out = 0,
    [int]$In = 0,
    [int]$Wiggle = 0,      # alternate out/in N times in THIS process (true cadence — repeated
                           # single-notch invocations pay ~1s process startup each and can't
                           # hold the 120ms gesture-preview window open)
    [string]$Window = 'map',
    [int]$Hwnd,
    [int]$DelayMs = 120
)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'harness-common.ps1')

$info       = Resolve-GameTarget -Hwnd $Hwnd -Window $Window
$hwndTarget = $info.Hwnd

$notches = if ($Wiggle -gt 0) { $Wiggle } elseif ($In -gt 0) { $In } else { $Out }
if ($notches -le 0) { Write-Warning "Specify -Out N, -In N or -Wiggle N (notches)"; exit 0 }
$delta   = if ($In -gt 0) { 120 } else { -120 }   # +in / -out, one notch = 120

# WM_MOUSEWHEEL lParam is SCREEN coords; aim at the window centre.
$WM_MOUSEWHEEL = 0x020A
$sp = New-Object GameWin32+POINT
$sp.X = [int]($info.CW / 2); $sp.Y = [int]($info.CH / 2)
[void][GameWin32]::ClientToScreen($hwndTarget, [ref]$sp)
$lp = [IntPtr](($sp.Y -shl 16) -bor ($sp.X -band 0xFFFF))

for ($i = 0; $i -lt $notches; $i++) {
    # Wiggle alternates in TRIPLES (out,out,out,in,in,in,...): the game's wheel handler
    # (CWndArea::OnMouseWheel) accumulates and triggers a zoom per 2*WHEEL_DELTA. Pairs
    # are NOT enough — leftover accumulator residue (+/-120 from any odd notch history)
    # phase-shifts a pair into oscillating between -120/+120 without ever triggering.
    # Three same-direction notches guarantee a trigger from ANY residue.
    if ($Wiggle -gt 0) { $delta = if ([math]::Floor($i / 3) % 2) { 120 } else { -120 } }
    [void][GameWin32]::PostMessage($hwndTarget, $WM_MOUSEWHEEL, [IntPtr]($delta -shl 16), $lp)
    Start-Sleep -Milliseconds $DelayMs
}

Write-Output ("Sent {0} wheel notch(es) [{1}] to '{2}' [HWND {3}]" -f `
    $notches, $(if ($Wiggle -gt 0) { 'wiggle' } elseif ($In -gt 0) { 'in' } else { 'out' }), $info.Title, $hwndTarget.ToInt64())
exit 0
