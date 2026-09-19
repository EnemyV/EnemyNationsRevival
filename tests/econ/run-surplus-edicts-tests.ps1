# run-surplus-edicts-tests.ps1 -- compile & run the surplus-scaled edict fixture.
#
# Self-contained: invokes cl.exe directly on a header-only, dependency-free test.
# Does NOT touch the game build, CMake, or any game source -- safe to run while the
# main game is being built.
#
# test_surplus_edicts.cpp mirrors SurplusShare/SurplusScale (enations_latest/src/edicts.h)
# and the sequential-share walk in CPlayer::ApplySurplusEdicts (player.cpp), and pins the
# bounded-draft and fixed-point properties the family depends on.
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
if (-not $vs) { Write-Error 'VS 2022 vcvars64.bat not found (edit roots in run-surplus-edicts-tests.ps1).'; exit 2 }

$vcvars = Join-Path $vs 'VC\Auxiliary\Build\vcvars64.bat'
$outDir = 'd:\tmp\econtests'
New-Item -ItemType Directory -Force -Path $outDir | Out-Null

$src = Join-Path $here 'test_surplus_edicts.cpp'
if (-not (Test-Path $src)) { Write-Error "fixture source not found: $src"; exit 2 }

$exe = Join-Path $outDir 'surplus_edicts_tests.exe'
$obj = Join-Path $outDir 'surplus_edicts.obj'
$cl  = "cl /nologo /EHsc /std:c++17 /W4 /WX /fp:strict `"$src`" /Fo`"$obj`" /Fe`"$exe`""

cmd /c "`"$vcvars`" >nul 2>&1 && $cl"
if ($LASTEXITCODE -ne 0) { Write-Host '[surplus_edicts] COMPILE FAILED'; exit 2 }

& $exe
if ($LASTEXITCODE -ne 0) { Write-Host '[surplus_edicts] FAILED'; exit 1 }
Write-Host '[surplus_edicts] PASS'
exit 0
