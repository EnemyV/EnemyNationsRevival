# run-route-cursor-tests.ps1 -- compile & run the standalone BUGS #99 route-cursor
# save-format fixture (save format Release 8).
#
# Fully self-contained: invokes cl.exe directly on a header-only, dependency-free
# test. Does NOT touch the game build, CMake, or any game source -- safe to run
# while the game is being built, developed, or played.
#
# The fixture is compiled TWICE, /Od and /O2, and must pass under both: the writer
# and reader mirrors turn on signed/unsigned WORD comparisons and on a countdown
# that must run off the tail, and those are exactly the shapes an optimiser is
# allowed to reason about differently.
#
# Build artifacts go to $OutDir (default d:\tmp\ordertests), outside the repo.
#
# Exit codes: 0 all pass, 1 a test failed, 2 toolchain / compile error.

param(
    [string]$OutDir = 'd:\tmp\ordertests'
)

$ErrorActionPreference = 'Stop'
$here = Split-Path -Parent $MyInvocation.MyCommand.Path

# Same VS 2022 roots build.ps1 and tests/ai key off of.
$roots = @(
    'C:\Program Files\Microsoft Visual Studio\2022\Community',
    'C:\Program Files\Microsoft Visual Studio\2022\Enterprise',
    'C:\Program Files\Microsoft Visual Studio\2022\Professional'
)
$vs = $roots | Where-Object { Test-Path (Join-Path $_ 'VC\Auxiliary\Build\vcvars64.bat') } | Select-Object -First 1
if (-not $vs) { Write-Error 'VS 2022 vcvars64.bat not found (edit roots in run-route-cursor-tests.ps1).'; exit 2 }

$vcvars = Join-Path $vs 'VC\Auxiliary\Build\vcvars64.bat'
New-Item -ItemType Directory -Force -Path $OutDir | Out-Null

$src = Join-Path $here 'test_route_cursor.cpp'
if (-not (Test-Path $src)) { Write-Error "fixture source not found: $src"; exit 2 }

$failed = 0

foreach ($opt in @('/Od', '/O2')) {
    $tag = $opt.TrimStart('/')
    $exe = Join-Path $OutDir "route_cursor_$tag.exe"
    $obj = Join-Path $OutDir "route_cursor_$tag.obj"
    $cl  = "cl /nologo /EHsc /std:c++17 /W4 /WX $opt `"$src`" /Fo`"$obj`" /Fe`"$exe`""

    Write-Host "--- route_cursor [$tag] ---"
    cmd /c "`"$vcvars`" >nul 2>&1 && $cl"
    if ($LASTEXITCODE -ne 0) { Write-Host "[route_cursor] COMPILE FAILED ($tag)"; exit 2 }

    & $exe
    if ($LASTEXITCODE -ne 0) { Write-Host "[route_cursor] FAILED ($tag)"; $failed = 1 }
    else                     { Write-Host "[route_cursor] PASS ($tag)" }
}

if ($failed -ne 0) { exit 1 }
exit 0
