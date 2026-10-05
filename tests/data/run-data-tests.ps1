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
    [string]$OutDir = (Join-Path $env:TEMP 'datatests'),
    # Extracted effect.rif, for the explosion suite's art measurement. Left
    # empty it is searched for beside the repo (see below) and, failing that,
    # that one check SKIPs -- the rest of the suite needs no data files.
    [string]$EffectRif = '',
    # Optional extracted data/data directory. When supplied, a private copy is
    # used to run the production aggregate hash and edit gameplay/art bytes.
    [string]$RealDataDir = ''
)

$ErrorActionPreference = 'Stop'
$here = Split-Path -Parent $MyInvocation.MyCommand.Path

# Same VS 2022 roots build.ps1 keys off of.
$roots = @()
foreach ( $vsBase in @( "$env:ProgramFiles\Microsoft Visual Studio\2022",
                        "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2022" ) ) {
    foreach ( $vsEd in @( 'Community', 'Enterprise', 'Professional', 'BuildTools' ) ) {
        $roots += ( Join-Path $vsBase $vsEd )
    }
}
$vs = $roots | Where-Object { Test-Path (Join-Path $_ 'VC\Auxiliary\Build\vcvars64.bat') } | Select-Object -First 1
if (-not $vs) { Write-Error 'VS 2022 vcvars64.bat not found (edit roots in run-data-tests.ps1).'; exit 2 }
$vcvars = Join-Path $vs 'VC\Auxiliary\Build\vcvars64.bat'

New-Item -ItemType Directory -Force -Path $OutDir | Out-Null

function Test-LooseDataCopy([string]$DataPath) {
    $manifestPath = Join-Path $DataPath 'manifest.txt'
    if (-not (Test-Path -LiteralPath $manifestPath -PathType Leaf)) { return $false }
    $listed = @{}
    foreach ($line in Get-Content -LiteralPath $manifestPath) {
        $fields = $line -split '\s+'
        if ($fields.Count -ne 3 -or $fields[1] -notmatch '^\d+$' -or $fields[2] -notmatch '^[0-9a-fA-F]{64}$') { return $false }
        $relative = $fields[0].Replace('/', '\')
        $path = Join-Path $DataPath $relative
        if (-not (Test-Path -LiteralPath $path -PathType Leaf)) { return $false }
        $item = Get-Item -LiteralPath $path
        if ($item.Length -ne [long]$fields[1]) { return $false }
        if ((Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash -ne $fields[2]) { return $false }
        $listed[$relative.ToLowerInvariant()] = $true
    }
    $actual = @(Get-ChildItem -LiteralPath $DataPath -Recurse -File)
    if ($actual.Count -ne ($listed.Count + 1)) { return $false }
    foreach ($item in $actual) {
        $relative = $item.FullName.Substring($DataPath.Length + 1).ToLowerInvariant()
        if ($relative -ne 'manifest.txt' -and -not $listed.ContainsKey($relative)) { return $false }
    }
    return $listed.Count -gt 0
}

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

# Generate an isolated copy of the production aggregate source/header with only
# the stdafx include redirected to our shim and the datahash.h include pointed
# at a generated header. The function body and gameplay member table are copied
# verbatim from the shipped source on each run.
$hashHeader = Join-Path $here '..\..\enations_latest\src\datahash.h'
if (Test-Path $hashHeader) {
    Get-Content $hashHeader |
        Where-Object { $_ -notmatch '^\s*#include\s+"stdafx\.h"' } |
        Set-Content -Encoding ascii (Join-Path $OutDir 'datahash_under_test.h')
}
$hashSource = Join-Path $here '..\..\enations_latest\src\datahash.cpp'
if (Test-Path $hashSource) {
    $hashText = Get-Content -Raw $hashSource
    $hashText = $hashText.Replace('#include "datahash.h"', '#include "datahash_under_test.h"')
    $hashText = "#include `"stdafx.h`"`r`n" + $hashText
    Set-Content -Encoding ascii (Join-Path $OutDir 'datahash_under_test.cpp') $hashText
}

# The explosion suite re-derives EXPL_KILLFRAME from the stock explosion sprite's
# frame count, so it needs the extracted effect.rif. The loose set lives OUTSIDE
# both repos, so walk up from here looking for the usual extract location; the
# check SKIPs cleanly when it is not on this machine.
$effect = $EffectRif
if (-not $effect) {
    $probe = $here
    for ($i = 0; $i -lt 8 -and $probe; $i++) {
        $cand = Join-Path $probe 'local-tools\data-loose\data\effect\effect.rif'
        if (Test-Path $cand) { $effect = (Resolve-Path $cand).Path; break }
        $probe = Split-Path -Parent $probe
    }
}
if ($effect) { Write-Host "effect.rif for the art measurement: $effect" }
else         { Write-Host 'effect.rif not found - the art measurement will SKIP' }

$suites = @(
    @{ name = 'test_data_hash';   extra = '' },
    @{ name = 'test_data_loader'; extra = "/I`"$here\shim`" /I`"$OutDir`" /I`"$here\..\..\windward\wind22\include`" shlwapi.lib" },
    @{ name = 'test_data_net';    extra = '' },
    @{ name = 'test_data_expl';   extra = '' }
)

if ($RealDataDir) {
    $resolvedData = (Resolve-Path $RealDataDir).Path
    if (-not (Test-Path (Join-Path $resolvedData 'manifest.txt'))) {
        Write-Error "RealDataDir must point at the extracted data/ directory containing manifest.txt: $resolvedData"
        exit 2
    }
    $sourceManifestHash = (Get-FileHash -LiteralPath (Join-Path $resolvedData 'manifest.txt') -Algorithm SHA256).Hash
    $copyRoot = Join-Path $OutDir 'real-data-copy'
    $copyData = Join-Path $copyRoot 'data'
    if (Test-Path $copyRoot) {
        $copyManifestHash = if (Test-Path (Join-Path $copyData 'manifest.txt')) {
            (Get-FileHash -LiteralPath (Join-Path $copyData 'manifest.txt') -Algorithm SHA256).Hash
        } else { '' }
        if ($copyManifestHash -ne $sourceManifestHash -or -not (Test-LooseDataCopy $copyData)) {
            Write-Error "Existing real-data fixture copy is dirty or incomplete; choose a fresh OutDir: $copyData"
            exit 2
        }
        Write-Host "Reusing verified real-data fixture copy: $copyData"
    } else {
        New-Item -ItemType Directory -Force -Path $copyRoot | Out-Null
        Copy-Item -LiteralPath $resolvedData -Destination $copyData -Recurse
        if ((Get-FileHash -LiteralPath (Join-Path $copyData 'manifest.txt') -Algorithm SHA256).Hash -ne $sourceManifestHash -or
            -not (Test-LooseDataCopy $copyData)) {
            Write-Error "Copied real-data fixture failed manifest verification: $copyData"
            exit 2
        }
    }
    $aggregateExtra = "/I`"$here\shim`" /I`"$OutDir`" /I`"$here\..\..\windward\wind22\include`" /I`"$here\..\..\enations_latest\src`" shlwapi.lib"
    $suites += @{ name = 'test_data_aggregate'; extra = $aggregateExtra }
}

$failed = 0
foreach ($opt in @('/Od', '/O2')) {
    foreach ($s in $suites) {
        $src = Join-Path $here "$($s.name).cpp"
        if (-not (Test-Path $src)) { Write-Host "[$($s.name)] SKIP (no source)"; continue }

        $tag = "$($s.name)$($opt.Replace('/',''))"
        $exe = Join-Path $OutDir "$tag.exe"
        # _CRT_SECURE_NO_WARNINGS: the shipped datafile.cpp uses fopen/_strlwr and
        # the fixtures use getenv. Those are the production spellings; the fixture
        # is not the place to argue with them.
        $cl  = "cl /nologo /EHsc /std:c++17 /W4 /D_CRT_SECURE_NO_WARNINGS $opt `"$src`" /Fo`"$OutDir\$tag.obj`" /Fe`"$exe`" $($s.extra)"

        Write-Host "=== $($s.name) $opt ==="
        cmd /c "`"$vcvars`" >nul 2>&1 && $cl"
        if ($LASTEXITCODE -ne 0) { Write-Error "compile failed: $($s.name) $opt"; exit 2 }

        # Scratch dir via the environment, never argv: CDataFile::Init treats the
        # first command-line argument as a .dat path.
        $env:EN_DATA_TEST_DIR = Join-Path $OutDir "$tag.work"
        # Repo root for the source lints in test_data_expl.
        $env:EN_REPO_ROOT = (Resolve-Path (Join-Path $here '..\..')).Path
        $env:EN_EFFECT_RIF = $effect
        if ($s.name -eq 'test_data_aggregate') {
            $env:EN_REAL_DATA_DIR = Join-Path $OutDir 'real-data-copy\data'
        }
        & $exe
        if ($LASTEXITCODE -ne 0) { $failed = 1 }
        Remove-Item Env:\EN_REAL_DATA_DIR -ErrorAction SilentlyContinue
    }
}

if ($RealDataDir -and ((Get-FileHash -LiteralPath (Join-Path $OutDir 'real-data-copy\data\manifest.txt') -Algorithm SHA256).Hash -ne $sourceManifestHash -or
    -not (Test-LooseDataCopy (Join-Path $OutDir 'real-data-copy\data')))) {
    Write-Error 'Real-data fixture copy was not restored to manifest bytes after the hash checks.'
    exit 1
}
if ($failed -ne 0) { exit 1 }
Write-Host 'ALL DATA SUITES PASSED (/Od and /O2)'
exit 0
