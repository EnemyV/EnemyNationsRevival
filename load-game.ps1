# load-game.ps1 — fast path: launch the game (if needed), open the Load dialog,
# and (optionally) load a save by name. Built for the LLM to get in-game quickly
# without screenshotting the main menu (button coords are fixed).
#
# Usage:
#   ./load-game.ps1                 # launch if needed, click Load, screenshot the save list
#   ./load-game.ps1 -Save savegame3 # ...then pick that save, VERIFY it, and click Open
#   ./load-game.ps1 -NoLaunch       # don't auto-launch; assume game is at the menu
#   ./load-game.ps1 -Release        # use the Release run dir instead of Debug
#
# Flow: main menu "Load Single Player Game" (fixed coord) -> "Load Game" dialog
# (its own child SDL window) -> list of saves -> Open button (fixed coord).
#
# SAFETY: -Save never clicks blind. The row index is computed from the run
# directory of the RUNNING exe (the very directory the dialog enumerates), the
# row is SINGLE-clicked, the dialog is re-captured and the highlighted row is
# read back from the pixels, and Open is only clicked when the highlight is on
# the row we asked for. Anything else is a hard failure (exit 1) — this script
# must never be able to load a save other than the one named.

[CmdletBinding()]
param(
    [string]$Save,
    [switch]$NoLaunch,
    [switch]$Release,             # run dir: ...\Release instead of ...\Debug
    [string]$ListShot = 'd:\tmp\savelist.png',
    [string]$VerifyShot,          # default: <ListShot>-selected.png (whole dialog)
    [switch]$NoVerify,            # escape hatch: skip the read-back (NOT recommended)
    [int]$PickDelaySec = 5,   # after load, wait this long then confirm the
                             # auto-selected player on the "Pick Your Player" screen
    [int]$PickOkX = 235,      # OK-button center (window-local px) on the 580x500 Pick
    [int]$PickOkY = 470       # dialog: btn = (m_width/2-100, m_height-45, 90, 30)
                              # -> center ((580/2-100)+45, (500-45)+15) = (235, 470)
                              # (SDL2PickPlayerDialog::OnInit, SDL2Dialogs.cpp)
)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'harness-common.ps1')
Add-Type -AssemblyName System.Drawing

# ---- fixed coordinates (native res) --------------------------------------
$LoadBtn = @{ X = 1850; Y = 145 }     # main menu "Load Single Player Game"

# Load Game dialog: a 480x420 BORDERLESS SDL child window of its own.
# Every number below is read straight out of SDL2FileBrowser::OnInit
# (enations_latest/src/SDL2FileBrowser.cpp) and SDL2Listbox (SDL2UI.cpp/.h).
#
# Widgets are laid out in MAIN-window coords (m_x + dx, m_y + dy); the dialog's
# own window shows the blit of rect (m_x, m_y, m_width, m_height) 1:1, and
# SDL2Dialog::HandleEvent adds (m_x, m_y) back onto every mouse event that
# arrives on the dialog window before hit-testing. So dialog-CLIENT coords ==
# the dy/dx offsets below, which is exactly what click.ps1 sends.
#
#   y  = 34                     # below the title bar
#   y += 24                     # after the path label
#   y += 28                     # after the "Up .." + drive-letter button row
#   -> ListTop = 34+24+28 = 86  # top of the SDL2Listbox
#   listH = m_height - 180 = 240
#   y += listH + 6 -> 332       # "Filename:" label + edit box (h 24)
#   y += 30        -> 362       # Open / Cancel row (btnW 100, btnH 28, gap 10)
#   btnX = 8 + ((480-16) - 210)/2 = 135  -> Open center = (135+50, 362+14)
#                                        = (185, 376)   <- OpenX/OpenY below
#
# Row height is SDL2Listbox::m_itemHeight, which is a PRIVATE member with NO
# setter (SDL2UI.h) — it is 22 for every listbox in the game, never 18. Both
# the renderer (iy = m_rect.y + i*m_itemHeight) and the hit test
# (idx = (event.y - m_rect.y) / m_itemHeight + m_scrollOffset) use it, so:
#
#     rowY(idx) = ListTop + idx*ItemH + ItemH/2      # center of row idx
#     idx(y)    = (y - ListTop) / ItemH              # what the game will pick
#
# The listbox draws floor(listH / ItemH) = 240/22 = 10 rows and has NO
# scrollbar, so rows >= 10 are unreachable — the script must refuse, not click.
$Dlg = @{
    ListTop  = 86      # file list top (client y)  = 34 + 24 + 28
    ListLeft = 14      # click x inside a row      = m_rect.x(8) + 6 (text inset)
    ItemH    = 22      # SDL2Listbox::m_itemHeight (SDL2UI.h) — NOT 18
    ListX    = 8       # m_rect.x of the listbox   = brdSide
    ListW    = 464     # m_rect.w of the listbox   = m_width - brdSide*2
    OpenX    = 185; OpenY = 376   # Open button center
    # SDL2Listbox::m_colSelBg — the selected row's fill. Read back to prove
    # which row the game actually selected.
    SelR = 48; SelG = 58; SelB = 148
}

# ---- run directory (SINGLE source of truth) ------------------------------
# The dialog lists SDL2FileBrowser's m_currentDir, which defaults to
# ExeDirectory() — the folder of the RUNNING enations.exe. Every worktree has
# its own cmakeBuild-x64\...\Debug with its own saves, so a hardcoded 'src'
# path would enumerate one folder while the dialog shows another: the row index
# would be computed against the wrong list and a DIFFERENT save would load.
# So: resolve our own tree from $PSScriptRoot the way build.ps1 does, and once
# the game is up, prefer the running process's real exe path over that.
$repoRoot = $PSScriptRoot
$config   = 'Debug'
if ($Release) { $config = 'Release' }
$exe = Join-Path $repoRoot "cmakeBuild-x64\enations_latest\src\$config\enations.exe"

function Get-ProcessExePath {
    param($Process)
    if (-not $Process) { return $null }
    try {
        $p = $Process.MainModule.FileName
        if ($p) { return $p }
    } catch { }
    try {
        $ci = Get-CimInstance Win32_Process -Filter "ProcessId=$($Process.Id)" -ErrorAction Stop
        if ($ci -and $ci.ExecutablePath) { return $ci.ExecutablePath }
    } catch { }
    return $null
}

function Wait-Window {
    param([string]$Title, [int]$TimeoutMs = 20000)
    $sw = [Diagnostics.Stopwatch]::StartNew()
    while ($sw.ElapsedMilliseconds -lt $TimeoutMs) {
        $proc = Find-GameProcess
        if ($proc) {
            $w = Get-GameSdlWindows -Process $proc | Where-Object { $_.Title -like "*$Title*" } | Select-Object -First 1
            if ($w) { return $w }
        }
        Start-Sleep -Milliseconds 300
    }
    return $null
}

# Enumerate a directory EXACTLY the way SDL2FileBrowser::PopulateList does:
#   * _findfirst("<dir>/*") — so hidden/system entries are included, "." is
#     dropped and ".." is NOT (it shows up as the "[..]" row).
#   * subdirectories first, each rendered as "[name]", then the files.
#   * files kept only when EndsWithCI(name, ".en").
#   * BOTH groups sorted with _stricmp — an ordinal compare of the lowercased
#     names, NOT the culture-aware compare Sort-Object uses. That is why the
#     dialog shows Save015-Island-2.en < Save015-Island-3.en < Save015-Island.en
#     ('-' 0x2D sorts before '.' 0x2E); Sort-Object would put Save015-Island.en
#     first and every row index after it would be off by one.
#     (ASCII names only — _stricmp is C-locale tolower, which is what save
#     names have always been.)
function Get-BrowserEntries {
    param([string]$Dir, [string]$Filter = '.en')

    $cmp = [System.Comparison[string]] {
        param([string]$a, [string]$b)
        [string]::CompareOrdinal($a.ToLowerInvariant(), $b.ToLowerInvariant())
    }

    $dirNames = New-Object 'System.Collections.Generic.List[string]'
    # ".." exists for anything that is not a drive root
    if ((Split-Path -Path $Dir -Parent)) { $dirNames.Add('..') }
    foreach ($d in @(Get-ChildItem -LiteralPath $Dir -Directory -Force -ErrorAction SilentlyContinue)) {
        $dirNames.Add($d.Name)
    }
    $dirArr = [string[]]$dirNames.ToArray()
    if ($dirArr.Length -gt 1) { [Array]::Sort($dirArr, $cmp) }

    $fileNames = New-Object 'System.Collections.Generic.List[string]'
    foreach ($f in @(Get-ChildItem -LiteralPath $Dir -File -Force -ErrorAction SilentlyContinue)) {
        if ($f.Name -like "*$Filter") { $fileNames.Add($f.Name) }
    }
    $fileArr = [string[]]$fileNames.ToArray()
    if ($fileArr.Length -gt 1) { [Array]::Sort($fileArr, $cmp) }

    $entries = New-Object 'System.Collections.Generic.List[object]'
    foreach ($n in $dirArr)  { $entries.Add([pscustomobject]@{ Name = $n; Label = "[$n]"; IsDir = $true  }) }
    foreach ($n in $fileArr) { $entries.Add([pscustomobject]@{ Name = $n; Label = $n;     IsDir = $false }) }
    return $entries.ToArray()
}

# Read back WHICH row the game highlighted, from the dialog's own pixels.
# SDL2Listbox fills the selected row with m_colSelBg over the full row rect
# {m_rect.x+1, iy, m_rect.w-2, m_itemHeight}; unselected rows are white. Sample
# only the left and right MARGINS of the row band — the text slot is
# {m_rect.x+6, iy, m_rect.w-12, ...}, and unselected rows draw their text in
# the SAME blue as the selection fill, so sampling across the text would not
# discriminate. Returns -1 when nothing is highlighted.
function Get-SelectedRowFromPixels {
    param($Bitmap, [int]$RowCount)

    $tol = 10
    $xs = New-Object 'System.Collections.Generic.List[int]'
    foreach ($x in @(($Dlg.ListX + 1)..($Dlg.ListX + 4))) { $xs.Add($x) }                       # left margin
    # SCROLLBAR INTERACTION (WinOpus 2026-09-13): SDL2Listbox now reserves a right-hand
    # gutter for the scrollbar (ContentW()), and the selection fill stops short of it.
    # Sampling the old right margin therefore reads GUTTER, never highlight, so the 80%
    # threshold could never be met and this returned -1 on a correctly selected row --
    # verified live: the row WAS highlighted and the Filename field agreed. Drop the
    # right-margin band; the left margin alone identifies the row and is unaffected.
    # (Failing closed was the right direction, but it blocked every load.)

    $best = -1; $bestFrac = 0.0
    for ($i = 0; $i -lt $RowCount; $i++) {
        $top = $Dlg.ListTop + $i * $Dlg.ItemH
        $hit = 0; $tot = 0
        for ($y = $top + 2; $y -le ($top + $Dlg.ItemH - 3); $y++) {
            if ($y -ge $Bitmap.Height) { break }
            foreach ($x in $xs) {
                if ($x -ge $Bitmap.Width) { continue }
                $tot++
                $c = $Bitmap.GetPixel($x, $y)
                if ([math]::Abs([int]$c.R - $Dlg.SelR) -le $tol -and
                    [math]::Abs([int]$c.G - $Dlg.SelG) -le $tol -and
                    [math]::Abs([int]$c.B - $Dlg.SelB) -le $tol) { $hit++ }
            }
        }
        if ($tot -eq 0) { continue }
        $frac = $hit / $tot
        if ($frac -gt $bestFrac) { $bestFrac = $frac; $best = $i }
    }
    if ($bestFrac -lt 0.8) { return -1 }   # no row highlighted (or capture is junk)
    return $best
}

# ---- 1. launch if needed -------------------------------------------------
$launchedByUs = $false
if (-not (Find-GameProcess) -and -not $NoLaunch) {
    if (-not (Test-Path -LiteralPath $exe)) { Write-Error "x64 $config exe not found: $exe"; exit 1 }
    Write-Output "Launching $exe ..."
    Start-Process -FilePath $exe -WorkingDirectory 'd:\Enemy Nations'
    $launchedByUs = $true
}

$menu = Wait-Window -Title 'Game View'
if (-not $menu) { Write-Error "Main menu window never appeared."; exit 1 }

# The dialog enumerates the RUNNING exe's directory — take the path from the
# live process, not from our guess, so the row index can only ever come from
# the folder the dialog is actually showing.
$runningExe = Get-ProcessExePath -Process (Find-GameProcess)
if ($runningExe) {
    if (-not $launchedByUs -and ($runningExe -ne $exe)) {
        Write-Warning "Running exe is NOT this tree's build:`n  running: $runningExe`n  mine   : $exe`nUsing the RUNNING exe's directory for the save list."
    }
    $exe = $runningExe
} elseif (-not $launchedByUs) {
    Write-Warning "Could not read the running exe's path; falling back to $exe"
}
$dir = Split-Path -Path $exe -Parent

# ---- 2. click "Load Single Player Game" ----------------------------------
& (Join-Path $PSScriptRoot 'click.ps1') -Window main -X $LoadBtn.X -Y $LoadBtn.Y | Out-Null

# ---- 3. wait for the Load Game dialog ------------------------------------
# NB: PowerShell vars are case-insensitive, so this MUST NOT be named $dlg —
# that would alias the $Dlg coords hashtable above and clobber it.
$dlgWin = Wait-Window -Title 'Load Game' -TimeoutMs 8000
if (-not $dlgWin) { Write-Error "Load Game dialog never appeared (is a game already loaded? try from the menu)."; exit 1 }

# ---- 4. screenshot just the save list ------------------------------------
$listH = ($dlgWin.CH - 180)   # matches SDL2FileBrowser listH = m_height-180
$visibleRows = [int][math]::Floor($listH / $Dlg.ItemH)   # 240/22 = 10, no scrollbar
& (Join-Path $PSScriptRoot 'screenshot.ps1') -Window 'Load Game' -Full -Region ("0,{0},{1},{2}" -f $Dlg.ListTop, $dlgWin.CW, $listH) -Out $ListShot | Out-Null
Write-Output "Save list captured -> $ListShot  (read it to see the save names)"

# ---- 5. optional: pick a save by name and Open ---------------------------
if ($Save) {
    if (-not (Test-Path -LiteralPath $dir)) {
        Write-Error "Run directory does not exist: $dir"; exit 1
    }
    Write-Output "Save list directory (running exe's dir): $dir"

    $entries = @(Get-BrowserEntries -Dir $dir)
    $labels  = @($entries | ForEach-Object { $_.Label })

    # Match FILES only, and never guess: exact name wins (with or without the
    # .en extension); otherwise a substring match must be UNIQUE. A first-match
    # wins search would happily pick Save015-Island-2.en for -Save
    # 'Save015-Island'.
    $files = @($entries | Where-Object { -not $_.IsDir })
    $exact = @($files | Where-Object {
        $_.Name.Equals($Save, [StringComparison]::OrdinalIgnoreCase) -or
        $_.Name.Equals("$Save.en", [StringComparison]::OrdinalIgnoreCase)
    })
    $hits = $exact
    if ($hits.Count -eq 0) {
        $hits = @($files | Where-Object { $_.Name.IndexOf($Save, [StringComparison]::OrdinalIgnoreCase) -ge 0 })
    }
    if ($hits.Count -eq 0) {
        Write-Error "Save '$Save' not found in $dir. Dialog rows: $($labels -join ', ')"
        exit 1
    }
    if ($hits.Count -gt 1) {
        Write-Error "Save '$Save' is ambiguous — matches: $(($hits | ForEach-Object { $_.Name }) -join ', '). Pass the full name."
        exit 1
    }
    $target = $hits[0]
    $idx = [Array]::IndexOf($labels, $target.Label)
    if ($idx -lt 0) { Write-Error "Internal error: '$($target.Label)' missing from the row list."; exit 1 }

    # DO NOT SCROLL FROM THE HARNESS (WinOpus 2026-09-13, after a near-miss).
    # SDL2Listbox now HAS a scrollbar, and I taught this script to page to an
    # off-window row. It loaded the WRONG SAVE and reported success: a trough click
    # pages by a whole window and clamps to (count - visibleRows), not by the one row
    # my arithmetic assumed, so the list never moved and visible row 9 was a different
    # file. The read-back did not catch it because it compares the row INDEX it clicked
    # against the row INDEX that highlighted - those agree by construction whenever the
    # scroll offset is wrong, so that check is blind to exactly this error.
    # Until the read-back reads the FILENAME FIELD (the only ground truth on screen),
    # refusing is correct: a harness that silently loads the wrong save is worse than
    # one that stops. Scroll the dialog by hand, or pass a save inside the window.
    if ($idx -ge $visibleRows) {
        Write-Error ("'{0}' is row {1}; only rows 0-{2} are in the dialog's window ({3}px list / {4}px rows). The widget HAS a scrollbar now, but this script does not drive it safely - see the note above. Scroll manually and re-run, or pass one of: {5}" -f `
            $target.Name, $idx, ($visibleRows - 1), $listH, $Dlg.ItemH, (($labels | Select-Object -First $visibleRows) -join ', '))
        exit 1
    }
    $rowY = $Dlg.ListTop + $idx * $Dlg.ItemH + [int]($Dlg.ItemH / 2)
    Write-Output ("Selecting row {0} ('{1}') at y={2}" -f $idx, $target.Label, $rowY)

    # Single click = select only (SDL2FileBrowser::OnFileSelected copies the name
    # into the Filename: box). Deliberately NOT a double click: a double click
    # confirms immediately, leaving no chance to check what got selected.
    & (Join-Path $PSScriptRoot 'click.ps1') -Window 'Load Game' -X $Dlg.ListLeft -Y $rowY | Out-Null

    # ---- 5a. read the selection back before committing --------------------
    if (-not $NoVerify) {
        if (-not $VerifyShot) {
            $shotDir = [IO.Path]::GetDirectoryName($ListShot)
            if (-not $shotDir) { $shotDir = '.' }
            $VerifyShot = Join-Path $shotDir `
                          ([IO.Path]::GetFileNameWithoutExtension($ListShot) + '-selected' + [IO.Path]::GetExtension($ListShot))
        }
        # .NET resolves relative paths against the PROCESS cwd, not $PWD.
        $VerifyShot = [IO.Path]::Combine((Get-Location).ProviderPath, $VerifyShot)

        $seen = -1
        for ($try = 0; $try -lt 5; $try++) {
            Start-Sleep -Milliseconds 400
            $info = Wait-Window -Title 'Load Game' -TimeoutMs 1000
            if (-not $info) { break }      # dialog vanished — handled below
            $bmp = $null
            try { $bmp = Get-WindowBitmap -Info $info } catch { continue }
            try {
                $seen = Get-SelectedRowFromPixels -Bitmap $bmp -RowCount $visibleRows
                if ($seen -eq $idx) {
                    try { $bmp.Save($VerifyShot, [System.Drawing.Imaging.ImageFormat]::Png) } catch { }
                    break
                }
            } finally { $bmp.Dispose() }
        }

        if ($seen -ne $idx) {
            $sawWhat = '(nothing highlighted, or the dialog could not be captured)'
            if ($seen -ge 0 -and $seen -lt $labels.Count) { $sawWhat = "row $seen = '$($labels[$seen])'" }
            elseif ($seen -ge 0) { $sawWhat = "row $seen" }
            Write-Error ("Selection check FAILED: asked for row {0} ('{1}') but the dialog highlights {2}. NOT clicking Open — nothing was loaded. Check {3}." -f `
                $idx, $target.Label, $sawWhat, $ListShot)
            exit 1
        }
        Write-Output ("Verified: the dialog highlights row {0} ('{1}') -> {2}" -f $idx, $target.Label, $VerifyShot)
    }

    # ---- 5b. confirm with Open -------------------------------------------
    # Two tries: the first click can land only as a focus grab on the borderless
    # dialog. Both are the same harmless "confirm the row we just verified".
    & (Join-Path $PSScriptRoot 'click.ps1') -Window 'Load Game' -X $Dlg.OpenX -Y $Dlg.OpenY | Out-Null
    Start-Sleep -Milliseconds 500
    if (Wait-Window -Title 'Load Game' -TimeoutMs 800) {
        & (Join-Path $PSScriptRoot 'click.ps1') -Window 'Load Game' -X $Dlg.OpenX -Y $Dlg.OpenY | Out-Null
        Start-Sleep -Milliseconds 500
    }
    if (Wait-Window -Title 'Load Game' -TimeoutMs 2000) {
        # OnConfirm() refuses to close when the Filename: box is empty or the
        # file does not _stat — so the dialog still being up means nothing loaded.
        Write-Error "Clicked Open but the Load Game dialog is still up — '$($target.Name)' was not accepted. Nothing loaded."
        exit 1
    }
    Write-Output "Load issued for '$($target.Name)'."


# PICKER RESOLUTION BY SIZE, not title (WinOpus 2026-09-13). The Pick-Your-Player
# window's title reads back from GetWindowTextW as the single character 'P' - the
# UTF-16 resource-string truncation this codebase has hit before - so EVERY
# `-Window pick` / `Wait-Window -Title 'Pick Your Player'` lookup fails and the
# script gave up on a dialog that was plainly on screen. Its 580x500 client rect
# is distinctive, so match on that instead.
Add-Type -TypeDefinition @'
using System;using System.Runtime.InteropServices;using System.Collections.Generic;
public class EnPickWin {
  [DllImport("user32.dll")] static extern bool EnumWindows(EnumProc f, IntPtr l);
  [DllImport("user32.dll")] static extern bool IsWindowVisible(IntPtr h);
  [DllImport("user32.dll")] static extern uint GetWindowThreadProcessId(IntPtr h, out uint p);
  [DllImport("user32.dll")] static extern bool GetClientRect(IntPtr h, out RECT r);
  public struct RECT { public int l, t, r, b; }
  delegate bool EnumProc(IntPtr h, IntPtr l);
  public static long Find(uint pid, int cw, int ch) {
    long found = 0;
    EnumWindows((h, l) => {
      uint p; GetWindowThreadProcessId(h, out p);
      if (p == pid && IsWindowVisible(h)) {
        RECT r; GetClientRect(h, out r);
        if (r.r == cw && r.b == ch) { found = h.ToInt64(); return false; }
      }
      return true;
    }, IntPtr.Zero);
    return found;
  }
}
'@ -ErrorAction SilentlyContinue

function Get-PickerHwnd {
    param([int]$TimeoutMs = 60000)
    $proc = Get-Process enations -ErrorAction SilentlyContinue | Select-Object -First 1
    if (-not $proc) { return 0 }
    $deadline = (Get-Date).AddMilliseconds($TimeoutMs)
    while ((Get-Date) -lt $deadline) {
        $h = [EnPickWin]::Find([uint32]$proc.Id, 580, 500)
        if ($h -ne 0) { return $h }
        Start-Sleep -Milliseconds 500
    }
    return 0
}

    # ---- 6. confirm the auto-selected player ------------------------------
    # Loading a save lands on the "Pick Your Player" dialog with a player already
    # selected. Confirm it with a robust click-OK -> Enter -> click-OK chain:
    #   * SDL routes KEYBOARD events to its focus window, and the harness posts via
    #     PostMessage WITHOUT activating the (borderless) dialog -> a raw Enter is
    #     dropped. But a MOUSE click IS delivered to the targeted window AND gives
    #     it SDL keyboard focus.
    #   * So: click OK first (confirms directly, same path as the save-list click);
    #     if the dialog is still up the click only focused it, so NOW Enter routes
    #     (Enter is bound to OK in SDL2Dialog::HandleEvent); final OK click as
    #     last resort. Whichever lands first, the rest are harmless no-ops.
    if ($PickDelaySec -gt 0) {
        Write-Output "Waiting ${PickDelaySec}s for load to settle, then confirming player..."
        Start-Sleep -Seconds $PickDelaySec

        # Wait for the picker by SIZE. A 3.8MB save with ~800 units takes well over
        # the old fixed delay to reach this dialog, so poll rather than assume.
        $pickH = Get-PickerHwnd -TimeoutMs 90000
        if ($pickH -eq 0) { Write-Error "Pick-Your-Player dialog never appeared (580x500). Nothing loaded."; exit 1 }
        Write-Output ("Picker found (hwnd {0}); confirming the pre-selected player." -f $pickH)

        & (Join-Path $PSScriptRoot 'click.ps1') -Hwnd ([int]$pickH) -X $PickOkX -Y $PickOkY | Out-Null
        Start-Sleep -Milliseconds 600

        if ((Get-PickerHwnd -TimeoutMs 1200) -ne 0) {
            # click only focused it -> Enter now reaches the (focused) dialog
            & (Join-Path $PSScriptRoot 'keys.ps1') -Hwnd ([int]$pickH) -Key Enter | Out-Null
            Start-Sleep -Milliseconds 400
            if ((Get-PickerHwnd -TimeoutMs 1000) -ne 0) {
                & (Join-Path $PSScriptRoot 'click.ps1') -Hwnd ([int]$pickH) -X $PickOkX -Y $PickOkY | Out-Null
                Start-Sleep -Milliseconds 400
            }
        }

        if ((Get-PickerHwnd -TimeoutMs 500) -ne 0) {
            Write-Warning "Pick-Your-Player dialog still open after OK+Enter+OK — confirm manually."
        } else {
            Write-Output "Player confirmed; in-game."
        }
    }
}
exit 0
