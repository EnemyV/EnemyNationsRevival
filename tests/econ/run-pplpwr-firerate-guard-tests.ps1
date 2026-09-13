# run-pplpwr-firerate-guard-tests.ps1 -- compile & run the standalone BUGS #87/#82
# power/workforce-ratio and fire-rate divisor guard fixture.
#
# Fully self-contained: invokes cl.exe directly on a header-only, dependency-free
# test. Does NOT touch the game build, CMake, or any game source -- safe to run
# while the game is being built, developed, or played.
#
# Compiled TWICE, /Od and /O2, and must pass under both.
#
# Build artifacts go to $OutDir (default d:\tmp\econtests), outside the repo.
#
# Exit codes: 0 all pass, 1 a test failed, 2 toolchain / compile error.

param(
    [string]$OutDir = 'd:\tmp\econtests'
)

$ErrorActionPreference = 'Stop'
$here = Split-Path -Parent $MyInvocation.MyCommand.Path

$roots = @(
    'C:\Program Files\Microsoft Visual Studio\2022\Community',
    'C:\Program Files\Microsoft Visual Studio\2022\Enterprise',
    'C:\Program Files\Microsoft Visual Studio\2022\Professional'
)
$vs = $roots | Where-Object { Test-Path (Join-Path $_ 'VC\Auxiliary\Build\vcvars64.bat') } | Select-Object -First 1
if (-not $vs) { Write-Error 'VS 2022 vcvars64.bat not found (edit roots in run-pplpwr-firerate-guard-tests.ps1).'; exit 2 }

$vcvars = Join-Path $vs 'VC\Auxiliary\Build\vcvars64.bat'
New-Item -ItemType Directory -Force -Path $OutDir | Out-Null

$src = Join-Path $here 'test_pplpwr_firerate_guard.cpp'
if (-not (Test-Path $src)) { Write-Error "fixture source not found: $src"; exit 2 }

$failed = 0

foreach ($opt in @('/Od', '/O2')) {
    $tag = $opt.TrimStart('/')
    $exe = Join-Path $OutDir "pplpwr_firerate_guard_$tag.exe"
    $obj = Join-Path $OutDir "pplpwr_firerate_guard_$tag.obj"
    # /fp:strict: the guard's finiteness claims must hold under the same strict IEEE
    # float semantics the shipped code (built without fast-math) relies on.
    $cl  = "cl /nologo /EHsc /std:c++17 /W4 /WX /fp:strict $opt `"$src`" /Fo`"$obj`" /Fe`"$exe`""

    Write-Host "--- pplpwr_firerate_guard [$tag] ---"
    cmd /c "`"$vcvars`" >nul 2>&1 && $cl"
    if ($LASTEXITCODE -ne 0) { Write-Host "[pplpwr_firerate_guard] COMPILE FAILED ($tag)"; exit 2 }

    & $exe
    if ($LASTEXITCODE -ne 0) { Write-Host "[pplpwr_firerate_guard] FAILED ($tag)"; $failed = 1 }
    else                     { Write-Host "[pplpwr_firerate_guard] PASS ($tag)" }
}

if ($failed -ne 0) { exit 1 }
exit 0
