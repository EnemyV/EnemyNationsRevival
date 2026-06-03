# harness-common.ps1 — shared window resolution + capture for the Enemy Nations
# test harness (screenshot.ps1 / click.ps1 / keys.ps1 / dbgshot.ps1).
#
# Dot-source this from a harness script:
#     . (Join-Path $PSScriptRoot 'harness-common.ps1')
#
# Why this exists: in-game the game is NOT a single window. The main SDL window
# ("Enemy Nations - Game View") holds the toolbar/chrome, but the interactive
# map, radar, and every detached panel (Vehicles/Buildings/Research/dialogs) are
# each their OWN child SDL_app window with its own SDL windowID. SDL tags input
# events with the windowID of the HWND that received them, so a click posted to
# the parent never reaches the map child. PrintWindow on the parent likewise
# can't capture the child GL windows. So the harness must target the RIGHT
# SDL_app window by role, not just "the biggest one". HWNDs are also unstable
# (they change on reload), so everything resolves by class+title each call.

# ---- Win32 interop -------------------------------------------------------
if (-not ('GameWin32' -as [type])) {
    Add-Type @'
using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Text;

public class GameWindowInfo {
    public IntPtr Hwnd;
    public IntPtr Parent;
    public string Title;
    public string Class;
    public int CW;     // client width
    public int CH;     // client height
    public int WL, WT, WR, WB;   // window rect (screen)
    public int Area;   // client area, for sorting
}

public static class GameWin32 {
    [DllImport("user32.dll")] public static extern bool PostMessage(IntPtr hWnd, uint Msg, IntPtr wParam, IntPtr lParam);
    [DllImport("user32.dll")] public static extern bool GetClientRect(IntPtr hWnd, out RECT lpRect);
    [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr hWnd, out RECT lpRect);
    [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr hWnd, IntPtr hdcBlt, uint nFlags);
    [DllImport("user32.dll")] public static extern bool EnumWindows(EnumWindowsProc enumProc, IntPtr lParam);
    [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr hWnd, out uint lpdwProcessId);
    [DllImport("user32.dll")] public static extern bool IsWindowVisible(IntPtr hWnd);
    [DllImport("user32.dll")] public static extern bool IsIconic(IntPtr hWnd);
    [DllImport("user32.dll")] public static extern IntPtr GetParent(IntPtr hWnd);
    [DllImport("user32.dll", CharSet = CharSet.Auto)] public static extern int GetWindowText(IntPtr hWnd, StringBuilder lpString, int nMaxCount);
    [DllImport("user32.dll", CharSet = CharSet.Auto)] public static extern int GetClassName(IntPtr hWnd, StringBuilder lpClassName, int nMaxCount);
    [DllImport("user32.dll")] public static extern bool ClientToScreen(IntPtr hWnd, ref POINT lpPoint);
    [DllImport("user32.dll")] public static extern bool SetCursorPos(int X, int Y);
    public delegate bool EnumWindowsProc(IntPtr hWnd, IntPtr lParam);
    [StructLayout(LayoutKind.Sequential)] public struct RECT { public int Left, Top, Right, Bottom; }
    [StructLayout(LayoutKind.Sequential)] public struct POINT { public int X, Y; }

    public static GameWindowInfo Info(IntPtr hwnd) {
        RECT c, w; GetClientRect(hwnd, out c); GetWindowRect(hwnd, out w);
        var title = new StringBuilder(256); GetWindowText(hwnd, title, 256);
        var cls = new StringBuilder(256); GetClassName(hwnd, cls, 256);
        int cw = c.Right - c.Left, ch = c.Bottom - c.Top;
        return new GameWindowInfo {
            Hwnd = hwnd, Parent = GetParent(hwnd),
            Title = title.ToString(), Class = cls.ToString(),
            CW = cw, CH = ch, WL = w.Left, WT = w.Top, WR = w.Right, WB = w.Bottom,
            Area = cw * ch
        };
    }

    public static List<GameWindowInfo> EnumProcessWindows(uint targetPid) {
        var results = new List<GameWindowInfo>();
        EnumWindows(delegate(IntPtr hwnd, IntPtr lparam) {
            uint wpid; GetWindowThreadProcessId(hwnd, out wpid);
            if (wpid == targetPid && IsWindowVisible(hwnd))
                results.Add(Info(hwnd));
            return true;
        }, IntPtr.Zero);
        return results;
    }
}
'@
}

# ---- Role aliases --------------------------------------------------------
# Map a friendly -Window role to a title substring. Anything not in the table
# is treated as a raw (case-insensitive) title substring.
$script:HarnessRoleAliases = @{
    'main'      = 'Game View'; 'menu'   = 'Game View'; 'game' = 'Game View'
    'gameview'  = 'Game View'; 'chrome' = 'Game View'
    'map'       = 'Area Map';  'area'   = 'Area Map'
    'radar'     = 'Radar'
    'vehicles'  = 'Vehicles';  'veh'    = 'Vehicles'
    'buildings' = 'Buildings'; 'bld'    = 'Buildings'
    'research'  = 'Research'
    'pick'      = 'Pick Your Player'; 'player' = 'Pick Your Player'
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

# All visible SDL_app windows of the game, sorted largest client area first.
# SDL only translates Win32 input for windows IT owns (class SDL_app), so the
# MFC plumbing windows (EnAreaWnd / EnPopupWnd / EnWorldWnd) are excluded.
function Get-GameSdlWindows {
    param($Process)
    if (-not $Process) { return @() }
    $all = [GameWin32]::EnumProcessWindows([uint32]$Process.Id)
    return @($all | Where-Object { $_.Class -eq 'SDL_app' -and $_.CW -gt 0 -and $_.CH -gt 0 } |
             Sort-Object Area -Descending)
}

# Resolve the target window. Returns a GameWindowInfo, or exits with an error.
#   -Hwnd     : explicit window handle (wins over everything)
#   -Window   : role alias or raw title substring
#   (neither) : auto — in-game pick "Area Map"; otherwise the largest window
#               (covers the main menu and modal dialogs, which are single-window)
function Resolve-GameTarget {
    param([int]$Hwnd, [string]$Window, [string]$ProcessName)

    if ($Hwnd -gt 0) { return [GameWin32]::Info([IntPtr]$Hwnd) }

    $proc = Find-GameProcess -ExplicitName $ProcessName
    if (-not $proc) {
        $hint = if ($ProcessName) { "'$ProcessName'" } else { "'enations' or 'enations_gate'" }
        Write-Error "No process found matching $hint. Is the game running?"
        exit 1
    }
    $windows = Get-GameSdlWindows -Process $proc
    if ($windows.Count -eq 0) {
        Write-Error "Process '$($proc.ProcessName)' (PID $($proc.Id)) has no visible SDL_app windows."
        exit 1
    }

    if ($Window) {
        $key = $Window.ToLower()
        $needle = if ($script:HarnessRoleAliases.ContainsKey($key)) { $script:HarnessRoleAliases[$key] } else { $Window }
        $hit = $windows | Where-Object { $_.Title -like "*$needle*" } | Select-Object -First 1
        if (-not $hit) {
            $have = ($windows | ForEach-Object { "'$($_.Title)'" }) -join ', '
            Write-Error "No SDL_app window matching '$Window' (needle '$needle'). Open windows: $have"
            exit 1
        }
        return $hit
    }

    # Auto: prefer the interactive map when in-game; else the largest window
    # (main menu / modal dialog / pick-player are single-window states).
    $map = $windows | Where-Object { $_.Title -like '*Area Map*' } | Select-Object -First 1
    if ($map) { return $map }
    return $windows[0]
}

# Capture one window's client area to a System.Drawing.Bitmap (caller disposes).
# PrintWindow works even when the window is occluded/background; -Screen uses a
# screen-grab (window must be visible & unoccluded).
function Get-WindowBitmap {
    param([Parameter(Mandatory)]$Info, [switch]$Screen)
    Add-Type -AssemblyName System.Drawing
    $w = $Info.CW; $h = $Info.CH
    if ($w -le 0 -or $h -le 0) { throw "Window '$($Info.Title)' has zero size ($w x $h) — minimized?" }
    $bmp = New-Object System.Drawing.Bitmap $w, $h
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    if ($Screen) {
        $pt = New-Object GameWin32+POINT; $pt.X = 0; $pt.Y = 0
        [void][GameWin32]::ClientToScreen($Info.Hwnd, [ref]$pt)
        $g.CopyFromScreen($pt.X, $pt.Y, 0, 0, (New-Object System.Drawing.Size $w, $h))
    } else {
        $hdc = $g.GetHdc()
        $PW_CLIENTONLY = 0x1; $PW_RENDERFULLCONTENT = 0x2
        $ok = [GameWin32]::PrintWindow($Info.Hwnd, $hdc, [uint32]($PW_CLIENTONLY -bor $PW_RENDERFULLCONTENT))
        $g.ReleaseHdc($hdc)
        if (-not $ok) { $g.Dispose(); $bmp.Dispose(); throw "PrintWindow failed for '$($Info.Title)'. Try -Screen." }
    }
    $g.Dispose()
    return $bmp
}

# Composite the main window + all child SDL_app windows into one bitmap, so a
# single image shows the true on-screen layout (PrintWindow on the parent alone
# misses the child GL windows). Children are blitted at their client-origin
# offset relative to the main window, smaller windows last (drawn on top).
function Get-CompositeBitmap {
    param([Parameter(Mandatory)]$Process)
    Add-Type -AssemblyName System.Drawing
    $windows = Get-GameSdlWindows -Process $Process
    if ($windows.Count -eq 0) { throw "No SDL_app windows to composite." }
    $main = $windows | Where-Object { $_.Parent -eq [IntPtr]::Zero } | Select-Object -First 1
    if (-not $main) { $main = $windows[0] }

    $canvas = New-Object System.Drawing.Bitmap $main.CW, $main.CH
    $cg = [System.Drawing.Graphics]::FromImage($canvas)

    # main client origin in screen coords (offsets are relative to this)
    $mo = New-Object GameWin32+POINT; $mo.X = 0; $mo.Y = 0
    [void][GameWin32]::ClientToScreen($main.Hwnd, [ref]$mo)

    # main first (background), then children largest→smallest so panels land on top
    $ordered = @($main) + @($windows | Where-Object { $_.Hwnd -ne $main.Hwnd } | Sort-Object Area -Descending)
    foreach ($win in $ordered) {
        try { $b = Get-WindowBitmap -Info $win } catch { continue }
        if ($win.Hwnd -eq $main.Hwnd) {
            $cg.DrawImage($b, 0, 0)
        } else {
            $co = New-Object GameWin32+POINT; $co.X = 0; $co.Y = 0
            [void][GameWin32]::ClientToScreen($win.Hwnd, [ref]$co)
            $cg.DrawImage($b, ($co.X - $mo.X), ($co.Y - $mo.Y))
        }
        $b.Dispose()
    }
    $cg.Dispose()
    return $canvas
}
