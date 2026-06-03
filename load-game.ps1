# load-game.ps1 — fast path: launch the game (if needed), open the Load dialog,
# and (optionally) load a save by name. Built for the LLM to get in-game quickly
# without screenshotting the main menu (button coords are fixed).
#
# Usage:
#   ./load-game.ps1                 # launch if needed, click Load, screenshot the save list
#   ./load-game.ps1 -Save savegame3 # ...then pick that save and click Open
#   ./load-game.ps1 -NoLaunch       # don't auto-launch; assume game is at the menu
#
# Flow: main menu "Load Single Player Game" (fixed coord) -> "Load Game" dialog
# (its own child SDL window) -> list of saves -> Open button (fixed coord).

[CmdletBinding()]
param(
    [string]$Save,
    [switch]$NoLaunch,
    [string]$ListShot = 'd:\tmp\savelist.png'
)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'harness-common.ps1')

# ---- fixed coordinates (native res) --------------------------------------
$LoadBtn = @{ X = 1850; Y = 145 }     # main menu "Load Single Player Game"
# Load Game dialog (480x420 child window). From SDL2FileBrowser.cpp geometry:
$Dlg = @{
    ListTop  = 86      # file list top (client y)
    ListLeft = 14      # click x inside a row
    ItemH    = 18      # listbox row height
    OpenX    = 185; OpenY = 376   # Open button center
}

$exe = 'd:\Enemy Nations\src\cmakeBuild-x64\enations_latest\src\Debug\enations.exe'

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

# ---- 1. launch if needed -------------------------------------------------
if (-not (Find-GameProcess) -and -not $NoLaunch) {
    if (-not (Test-Path $exe)) { Write-Error "x64 Debug exe not found: $exe"; exit 1 }
    Write-Output "Launching $exe ..."
    Start-Process -FilePath $exe -WorkingDirectory 'd:\Enemy Nations'
}

$menu = Wait-Window -Title 'Game View'
if (-not $menu) { Write-Error "Main menu window never appeared."; exit 1 }

# ---- 2. click "Load Single Player Game" ----------------------------------
& (Join-Path $PSScriptRoot 'click.ps1') -Window main -X $LoadBtn.X -Y $LoadBtn.Y | Out-Null

# ---- 3. wait for the Load Game dialog ------------------------------------
# NB: PowerShell vars are case-insensitive, so this MUST NOT be named $dlg —
# that would alias the $Dlg coords hashtable above and clobber it.
$dlgWin = Wait-Window -Title 'Load Game' -TimeoutMs 8000
if (-not $dlgWin) { Write-Error "Load Game dialog never appeared (is a game already loaded? try from the menu)."; exit 1 }

# ---- 4. screenshot just the save list ------------------------------------
$listH = ($dlgWin.CH - 180)   # matches SDL2FileBrowser listH = m_height-180
& (Join-Path $PSScriptRoot 'screenshot.ps1') -Window 'Load Game' -Full -Region ("0,{0},{1},{2}" -f $Dlg.ListTop, $dlgWin.CW, $listH) -Out $ListShot | Out-Null
Write-Output "Save list captured -> $ListShot  (read it to see the save names)"

# ---- 5. optional: pick a save by name and Open ---------------------------
if ($Save) {
    # enumerate saves the same way the dialog does, to find the row index
    $dir = 'd:\Enemy Nations\src\cmakeBuild-x64\enations_latest\src\Debug'
    $saves = @(Get-ChildItem -Path $dir -Filter *.en -File -ErrorAction SilentlyContinue |
               Sort-Object Name)  # files come after dirs; "[.]" is row 0
    $names = @('[.]') + ($saves | ForEach-Object { $_.Name })
    $idx = [Array]::FindIndex($names, [Predicate[string]]{ param($n) $n -like "*$Save*" })
    if ($idx -lt 0) {
        Write-Warning "Save '$Save' not found in list: $($names -join ', '). Screenshot saved; click manually."
        exit 0
    }
    $rowY = $Dlg.ListTop + $idx * $Dlg.ItemH + [int]($Dlg.ItemH / 2)
    Write-Output ("Double-clicking row {0} ('{1}') at y={2}" -f $idx, $names[$idx], $rowY)
    & (Join-Path $PSScriptRoot 'click.ps1') -Window 'Load Game' -X $Dlg.ListLeft -Y $rowY -Double | Out-Null
    Start-Sleep -Milliseconds 500
    # double-click confirms (OnFileDblClick -> OnConfirm); if the dialog is still
    # up, fall back to clicking Open explicitly.
    $still = Wait-Window -Title 'Load Game' -TimeoutMs 1500
    if ($still) {
        & (Join-Path $PSScriptRoot 'click.ps1') -Window 'Load Game' -X $Dlg.OpenX -Y $Dlg.OpenY | Out-Null
        Write-Output "Clicked Open."
    }
    Write-Output "Load issued for '$($names[$idx])'."
}
exit 0
