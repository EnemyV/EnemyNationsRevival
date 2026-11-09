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
#   test_data_expl_operate  the shipped CExplosion::Operate body against mocks
#   test_projmap_capacity  the shipped CProjMap projectile occupancy query
#   test_sprite_fallback  the shipped CSpriteCollection::GetSprite lookup
#   test_net_plyrjoin  the shipped FitsBuffer size checks and their senders
#   test_net_xfer     the shipped load-join save transfer (dxfer.cpp, vpxfer.cpp)
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

function Get-NormalizedPath([string]$Path) {
    if (Test-Path -LiteralPath $Path) {
        $full = [IO.Path]::GetFullPath((Resolve-Path -LiteralPath $Path).Path)
    } else {
        $full = [IO.Path]::GetFullPath($Path)
    }
    $root = [IO.Path]::GetPathRoot($full)
    if ($full.Length -gt $root.Length) { $full = $full.TrimEnd('\', '/') }
    return $full
}

function Test-PathOverlap([string]$PathA, [string]$PathB) {
    $a = Get-NormalizedPath $PathA
    $b = Get-NormalizedPath $PathB
    $sep = [IO.Path]::DirectorySeparatorChar
    $aPrefix = if ($a.EndsWith([string]$sep)) { $a } else { $a + $sep }
    $bPrefix = if ($b.EndsWith([string]$sep)) { $b } else { $b + $sep }
    return [string]::Equals($a, $b, [StringComparison]::OrdinalIgnoreCase) -or
        $a.StartsWith($bPrefix, [StringComparison]::OrdinalIgnoreCase) -or
        $b.StartsWith($aPrefix, [StringComparison]::OrdinalIgnoreCase)
}

function Test-PathHasReparseAncestor([string]$Path) {
    $current = Get-NormalizedPath $Path
    while ($current) {
        if (Test-Path -LiteralPath $current) {
            $item = Get-Item -Force -LiteralPath $current
            if (($item.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) { return $true }
        }
        $parent = Split-Path -Parent $current
        if (-not $parent -or $parent -eq $current) { break }
        $current = $parent
    }
    return $false
}

# The test runner copies RealDataDir and then edits the private copy. Reject any
# source/output overlap before creating OutDir: otherwise a repeated run could
# treat its own scratch copy as source, or recurse while copying into itself.
if ($RealDataDir) {
    if (-not (Test-Path -LiteralPath $RealDataDir -PathType Container)) {
        Write-Error "RealDataDir does not exist: $RealDataDir"
        exit 2
    }
    if ((Test-PathHasReparseAncestor $RealDataDir) -or (Test-PathHasReparseAncestor $OutDir)) {
        Write-Error 'RealDataDir and OutDir must not pass through symlinks, junctions, or other reparse points.'
        exit 2
    }
    if (Test-PathOverlap $RealDataDir $OutDir) {
        Write-Error "RealDataDir and OutDir overlap; choose disjoint paths. Source='$((Get-NormalizedPath $RealDataDir))' OutDir='$((Get-NormalizedPath $OutDir))'"
        exit 2
    }
}

# Tests change their working directory while checking the profile pin. Pass an
# absolute scratch path to the test processes so a relative OutDir cannot become
# invalid halfway through a test.
$OutDir = Get-NormalizedPath $OutDir

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
    @{ name = 'test_data_expl';   extra = '' },
    @{ name = 'test_data_expl_operate'; extra = "/I`"$here`"" },
    @{ name = 'test_projmap_capacity'; extra = "/I`"$here`"" },
    @{ name = 'test_sprite_fallback'; extra = "/I`"$here`"" },
    @{ name = 'test_net_plyrjoin'; extra = "/I`"$here`"" },
    @{ name = 'test_net_xfer'; extra = "/I`"$here`"" }
)

# Compile the actual shipped CExplosion::Operate body against the tiny mock
# world in tests/data. Regenerate it every run so the fixture cannot silently
# drift into a hand-maintained copy of the production lifecycle.
$projbasePath = Join-Path $here '..\..\enations_latest\src\projbase.cpp'
$projbaseText = Get-Content -Raw -LiteralPath $projbasePath
$operateStart = $projbaseText.IndexOf('void CExplosion::Operate ()', [StringComparison]::Ordinal)
$operateEnd = if ($operateStart -ge 0) {
    $projbaseText.IndexOf('void CVehicle::HandleCombat ()', $operateStart, [StringComparison]::Ordinal)
} else { -1 }
if ($operateStart -lt 0 -or $operateEnd -le $operateStart) {
    Write-Error 'Could not locate CExplosion::Operate boundaries in projbase.cpp.'
    exit 2
}
$operateGenerated = Join-Path $OutDir 'test_data_expl_operate_generated.cpp'
$operateText = '#include "test_data_expl_operate_prefix.h"' + "`r`n`r`n" +
    $projbaseText.Substring($operateStart, $operateEnd - $operateStart) +
    "`r`n#include `"test_data_expl_operate_suffix.h`"`r`n"
Set-Content -Encoding ascii -NoNewline -LiteralPath $operateGenerated -Value $operateText
$operateSuite = $suites | Where-Object { $_.name -eq 'test_data_expl_operate' }
$operateSuite.source = $operateGenerated

# Compile the actual shipped CProjMap capacity query against a minimal map/list
# model. Regenerate it every run so the test exercises production code.
$capacityStart = $projbaseText.IndexOf('int CProjMap::GetProjectileHexCount', [StringComparison]::Ordinal)
$capacityEnd = if ($capacityStart -ge 0) {
    $projbaseText.IndexOf('void CProjMap::Add', $capacityStart, [StringComparison]::Ordinal)
} else { -1 }
if ($capacityStart -lt 0 -or $capacityEnd -le $capacityStart) {
    Write-Error 'Could not locate CProjMap::GetProjectileHexCount boundaries in projbase.cpp.'
    exit 2
}
$capacityGenerated = Join-Path $OutDir 'test_projmap_capacity_generated.cpp'
$capacityText = '#include "test_projmap_capacity_prefix.h"' + "`r`n`r`n" +
    $projbaseText.Substring($capacityStart, $capacityEnd - $capacityStart) +
    "`r`n#include `"test_projmap_capacity_suffix.h`"`r`n"
Set-Content -Encoding ascii -NoNewline -LiteralPath $capacityGenerated -Value $capacityText
$capacitySuite = $suites | Where-Object { $_.name -eq 'test_projmap_capacity' }
$capacitySuite.source = $capacityGenerated

# Compile the mutable production GetSprite overload against a compact mock
# collection. The fallback behavior must track the shipped method on each run.
$spritePath = Join-Path $here '..\..\enations_latest\src\sprite.cpp'
$spriteText = Get-Content -Raw -LiteralPath $spritePath
$spriteFirst = $spriteText.IndexOf('CSpriteCollection::GetSprite(', [StringComparison]::Ordinal)
$spriteSecond = if ($spriteFirst -ge 0) {
    $spriteText.IndexOf('CSpriteCollection::GetSprite(', $spriteFirst + 1, [StringComparison]::Ordinal)
} else { -1 }
$spriteStart = if ($spriteSecond -ge 0) {
    $spriteText.LastIndexOf('CSprite *', $spriteSecond, [StringComparison]::Ordinal)
} else { -1 }
$spriteEnd = if ($spriteSecond -ge 0) {
    $spriteText.IndexOf('// CSpriteCollection::GetCount', $spriteSecond, [StringComparison]::Ordinal)
} else { -1 }
if ($spriteStart -lt 0 -or $spriteEnd -le $spriteStart) {
    Write-Error 'Could not locate mutable CSpriteCollection::GetSprite boundaries in sprite.cpp.'
    exit 2
}
$spriteGenerated = Join-Path $OutDir 'test_sprite_fallback_generated.cpp'
$spriteHeaderPath = Join-Path $here '..\..\enations_latest\src\sprite.h'
$spriteHeaderText = Get-Content -Raw -LiteralPath $spriteHeaderPath
$getNumViewsMatch = [regex]::Match($spriteHeaderText, 'int\s+GetNumViews\(\)\s+const\s*\{[^}]*\}', [Text.RegularExpressions.RegexOptions]::Singleline)
if (-not $getNumViewsMatch.Success) {
    Write-Error 'Could not locate CSprite::GetNumViews in sprite.h.'
    exit 2
}
$getNumViewsGenerated = Join-Path $OutDir 'test_sprite_getnumviews_generated.h'
Set-Content -Encoding ascii -NoNewline -LiteralPath $getNumViewsGenerated -Value $getNumViewsMatch.Value
$spriteGeneratedText = '#include "test_sprite_fallback_prefix.h"' + "`r`n`r`n" +
    $spriteText.Substring($spriteStart, $spriteEnd - $spriteStart) +
    "`r`n#include `"test_sprite_fallback_suffix.h`"`r`n"
Set-Content -Encoding ascii -NoNewline -LiteralPath $spriteGenerated -Value $spriteGeneratedText
$spriteSuite = $suites | Where-Object { $_.name -eq 'test_sprite_fallback' }
$spriteSuite.source = $spriteGenerated

# Compile the shipped receive-side size checks: CNetCmd::FitsBuffer, both
# CNetPlyrJoin::Alloc overloads and the senders of the other variable-length
# records (netcmd.cpp, ipcmsg.cpp), against the shipped record classes
# (netcmd.h, ipcmsg.hpp) and CMaterialTypes (base.h), plus the mail receiver
# SDL2Mail_HandleIncoming (SDL2GameDialogs.cpp). Brace-matched, verbatim,
# every run. Every other type FitsBuffer names gets a stand-in class of the size
# wire_layout_assert.cpp pins for it (Release = the wire layout), so a fixed-size
# case is tested against the real number.
function Get-BraceBlock([string[]]$Lines, [string]$Pattern, [string]$Label, [switch]$Optional, [switch]$DropPreproc) {
    $start = -1
    for ($i = 0; $i -lt $Lines.Count; $i++) { if ($Lines[$i] -match $Pattern) { $start = $i; break } }
    if ($start -lt 0) {
        if ($Optional) { return '' }
        throw "extraction failed ($Label): not found"
    }
    $depth = 0; $started = $false
    $out = New-Object System.Collections.Generic.List[string]
    for ($i = $start; $i -lt $Lines.Count; $i++) {
        # -DropPreproc: ipcmsg's #if USE_FAKE_NET alternatives; the match is already
        # the branch the game builds.
        if (-not $DropPreproc -or $Lines[$i] -notmatch '^\s*#') { $out.Add($Lines[$i]) }
        foreach ($ch in $Lines[$i].ToCharArray()) {
            if ($ch -eq '{') { $depth++; $started = $true } elseif ($ch -eq '}') { $depth-- }
        }
        if ($started -and $depth -eq 0) { break }
    }
    if (-not $started -or $depth -ne 0) { throw "extraction failed ($Label): unbalanced braces" }
    return ($out -join "`r`n")
}
$netSrc = Join-Path $here '..\..\enations_latest\src'
$netcmdCpp = Get-Content -LiteralPath (Join-Path $netSrc 'netcmd.cpp')
$netcmdH   = Get-Content -LiteralPath (Join-Path $netSrc 'netcmd.h')
$baseH     = Get-Content -LiteralPath (Join-Path $netSrc 'base.h')
$ipcHpp    = Get-Content -LiteralPath (Join-Path $netSrc 'ipcmsg.hpp')
$ipcCpp    = Get-Content -LiteralPath (Join-Path $netSrc 'ipcmsg.cpp')
$mailCpp   = Get-Content -LiteralPath (Join-Path $netSrc 'SDL2GameDialogs.cpp')
$wireText  = Get-Content -Raw -LiteralPath (Join-Path $netSrc 'wire_layout_assert.cpp')
try {
    $fitsBody   = Get-BraceBlock $netcmdCpp '^BOOL CNetCmd::FitsBuffer\( int cbAvail \) const' 'FitsBuffer'
    $textHelper = Get-BraceBlock $netcmdCpp '^static BOOL TextEndsBefore\(' 'TextEndsBefore' -Optional
    $allocCopy  = Get-BraceBlock $netcmdCpp '^CNetPlyrJoin\* CNetPlyrJoin::Alloc\( const CNetPlyrJoin\* pData \)' 'Alloc(pData)'
    $allocPlyr  = Get-BraceBlock $netcmdCpp '^CNetPlyrJoin\* CNetPlyrJoin::Alloc\( const CPlayer\* pPlyr \)' 'Alloc(pPlyr)'
    $allocNetP  = Get-BraceBlock $netcmdCpp '^CNetPlayer\* CNetCmd::AllocPlayer\(' 'AllocPlayer'
    $allocChat  = Get-BraceBlock $netcmdCpp '^CNetChat\* CNetChat::Alloc\(' 'CNetChat::Alloc'
    $allocAi    = Get-BraceBlock $netcmdCpp '^CMsgAiMsg\* CMsgAiMsg::Alloc\(' 'CMsgAiMsg::Alloc'
    $ctorFile   = Get-BraceBlock $netcmdCpp '^CNetGetFile::CNetGetFile\(' 'CNetGetFile ctor'
    $clsCmd     = Get-BraceBlock $netcmdH '^class CNetCmd : public VPMsgHdr' 'class CNetCmd'
    $clsJoin    = Get-BraceBlock $netcmdH '^class CNetPlyrJoin : public CNetCmd' 'class CNetPlyrJoin'
    $clsEnum    = Get-BraceBlock $netcmdH '^class CNetEnumPlyrs : public CNetCmd' 'class CNetEnumPlyrs'
    $clsSelect  = Get-BraceBlock $netcmdH '^class CNetSelectPlyr : public CNetCmd' 'class CNetSelectPlyr'
    $clsPlayer  = Get-BraceBlock $netcmdH '^class CNetPlayer : public CNetCmd' 'class CNetPlayer'
    $clsChat    = Get-BraceBlock $netcmdH '^class CNetChat : public CNetCmd' 'class CNetChat'
    $clsToHp    = Get-BraceBlock $netcmdH '^class CNetToHp : public CNetCmd' 'class CNetToHp'
    $clsFile    = Get-BraceBlock $netcmdH '^class CNetGetFile : public CNetCmd' 'class CNetGetFile'
    $clsAi      = Get-BraceBlock $netcmdH '^class CMsgAiMsg : public CNetCmd' 'class CMsgAiMsg'
    # Optional: a tree from before the mail wire header has none (and its suite fails at run time).
    $clsIpcWire = Get-BraceBlock $netcmdH '^class CMsgIPCWire : public CNetCmd' 'class CMsgIPCWire' -Optional
    $clsIpc     = Get-BraceBlock $ipcHpp '^class CMsgIPC : public CNetCmd' 'class CMsgIPC' -DropPreproc
    $ctorIpc    = Get-BraceBlock $ipcCpp '^CMsgIPC::CMsgIPC\( int iType \) : CNetCmd' 'CMsgIPC ctor' -DropPreproc
    $dtorIpc    = Get-BraceBlock $ipcCpp '^CMsgIPC::~CMsgIPC\(' 'CMsgIPC dtor'
    $toBufIpc   = Get-BraceBlock $ipcCpp '^char \* CMsgIPC::ToBuf' 'CMsgIPC::ToBuf'
    $mailStruct = Get-BraceBlock $mailCpp '^struct SDL2MailMsg \{' 'struct SDL2MailMsg'
    $mailRecv   = Get-BraceBlock $mailCpp '^void SDL2Mail_HandleIncoming\(CMsgIPC\* pMsg\)' 'SDL2Mail_HandleIncoming'
    $clsMat     = Get-BraceBlock $baseH '^class CMaterialTypes' 'class CMaterialTypes'
} catch { Write-Error $_; exit 2 }
$mailInbox = $mailCpp | Where-Object { $_ -match '^static std::vector<SDL2MailMsg> g_mailInbox;' } | Select-Object -First 1
if (-not $mailInbox) { Write-Error 'extraction failed: g_mailInbox not found in SDL2GameDialogs.cpp'; exit 2 }
$maxFileLine = $netcmdH | Where-Object { $_ -match '^const int MAX_NET_GAME_FILE\s*=' } | Select-Object -First 1
if (-not $maxFileLine) { $maxFileLine = 'const int MAX_NET_GAME_FILE = 256 * 1024 * 1024;   // not in this source; the bound the fix uses' }
# Release pins: the section after #else in wire_layout_assert.cpp.
$pins = @{}
$relText = $wireText.Substring($wireText.IndexOf('#else'))
foreach ($m in [regex]::Matches($relText, 'static_assert\(\s*sizeof\(\s*(\w+)\s*\)\s*==\s*(\d+)')) { $pins[$m.Groups[1].Value] = [int]$m.Groups[2].Value }
if ($pins.Count -lt 50) { Write-Error "extraction failed: only $($pins.Count) wire pins found"; exit 2 }
$pinLines = ($pins.GetEnumerator() | Sort-Object Name | ForEach-Object { '    { "' + $_.Name + '", ' + $_.Value + ' },' }) -join "`r`n"
$mirrored = @('CNetCmd', 'CNetPlyrJoin', 'CNetEnumPlyrs', 'CNetSelectPlyr', 'CNetPlayer', 'CNetChat', 'CNetToHp',
              'CNetGetFile', 'CMsgAiMsg', 'CMsgIPC', 'CMsgIPCWire', 'int', 'char')
$standIns = New-Object System.Collections.Generic.List[string]
$seen = @{}
foreach ($m in [regex]::Matches($fitsBody, 'sizeof\(\s*(\w+)\s*\)|\(\s*const\s+(\w+)\s*\*\s*\)\s*this')) {
    $t = if ($m.Groups[1].Success) { $m.Groups[1].Value } else { $m.Groups[2].Value }
    if ($mirrored -contains $t -or $seen.ContainsKey($t)) { continue }
    $seen[$t] = $true
    if (-not $pins.ContainsKey($t))  { $standIns.Add("class $t : public CNetCmd { public: int m_iNumMsgs; };") }
    elseif ($pins[$t] -gt 16)        { $standIns.Add("class $t : public CNetCmd { public: int m_iNumMsgs; char m_pad[$($pins[$t]) - 16]; };") }
    elseif ($pins[$t] -eq 16)        { $standIns.Add("class $t : public CNetCmd { public: int m_iNumMsgs; };") }
    elseif ($pins[$t] -gt 12)        { $standIns.Add("class $t : public CNetCmd { public: char m_pad[$($pins[$t]) - 12]; };") }
    else                             { $standIns.Add("class $t : public CNetCmd { };") }
}
# ...and every other pinned type, so a test can name a type the source under
# test does not list yet.
foreach ($k in ($pins.Keys | Sort-Object)) {
    if ($mirrored -contains $k -or $seen.ContainsKey($k)) { continue }
    $seen[$k] = $true
    if ($pins[$k] -gt 16)     { $standIns.Add("class $k : public CNetCmd { public: int m_iNumMsgs; char m_pad[$($pins[$k]) - 16]; };") }
    elseif ($pins[$k] -eq 16) { $standIns.Add("class $k : public CNetCmd { public: int m_iNumMsgs; };") }
    elseif ($pins[$k] -gt 12) { $standIns.Add("class $k : public CNetCmd { public: char m_pad[$($pins[$k]) - 12]; };") }
    else                      { $standIns.Add("class $k : public CNetCmd { };") }
}
foreach ($m in [regex]::Matches($fitsBody, '\bNUM_\w+_ELEM\b')) {
    if ($seen.ContainsKey($m.Value)) { continue }
    $seen[$m.Value] = $true
    $standIns.Add("const int $($m.Value) = 1;")
}
$plyrJoinText = @(
    '#include "test_net_plyrjoin_prefix.h"', '',
    $clsMat, '',
    '#pragma pack( push, cnetcmd, 1 )',
    $clsCmd, $clsJoin, $clsEnum, $clsSelect, $clsPlayer, $clsChat, $clsToHp, $maxFileLine, $clsFile, $clsAi, $clsIpcWire,
    ($standIns -join "`r`n"),
    '#pragma pack( pop, cnetcmd )', '',
    $clsIpc, '',
    'struct WirePin { const char* m_pName; int m_iSize; };',
    'static const WirePin s_aWirePin[] = {', $pinLines, '};', '',
    $textHelper, '',
    $fitsBody, '', $allocCopy, '', $allocPlyr, '', $allocNetP, '', $allocChat, '', $allocAi, '', $ctorFile, '',
    $ctorIpc, '', $dtorIpc, '', $toBufIpc, '',
    $mailStruct, $mailInbox, '', $mailRecv, '',
    '#include "test_net_plyrjoin_suffix.h"', ''
) -join "`r`n"
$plyrJoinGenerated = Join-Path $OutDir 'test_net_plyrjoin_generated.cpp'
Set-Content -Encoding ascii -NoNewline -LiteralPath $plyrJoinGenerated -Value $plyrJoinText
$plyrJoinSuite = $suites | Where-Object { $_.name -eq 'test_net_plyrjoin' }
$plyrJoinSuite.source = $plyrJoinGenerated

# Compile the shipped load-join save transfer (dxfer.cpp + vpxfer.cpp, whole
# files) with only their stdafx.h include pointed at test_net_xfer_prefix.h.
# Copied fresh every run so the suite cannot test a stale transfer.
$xferDir = Join-Path $OutDir 'xfer'
New-Item -ItemType Directory -Force -Path $xferDir | Out-Null
foreach ($f in @('dxfer.h', 'vpxfer.h', 'dxfer.cpp', 'vpxfer.cpp')) {
    $src = Join-Path $netSrc $f
    if (-not (Test-Path -LiteralPath $src)) { Write-Error "missing source: $src"; exit 2 }
    $text = (Get-Content -Raw -LiteralPath $src).Replace('#include "stdafx.h"', '#include "test_net_xfer_prefix.h"')
    Set-Content -Encoding ascii -NoNewline -LiteralPath (Join-Path $xferDir $f) -Value $text
}
$xferGenerated = Join-Path $xferDir 'test_net_xfer_generated.cpp'
Set-Content -Encoding ascii -LiteralPath $xferGenerated -Value (@(
    '#include "dxfer.cpp"', '#include "vpxfer.cpp"', '#include "test_net_xfer_suffix.h"') -join "`r`n")
$xferSuite = $suites | Where-Object { $_.name -eq 'test_net_xfer' }
$xferSuite.source = $xferGenerated

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
        $src = if ($s.source) { $s.source } else { Join-Path $here "$($s.name).cpp" }
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
