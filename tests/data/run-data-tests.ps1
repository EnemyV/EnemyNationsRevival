# run-data-tests.ps1 -- compile & run the standalone .dat/loose-data tests
# (GitHub issue #36, 015 area 3).
#
# Same shape as tests/ai/run-ai-tests.ps1: cl.exe is invoked directly on
# self-contained sources, nothing touches the game build, CMake or the running
# game, and every artifact lands OUTSIDE the tree (d:\tmp\datatests by default).
#
# Suites:
#   test_data_hash    the gameplay hash walker (enations_latest/src/datahash_walk.h)
#   test_data_loader  the SHIPPED windward/wind22/src/datafile.cpp, compiled
#                     against tests/data/shim and driven over a real temp tree
#   test_data_net     the CNetJoin / CNetPublish header layout and length rule
#   test_data_expl    the explosion cleanup frame (EXPL_KILLFRAME)
#
# Every suite is compiled and run TWICE, /Od and /O2, because the walker and the
# explosion arithmetic are both inline-heavy.
#
# Exit codes: 0 all pass, 1 a test failed, 2 toolchain / compile error.

param(
    [string]$OutDir = 'd:\tmp\datatests'
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
if (-not $vs) { Write-Error 'VS 2022 vcvars64.bat not found (edit roots in run-data-tests.ps1).'; exit 2 }
$vcvars = Join-Path $vs 'VC\Auxiliary\Build\vcvars64.bat'

New-Item -ItemType Directory -Force -Path $OutDir | Out-Null

# The loader fixture compiles the SHIPPED windward/wind22/src/datafile.cpp, which
# needs the shipped datafile.h for CDataFile. That header's first line is
# #include "stdafx.h", and MSVC resolves a quoted include from the including
# file's own directory first, so including it as-is drags in the real wind22
# stdafx.h (SDL, CDIB, the whole stack) instead of tests/data/shim. Copy it here
# with that one line dropped -- REGENERATED from the shipped header every run, so
# the fixture cannot be testing a stale copy of the class.
$dfHeader = Join-Path $here '..\..\windward\wind22\include\datafile.h'
if (Test-Path $dfHeader) {
    Get-Content $dfHeader |
        Where-Object { $_ -notmatch '^\s*#include\s+"stdafx\.h"' } |
        Set-Content -Encoding ascii (Join-Path $OutDir 'datafile_under_test.h')
}

$suites = @(
    @{ name = 'test_data_hash';   extra = '' },
    @{ name = 'test_data_loader'; extra = "/I`"$here\shim`" /I`"$OutDir`" /I`"$here\..\..\windward\wind22\include`" shlwapi.lib" },
    @{ name = 'test_data_net';    extra = '' },
    @{ name = 'test_data_expl';   extra = '' }
)

$failed = 0
foreach ($opt in @('/Od', '/O2')) {
    foreach ($s in $suites) {
        $src = Join-Path $here "$($s.name).cpp"
        if (-not (Test-Path $src)) { Write-Host "[$($s.name)] SKIP (no source)"; continue }

        $tag = "$($s.name)$($opt.Replace('/',''))"
        $exe = Join-Path $OutDir "$tag.exe"
        $cl  = "cl /nologo /EHsc /std:c++17 /W4 $opt `"$src`" /Fo`"$OutDir\$tag.obj`" /Fe`"$exe`" $($s.extra)"

        Write-Host "=== $($s.name) $opt ==="
        cmd /c "`"$vcvars`" >nul 2>&1 && $cl"
        if ($LASTEXITCODE -ne 0) { Write-Error "compile failed: $($s.name) $opt"; exit 2 }

        # Scratch dir via the environment, never argv: CDataFile::Init treats the
        # first command-line argument as a .dat path.
        $env:EN_DATA_TEST_DIR = Join-Path $OutDir "$tag.work"
        & $exe
        if ($LASTEXITCODE -ne 0) { $failed = 1 }
    }
}

if ($failed -ne 0) { exit 1 }
Write-Host 'ALL DATA SUITES PASSED (/Od and /O2)'
exit 0
