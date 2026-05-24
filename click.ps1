# click.ps1 — send a mouse click to the running Enemy Nations window.
#
# Coordinates are CLIENT coords (relative to the window's content area, not screen).
# Take a screenshot first with ./screenshot.ps1 to see what coords to use — the
# screenshot uses the same client-area coordinate space.
#
# Usage:
#   ./click.ps1 -X 320 -Y 240               # left click at (320,240)
#   ./click.ps1 -X 100 -Y 50 -Right         # right click
#   ./click.ps1 -X 100 -Y 50 -Double        # double-click
#   ./click.ps1 -X 100 -Y 50 -NoMove        # skip the WM_MOUSEMOVE before click
#   ./click.ps1 -ProcessName foo            # override (default: auto enations/enations_gate)
#   ./click.ps1 -X 100 -Y 50 -Hwnd 12345    # target specific HWND
#
# Notes:
#   - Uses PostMessage, so the game window does NOT need focus.
#   - SDL2 reads WM_LBUTTONDOWN/UP/etc. into its event queue, so clicks land
#     as normal SDL_MOUSEBUTTONDOWN/UP events.
#
# Exit codes: 0 ok, 1 game not running, 2 click failed.

[CmdletBinding()]
param(
    [Parameter(Mandatory)] [int]$X,
    [Parameter(Mandatory)] [int]$Y,
    [string]$ProcessName,
    [int]$Hwnd,
    [switch]$Right,
    [switch]$Middle,
    [switch]$Double,
    [switch]$NoMove,
    [int]$DelayMs = 40
)

$ErrorActionPreference = 'Stop'

if (-not ('GameWin32' -as [type])) {
    Add-Type @'
using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Text;

public class GameWindowInfo {
    public IntPtr Hwnd; public string Title; public string Class;
    public int Width; public int Height; public int Area;
}

public static class GameWin32 {
    [DllImport("user32.dll")] public static extern bool PostMessage(IntPtr hWnd, uint Msg, IntPtr wParam, IntPtr lParam);
    [DllImport("user32.dll")] public static extern bool EnumWindows(EnumWindowsProc enumProc, IntPtr lParam);
    [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr hWnd, out uint lpdwProcessId);
    [DllImport("user32.dll")] public static extern bool IsWindowVisible(IntPtr hWnd);
    [DllImport("user32.dll")] public static extern bool GetClientRect(IntPtr hWnd, out RECT lpRect);
    [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr hWnd, out RECT lpRect);
    [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr hWnd, IntPtr hdcBlt, uint nFlags);
    [DllImport("user32.dll")] public static extern bool ClientToScreen(IntPtr hWnd, ref POINT lpPoint);
    [DllImport("user32.dll", CharSet = CharSet.Auto)] public static extern int GetWindowText(IntPtr hWnd, StringBuilder lpString, int nMaxCount);
    [DllImport("user32.dll", CharSet = CharSet.Auto)] public static extern int GetClassName(IntPtr hWnd, StringBuilder lpClassName, int nMaxCount);
    public delegate bool EnumWindowsProc(IntPtr hWnd, IntPtr lParam);
    [StructLayout(LayoutKind.Sequential)] public struct RECT { public int Left, Top, Right, Bottom; }
    [StructLayout(LayoutKind.Sequential)] public struct POINT { public int X, Y; }

    public static List<GameWindowInfo> EnumProcessWindows(uint targetPid) {
        var results = new List<GameWindowInfo>();
        EnumWindows(delegate(IntPtr hwnd, IntPtr lparam) {
            uint wpid; GetWindowThreadProcessId(hwnd, out wpid);
            if (wpid == targetPid && IsWindowVisible(hwnd)) {
                RECT r; GetClientRect(hwnd, out r);
                var title = new StringBuilder(256); GetWindowText(hwnd, title, 256);
                var cls = new StringBuilder(256); GetClassName(hwnd, cls, 256);
                int w = r.Right - r.Left; int h = r.Bottom - r.Top;
                results.Add(new GameWindowInfo {
                    Hwnd = hwnd, Title = title.ToString(), Class = cls.ToString(),
                    Width = w, Height = h, Area = w * h
                });
            }
            return true;
        }, IntPtr.Zero);
        return results;
    }
}
'@
}

function Find-GameProcess {
    param([string]$ExplicitName)
    if ($ExplicitName) {
        return (Get-Process $ExplicitName -ErrorAction SilentlyContinue | Select-Object -First 1)
    }
    foreach ($name in @('enations', 'enations_gate')) {
        $p = Get-Process $name -ErrorAction SilentlyContinue | Select-Object -First 1
        if ($p) { return $p }
    }
    return $null
}

function Resolve-GameHwnd {
    param([int]$HwndOverride, [string]$ProcessName)
    if ($HwndOverride -gt 0) { return [IntPtr]$HwndOverride }
    $proc = Find-GameProcess -ExplicitName $ProcessName
    if (-not $proc) {
        $hint = if ($ProcessName) { "'$ProcessName'" } else { "'enations' or 'enations_gate'" }
        Write-Error "No process found matching $hint. Is the game running?"
        exit 1
    }
    $windows = [GameWin32]::EnumProcessWindows([uint32]$proc.Id) | Sort-Object Area -Descending
    # Prefer the SDL_app window — it's where SDL2 translates Win32 messages into
    # SDL events. The MFC stub window (EnemyNationsMainWindow) sometimes covers
    # the SDL window at the same size, but PostMessages to it are dropped.
    $sdl = $windows | Where-Object { $_.Class -eq 'SDL_app' } | Select-Object -First 1
    if ($sdl) { return $sdl.Hwnd }
    if ($windows.Count -gt 0) { return $windows[0].Hwnd }
    if ($proc.MainWindowHandle -ne [IntPtr]::Zero) { return $proc.MainWindowHandle }
    Write-Error "Process '$($proc.ProcessName)' has no visible windows."
    exit 1
}

$hwndTarget = Resolve-GameHwnd -HwndOverride $Hwnd -ProcessName $ProcessName

# Win32 message constants
$WM_MOUSEMOVE    = 0x0200
$WM_LBUTTONDOWN  = 0x0201
$WM_LBUTTONUP    = 0x0202
$WM_RBUTTONDOWN  = 0x0204
$WM_RBUTTONUP    = 0x0205
$WM_MBUTTONDOWN  = 0x0207
$WM_MBUTTONUP    = 0x0208
$MK_LBUTTON      = 0x0001
$MK_RBUTTON      = 0x0002
$MK_MBUTTON      = 0x0010

if ($Right) {
    $downMsg = $WM_RBUTTONDOWN; $upMsg = $WM_RBUTTONUP; $mkButton = $MK_RBUTTON; $btnName = 'right'
} elseif ($Middle) {
    $downMsg = $WM_MBUTTONDOWN; $upMsg = $WM_MBUTTONUP; $mkButton = $MK_MBUTTON; $btnName = 'middle'
} else {
    $downMsg = $WM_LBUTTONDOWN; $upMsg = $WM_LBUTTONUP; $mkButton = $MK_LBUTTON; $btnName = 'left'
}

# Pack X (low word) and Y (high word) into LPARAM
$lparam = [IntPtr](($Y -shl 16) -bor ($X -band 0xFFFF))

if (-not $NoMove) {
    [void][GameWin32]::PostMessage($hwndTarget, $WM_MOUSEMOVE, [IntPtr]0, $lparam)
    Start-Sleep -Milliseconds $DelayMs
}

[void][GameWin32]::PostMessage($hwndTarget, $downMsg, [IntPtr]$mkButton, $lparam)
Start-Sleep -Milliseconds $DelayMs
[void][GameWin32]::PostMessage($hwndTarget, $upMsg, [IntPtr]0, $lparam)

if ($Double) {
    Start-Sleep -Milliseconds $DelayMs
    [void][GameWin32]::PostMessage($hwndTarget, $downMsg, [IntPtr]$mkButton, $lparam)
    Start-Sleep -Milliseconds $DelayMs
    [void][GameWin32]::PostMessage($hwndTarget, $upMsg, [IntPtr]0, $lparam)
}

$kind = if ($Double) { "$btnName double-click" } else { "$btnName click" }
Write-Output "Sent $kind at ($X,$Y) to HWND $hwndTarget"
exit 0
