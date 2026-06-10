# run-ai-tests.ps1 -- compile & run the standalone AI logic tests.
#
# Fully self-contained: invokes cl.exe directly on a header-only, dependency-free
# test. Does NOT touch the game build, CMake, or any game source -- safe to run
# while the main game is being built or developed.
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
if (-not $vs) { Write-Error 'VS 2022 vcvars64.bat not found (edit roots in run-ai-tests.ps1).'; exit 2 }

$vcvars = Join-Path $vs 'VC\Auxiliary\Build\vcvars64.bat'
$src    = Join-Path $here 'test_ai_staging.cpp'
$outDir = 'd:\tmp\aitests'
New-Item -ItemType Directory -Force -Path $outDir | Out-Null
$obj = Join-Path $outDir 'ai_tests.obj'
$exe = Join-Path $outDir 'ai_tests.exe'

$cl   = "cl /nologo /EHsc /std:c++17 /W4 `"$src`" /Fo`"$obj`" /Fe`"$exe`""
$line = "`"$vcvars`" >nul 2>&1 && $cl && `"$exe`""

cmd /c $line
exit $LASTEXITCODE
