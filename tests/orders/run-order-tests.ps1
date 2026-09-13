# run-order-tests.ps1 -- compile & run the standalone #38 order-queue tests.
#
# Self-contained, the same shape as tests/ai/run-ai-tests.ps1: invokes cl.exe directly
# on header-only, dependency-free sources. Does NOT touch the game build, CMake, or any
# game source -- safe to run while the game is being built or played. Objects and exes
# go to d:\tmp\ordertests, outside the tree.
#
# Both suites are built and run TWICE, at /Od and at /O2, because the serialize half is
# arithmetic and the dispatch half is a state machine: an optimiser difference in either
# is a real finding.
#
# The serialize suite is also handed six production source paths and LINTS them, so a
# model that has drifted from the code it mirrors fails here. The LIFECYCLE suite goes
# further and compiles the production bodies themselves.
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
if (-not $vs) { Write-Error 'VS 2022 vcvars64.bat not found (edit roots in run-order-tests.ps1).'; exit 2 }
$vcvars = Join-Path $vs 'VC\Auxiliary\Build\vcvars64.bat'

$outDir = 'd:\tmp\ordertests'
New-Item -ItemType Directory -Force -Path $outDir | Out-Null

# repo root = two levels up from tests/orders
$srcRoot  = Join-Path $here '..\..\enations_latest\src'
$newUnit  = Join-Path $srcRoot 'new_unit.cpp'
$vehicleH = Join-Path $srcRoot 'vehicle.h'
$versionH = Join-Path $srcRoot 'version.h'
$unitCpp  = Join-Path $srcRoot 'unit.cpp'
$vehCpp   = Join-Path $srcRoot 'vehicle.cpp'
$areaCpp  = Join-Path $srcRoot 'area.cpp'

$suites = @(
    @{ name = 'serialize'; src = (Join-Path $here 'test_order_serialize.cpp') },
    @{ name = 'dispatch';  src = (Join-Path $here 'test_order_dispatch.cpp')  }
)

$failed = 0
foreach ($opt in @('/Od', '/O2')) {
    $tag = $opt.Substring(1)
    foreach ($s in $suites) {
        $exe = Join-Path $outDir ("order_{0}_{1}.exe" -f $s.name, $tag)
        $obj = Join-Path $outDir ("order_{0}_{1}.obj" -f $s.name, $tag)
        $cl  = "cl /nologo /EHsc /std:c++17 /W4 /D_CRT_SECURE_NO_WARNINGS $opt `"$($s.src)`" /Fo`"$obj`" /Fe`"$exe`""
        cmd /c "`"$vcvars`" >nul 2>&1 && $cl"
        if ($LASTEXITCODE -ne 0) { Write-Host "[orders] COMPILE FAILED: $($s.name) $opt"; exit 2 }

        Write-Host "--- $($s.name) $opt ---"
        if ($s.name -eq 'serialize') {
            & $exe $newUnit $vehicleH $versionH $unitCpp $vehCpp $areaCpp
        } else {
            & $exe
        }
        if ($LASTEXITCODE -ne 0) { $failed = 1 }
    }
}

# The LIFECYCLE suite compiles the PRODUCTION method bodies (tests/traffic's technique)
# rather than the mirror, so it can catch a defect that lives in the shipped code. Its
# runner is Python because the extraction is; it builds and runs at both optimisations too.
foreach ($opt in @('/Od', '/O2')) {
    Write-Host "--- lifecycle $opt ---"
    python (Join-Path $here 'run-order-lifecycle.py') --opt $opt
    if ($LASTEXITCODE -ne 0) { $failed = 1 }
}

# The DRAG-PLACE suite (#38 step 6) extracts its bodies from area.cpp the same way, so it
# gets the same treatment: both optimisations, and the source lint on the working tree.
foreach ($opt in @('/Od', '/O2')) {
    Write-Host "--- drag-place $opt ---"
    python (Join-Path $here 'run-drag-place.py') --opt $opt
    if ($LASTEXITCODE -ne 0) { $failed = 1 }
}

# The ROAD-GHOST suite (queue-road indicator) extracts CVehicle::RoadStepToward from
# vehicle.cpp verbatim and lints that _NextRoadHex still calls it - both optimisations.
foreach ($opt in @('/Od', '/O2')) {
    Write-Host "--- road-ghost $opt ---"
    python (Join-Path $here 'run-road-ghost.py') --opt $opt
    if ($LASTEXITCODE -ne 0) { $failed = 1 }
}

if ($failed -ne 0) { Write-Host '[orders] FAILURES'; exit 1 }
Write-Host '[orders] all suites pass (/Od and /O2)'
exit 0
