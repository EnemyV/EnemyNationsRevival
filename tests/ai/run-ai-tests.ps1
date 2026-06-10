# run-ai-tests.ps1 -- compile & run the standalone AI logic tests + the
# stdgta.dat data-integrity tests.
#
# Fully self-contained: invokes cl.exe directly on header-only, dependency-free
# tests. Does NOT touch the game build, CMake, or any game source -- safe to run
# while the main game is being built or developed.
#
# The data test parses the SHIPPED stdgta.dat (resolved relative to the repo;
# override with -DataFile). If the file is missing it is reported as SKIP and
# does not fail the run.
#
# Exit codes: 0 all pass, 1 a test failed, 2 toolchain / compile error.

param(
    [string]$DataFile = ''
)

$ErrorActionPreference = 'Stop'
$here = Split-Path -Parent $MyInvocation.MyCommand.Path

# Same VS 2022 roots build.ps1 keys off of.
$roots = @(
    'C:\Program Files\Microsoft Visual Studio\2022\Community',
    'C:\Program Files\Microsoft Visual Studio\2022\Enterprise',
    'C:\Program Files\Microsoft Visual Studio\2022\Professional'
)
$vs = $roots | Where-Object { Test-Path (Join-Path $_ 'VC\Auxiliary\Build\vcvars64.bat') } | Select-Object -First 1
if (-not $vs) { Write-Error 'VS 2022 vcvars64.bat not found (edit roots in run-ai-tests.ps1).'; exit 2 }

$vcvars = Join-Path $vs 'VC\Auxiliary\Build\vcvars64.bat'
$outDir = 'd:\tmp\aitests'
New-Item -ItemType Directory -Force -Path $outDir | Out-Null

# repo root = two levels up from tests/ai
if (-not $DataFile) {
    $DataFile = Join-Path $here '..\..\enations_latest\data\files\stdgta.dat'
}

$srcLogic = Join-Path $here 'test_ai_staging.cpp'
$srcData  = Join-Path $here 'test_ai_data.cpp'
$srcPaths = Join-Path $here 'test_ai_paths.cpp'
$exeLogic = Join-Path $outDir 'ai_tests.exe'
$exeData  = Join-Path $outDir 'ai_data_tests.exe'
$exePaths = Join-Path $outDir 'ai_path_tests.exe'
$caigmgr  = Join-Path $here '..\..\enations_latest\src\caigmgr.cpp'

$clLogic = "cl /nologo /EHsc /std:c++17 /W4 `"$srcLogic`" /Fo`"$outDir\ai_tests.obj`" /Fe`"$exeLogic`""
$clData  = "cl /nologo /EHsc /std:c++17 /W4 `"$srcData`" /Fo`"$outDir\ai_data.obj`" /Fe`"$exeData`""
$clPaths = "cl /nologo /EHsc /std:c++17 /W4 `"$srcPaths`" /Fo`"$outDir\ai_paths.obj`" /Fe`"$exePaths`""

# compile + run logic suite
cmd /c "`"$vcvars`" >nul 2>&1 && $clLogic && `"$exeLogic`""
$logicExit = $LASTEXITCODE
if ($logicExit -eq 2) { exit 2 }   # compile failure

# compile + run data suite (skip cleanly if the data file is absent)
$dataExit = 0
cmd /c "`"$vcvars`" >nul 2>&1 && $clData"
if ($LASTEXITCODE -ne 0) { exit 2 }
if (Test-Path $DataFile) {
    & $exeData (Resolve-Path $DataFile).Path
    $dataExit = $LASTEXITCODE
    if ($dataExit -eq 2) { Write-Host "[ai_data] SKIP (cannot open $DataFile)"; $dataExit = 0 }
} else {
    Write-Host "[ai_data] SKIP (no data file at $DataFile)"
}

# compile + run research-path source lint (skip cleanly if source absent)
$pathsExit = 0
cmd /c "`"$vcvars`" >nul 2>&1 && $clPaths"
if ($LASTEXITCODE -ne 0) { exit 2 }
if (Test-Path $caigmgr) {
    & $exePaths (Resolve-Path $caigmgr).Path
    $pathsExit = $LASTEXITCODE
    if ($pathsExit -eq 2) { Write-Host "[ai_paths] SKIP (cannot open $caigmgr)"; $pathsExit = 0 }
} else {
    Write-Host "[ai_paths] SKIP (no source at $caigmgr)"
}

if ($logicExit -ne 0 -or $dataExit -ne 0 -or $pathsExit -ne 0) { exit 1 }
exit 0
