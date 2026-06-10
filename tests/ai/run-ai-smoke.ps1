# run-ai-smoke.ps1 -- runtime smoke gate for the AI threading work.
#
# Launches the x64 Debug game UNDER THE DEBUGGER (dbgcatch.ps1, per project
# convention) with EN_PERF=1, loads a save via load-game.ps1 -NoLaunch, lets it
# run for a window, then asserts AI-health invariants from perf.log:
#
#   A1  process still alive at the end of the window (no crash/exit)
#   A2  ai.q.depth never negative (load-restore accounting holds)
#   A3  if messages were enqueued, messages were processed (queue drains)
#   A4  final ai.q.depth small (no unbounded backlog)
#   A5  if the snapshot served at all, miss ratio < 5%
#   A6  debugger log shows no access violations
#
# Assertions are on aggregates over the whole window (not instants) to avoid
# flakiness. Requires: no enations.exe already running; a save matching -Save.
#
# Exit codes: 0 all pass, 1 a check failed, 2 environment problem.

param(
    [string]$Save        = '5-3',
    [int]   $Seconds     = 60,
    [int]   $PickDelaySec = 8,
    [string]$Exe         = 'd:\Enemy Nations\src\cmakeBuild-x64\enations_latest\src\Debug\enations.exe',
    [string]$RunDir      = 'd:\Enemy Nations'
)

$ErrorActionPreference = 'Stop'
$here     = Split-Path -Parent $MyInvocation.MyCommand.Path
$repoRoot = (Resolve-Path (Join-Path $here '..\..')).Path
$perfLog  = Join-Path $RunDir 'perf.log'
$dbgLog   = 'd:\tmp\aitests\smoke-dbg.log'
New-Item -ItemType Directory -Force -Path 'd:\tmp\aitests' | Out-Null

function Fail([string]$msg) { Write-Host "SMOKE FAIL: $msg" -ForegroundColor Red; exit 1 }
function Note([string]$msg) { Write-Host "  $msg" }

# --- preconditions -----------------------------------------------------------
if (Get-Process enations -ErrorAction SilentlyContinue) {
    Write-Host 'SMOKE SKIP: enations.exe already running (close it first).'; exit 2
}
if (-not (Test-Path $Exe)) { Write-Host "SMOKE SKIP: exe not found: $Exe"; exit 2 }

# record where perf.log ends now; we only parse lines written by THIS run
$perfStart = 0
if (Test-Path $perfLog) { $perfStart = (Get-Item $perfLog).Length }

# --- launch under debugger with EN_PERF --------------------------------------
$env:EN_PERF = '1'
$dbgSeconds  = $Seconds + 120   # cover menu + load + window + teardown
$dbgcatch    = Join-Path $repoRoot 'dbgcatch.ps1'
$dbgProc = Start-Process powershell -ArgumentList @(
    '-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', $dbgcatch,
    '-Seconds', $dbgSeconds, '-Exe', $Exe, '-WorkDir', $RunDir
) -RedirectStandardOutput $dbgLog -PassThru -WindowStyle Hidden

Note "launched under dbgcatch (pid $($dbgProc.Id)), waiting for menu..."
$deadline = (Get-Date).AddSeconds(60)
$game = $null
while ((Get-Date) -lt $deadline) {
    $game = Get-Process enations -ErrorAction SilentlyContinue
    if ($game) { break }
    Start-Sleep -Seconds 2
}
if (-not $game) { Fail 'game process never appeared' }
Start-Sleep -Seconds 12   # let the main menu settle

# --- load the save ------------------------------------------------------------
Note "loading save '$Save'..."
& (Join-Path $repoRoot 'load-game.ps1') -NoLaunch -Save $Save -PickDelaySec $PickDelaySec | Out-Null
Start-Sleep -Seconds 5

$game = Get-Process enations -ErrorAction SilentlyContinue
if (-not $game) { Fail 'game died during load' }

# --- observation window --------------------------------------------------------
Note "observing for $Seconds s..."
Start-Sleep -Seconds $Seconds

# A1: still alive
$game = Get-Process enations -ErrorAction SilentlyContinue
$alive = [bool]$game
if ($game) { Stop-Process -Id $game.Id -Force -Confirm:$false }   # clean up

# --- parse this run's perf.log lines ------------------------------------------
if (-not (Test-Path $perfLog)) { Fail 'no perf.log written (EN_PERF not honored?)' }
$fs = [IO.File]::Open($perfLog, 'Open', 'Read', 'ReadWrite')
$fs.Seek($perfStart, 'Begin') | Out-Null
$reader = New-Object IO.StreamReader($fs)
$text = $reader.ReadToEnd(); $reader.Close(); $fs.Close()
$lines = $text -split "`n" | Where-Object { $_ -match 'ai\.q\.depth=' }
if ($lines.Count -lt 5) { Fail "too few perf samples in window ($($lines.Count)) - did the load fail?" }

function SumOf([string]$name) {
    ($lines | ForEach-Object { if ($_ -match "$name=(-?\d+)") { [int]$Matches[1] } } | Measure-Object -Sum).Sum
}
function MinOf([string]$name) {
    ($lines | ForEach-Object { if ($_ -match "$name=(-?\d+)") { [int]$Matches[1] } } | Measure-Object -Minimum).Minimum
}
$qMin   = MinOf  'ai\.q\.depth'
$qLast  = if ($lines[-1] -match 'ai\.q\.depth=(-?\d+)') { [int]$Matches[1] } else { 0 }
$enq    = SumOf 'ai\.msg\.enq'
$proc   = SumOf 'ai\.msg\.proc'
$hit    = SumOf 'ai\.snap\.hit'
$miss   = SumOf 'ai\.snap\.miss'

Note "samples=$($lines.Count) qMin=$qMin qLast=$qLast enq=$enq proc=$proc snapHit=$hit snapMiss=$miss alive=$alive"

# --- assertions ----------------------------------------------------------------
$failed = 0
if (-not $alive)        { Write-Host 'A1 FAIL: process died during window' -ForegroundColor Red; $failed++ }
else                    { Write-Host 'A1 ok: process alive through window' }
if ($qMin -lt 0)        { Write-Host "A2 FAIL: ai.q.depth went negative ($qMin)" -ForegroundColor Red; $failed++ }
else                    { Write-Host 'A2 ok: queue gauge never negative' }
if ($enq -gt 0 -and $proc -eq 0) { Write-Host 'A3 FAIL: messages enqueued but none processed' -ForegroundColor Red; $failed++ }
else                    { Write-Host 'A3 ok: queue drains (or no traffic)' }
if ($qLast -gt 100)     { Write-Host "A4 FAIL: backlog grew unbounded (final depth $qLast)" -ForegroundColor Red; $failed++ }
else                    { Write-Host 'A4 ok: no unbounded backlog' }
if (($hit + $miss) -gt 0 -and $miss / [double]($hit + $miss) -ge 0.05) {
    Write-Host "A5 FAIL: snapshot miss ratio $([math]::Round(100*$miss/($hit+$miss),1))%" -ForegroundColor Red; $failed++
} else                  { Write-Host 'A5 ok: snapshot healthy (or unused this window)' }
$av = Select-String -Path $dbgLog -Pattern 'ACCESS_VIOLATION|0xC0000005' -ErrorAction SilentlyContinue
if ($av)                { Write-Host "A6 FAIL: access violation in debugger log ($dbgLog)" -ForegroundColor Red; $failed++ }
else                    { Write-Host 'A6 ok: no access violations caught' }

if ($failed) { Write-Host "SMOKE: $failed check(s) FAILED (debugger log: $dbgLog)" -ForegroundColor Red; exit 1 }
Write-Host 'SMOKE: all checks passed' -ForegroundColor Green
exit 0
