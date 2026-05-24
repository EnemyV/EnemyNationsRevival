# keys.ps1 — send keystrokes to the running Enemy Nations window.
#
# Usage:
#   ./keys.ps1 -Key Enter                  # press named key once
#   ./keys.ps1 -Key Down -Times 3          # repeat a key
#   ./keys.ps1 -Text "Hello"               # type literal text
#   ./keys.ps1 -ListKeys                   # show the named-key table
#   ./keys.ps1 -ProcessName foo            # override (default: auto enations/enations_gate)
#   ./keys.ps1 -Key Enter -Hwnd 12345      # target specific HWND
#
# Notes:
#   - Uses PostMessage (no focus needed). For text, sends WM_KEYDOWN +
#     WM_CHAR + WM_KEYUP per character — SDL2's text input picks this up.
#   - Modifier keys (Ctrl/Shift/Alt held with other keys) are NOT supported.
#   - Common keys: Enter, Escape, Tab, Space, Backspace, Delete, Up, Down,
#     Left, Right, Home, End, PageUp, PageDown, F1..F12, A..Z, 0..9.
#
# Exit codes: 0 ok, 1 game not running, 2 send failed.

[CmdletBinding()]
param(
    [string]$Key,
    [string]$Text,
    [int]$Times = 1,
    [int]$DelayMs = 30,
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
    # Prefer SDL_app — SDL2 only translates Win32 messages for windows it owns.
    $sdl = $windows | Where-Object { $_.Class -eq 'SDL_app' } | Select-Object -First 1
    if ($sdl) { return $sdl.Hwnd }
    if ($windows.Count -gt 0) { return $windows[0].Hwnd }
    if ($proc.MainWindowHandle -ne [IntPtr]::Zero) { return $proc.MainWindowHandle }
    Write-Error "Process '$($proc.ProcessName)' has no visible windows."
    exit 1
}

$hwndTarget = Resolve-GameHwnd -HwndOverride $Hwnd -ProcessName $ProcessName

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
Write-Output ("Sent {0} to HWND {1}" -f ($summary -join ' '), $hwndTarget)
exit 0
