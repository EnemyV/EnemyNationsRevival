# run-ui-tests.ps1 -- compile & run the standalone SDL2 UI geometry tests.
#
# Same shape as tests/ai/run-ai-tests.ps1 and tests/orders/run-order-tests.ps1: invokes
# cl.exe directly on the PRODUCTION header, so it does NOT touch the game build, CMake,
# or any game source -- safe to run while the game is being built or played. Objects and
# exes go to d:\tmp\uitests, outside the tree.
#
# The suite compiles enations_latest/src/SDL2Scrollbar.h itself (not a mirror of it), so
# it needs the bundled SDL2 headers on the include path. It never LINKS SDL: every
# function it exercises is pure arithmetic over SDL_Rect.
#
# Built and run at both /Od and /O2 -- the geometry is integer division, where an
# optimiser difference would be a real finding.
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
if (-not $vs) { Write-Error 'VS 2022 vcvars64.bat not found (edit roots in run-ui-tests.ps1).'; exit 2 }
$vcvars = Join-Path $vs 'VC\Auxiliary\Build\vcvars64.bat'

$outDir = 'd:\tmp\uitests'
New-Item -ItemType Directory -Force -Path $outDir | Out-Null

# repo root = two levels up from tests/ui
$srcRoot = Resolve-Path (Join-Path $here '..\..\enations_latest\src')
$sdlInc  = Resolve-Path (Join-Path $here '..\..\tools\sdl2\SDL2-2.30.12\include')

$src = Join-Path $here 'test_scrollbar_geom.cpp'

$failed = 0
foreach ($opt in @('/Od', '/O2')) {
    $tag = $opt.Substring(1)
    $exe = Join-Path $outDir ("ui_scrollbar_{0}.exe" -f $tag)
    $obj = Join-Path $outDir ("ui_scrollbar_{0}.obj" -f $tag)
    $cl  = "cl /nologo /EHsc /std:c++17 /W4 /D_CRT_SECURE_NO_WARNINGS $opt /I`"$srcRoot`" /I`"$sdlInc`" `"$src`" /Fo`"$obj`" /Fe`"$exe`""
    cmd /c "`"$vcvars`" >nul 2>&1 && $cl"
    if ($LASTEXITCODE -ne 0) { Write-Host "[ui] COMPILE FAILED: scrollbar $opt"; exit 2 }

    Write-Host "--- scrollbar geometry $opt ---"
    & $exe
    if ($LASTEXITCODE -ne 0) { $failed = 1 }
}

# --- radar / world-map background BAKE CADENCE -------------------------------
# Compiles the production enations_latest/src/radarbake.h and table-tests
# RadarShouldBake, then lints world.cpp/world.h for the promotion that pinned the
# radar to the 140ms cadence. The exe is ALSO run with --baseline, which evaluates
# the same table against a verbatim replication of the pre-fix decision: that run
# MUST fail, or the table is not actually pinning the regression.
$srcRadar = Join-Path $here "test_radar_bake.cpp"
foreach ($opt in @("/Od", "/O2")) {
    $tag = $opt.Substring(1)
    $exe = Join-Path $outDir ("ui_radar_bake_{0}.exe" -f $tag)
    $obj = Join-Path $outDir ("ui_radar_bake_{0}.obj" -f $tag)
    $cl  = "cl /nologo /EHsc /std:c++17 /W4 /D_CRT_SECURE_NO_WARNINGS $opt /I`"$srcRoot`" `"$srcRadar`" /Fo`"$obj`" /Fe`"$exe`""
    cmd /c "`"$vcvars`" >nul 2>&1 && $cl"
    if ($LASTEXITCODE -ne 0) { Write-Host "[ui] COMPILE FAILED: radar_bake $opt"; exit 2 }

    Write-Host "--- radar bake cadence $opt ---"
    & $exe "--src=$srcRoot"
    if ($LASTEXITCODE -ne 0) { $failed = 1 }

    Write-Host "--- radar bake cadence $opt : pre-fix baseline (a FAIL here is the pass) ---"
    & $exe "--baseline" | Out-Null
    if ($LASTEXITCODE -eq 0) {
        Write-Host "[ui] BASELINE DID NOT FAIL -- the radar-bake table no longer discriminates"
        $failed = 1
    } else {
        Write-Host "[ui] pre-fix baseline correctly rejected (exit $LASTEXITCODE)"
    }
}

if ($failed -ne 0) { Write-Host '[ui] FAILURES'; exit 1 }
Write-Host '[ui] all suites pass (/Od and /O2)'
exit 0
