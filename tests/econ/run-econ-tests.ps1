# run-econ-tests.ps1 -- compile & run the standalone economy/production tests.
#
# Fully self-contained: invokes cl.exe directly on header-only, dependency-free
# tests. Does NOT touch the game build, CMake, or any game source -- safe to run
# while the main game is being built or developed.
#
# test_mat_prod_bar.cpp (bug #13): mirrors CBuilding::BuildMaterials' numeric core,
# drives it fed vs iron-starved, and pins the shipped bar predicate
# (enecon::MatBarStalled, enations_latest/src/econprod.h) against it. It also lints
# new_unit.cpp so the gate can't be reverted silently -- the lint skips cleanly if
# that source is missing.
#
# Exit codes: 0 all pass, 1 a test failed, 2 toolchain / compile error.

$ErrorActionPreference = 'Stop'
$here = Split-Path -Parent $MyInvocation.MyCommand.Path

# Same VS 2022 roots build.ps1 keys off of.
$roots = @(
    'C:\Program Files\Microsoft Visual Studio\2022\Community',
    'C:\Program Files\Microsoft Visual Studio\2022\Enterprise',
    'C:\Program Files\Microsoft Visual Studio\2022\Professional'
)
$vs = $roots | Where-Object { Test-Path (Join-Path $_ 'VC\Auxiliary\Build\vcvars64.bat') } | Select-Object -First 1
if (-not $vs) { Write-Error 'VS 2022 vcvars64.bat not found (edit roots in run-econ-tests.ps1).'; exit 2 }

$vcvars = Join-Path $vs 'VC\Auxiliary\Build\vcvars64.bat'
$outDir = 'd:\tmp\econtests'
New-Item -ItemType Directory -Force -Path $outDir | Out-Null

$srcBar  = Join-Path $here 'test_mat_prod_bar.cpp'
$exeBar  = Join-Path $outDir 'mat_prod_bar_tests.exe'
$newUnit = Join-Path $here '..\..\enations_latest\src\new_unit.cpp'

$clBar = "cl /nologo /EHsc /std:c++17 /W4 `"$srcBar`" /Fo`"$outDir\mat_prod_bar.obj`" /Fe`"$exeBar`""

cmd /c "`"$vcvars`" >nul 2>&1 && $clBar"
if ($LASTEXITCODE -ne 0) { exit 2 }

$barExit = 0
if (Test-Path $newUnit) {
    & $exeBar (Resolve-Path $newUnit).Path
    $barExit = $LASTEXITCODE
    if ($barExit -eq 2) { Write-Host "[matbar] SKIP (cannot open $newUnit)"; $barExit = 0 }
} else {
    & $exeBar
    $barExit = $LASTEXITCODE
}

if ($barExit -ne 0) { exit 1 }
exit 0
