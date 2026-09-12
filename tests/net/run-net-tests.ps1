# run-net-tests.ps1 -- compile & run standalone tests for the net-hygiene fixes
# (tools/vdmplay/tcpnet.cpp extractHostPart / TranslateAddressString, and
# enations_latest/src/racedata.cpp CInitData::Set).
#
# Same philosophy as tests/ai/run-ai-tests.ps1: fully self-contained, invokes
# cl.exe directly, does NOT touch the game build, CMake, or any game source --
# safe to run while the main game is being built or developed. Artifacts land
# outside the tree (d:\tmp\nettests).
#
# Rather than hand-copy the algorithm into the test (which drifts silently the
# next time someone touches the real function), this script EXTRACTS the
# shipped function bodies verbatim out of the real .cpp files at run time --
# byte-for-byte from the opening brace to the matching close -- and compiles
# those bodies into a small generated harness with local mock types standing
# in for the real classes (CTcpNet::TCPAddress / CRaceDef / CInitData) so the
# test never links the whole engine. If a signature ever moves or is renamed,
# extraction fails loudly (exit 2) instead of silently testing stale text.
#
# Exit codes: 0 all pass, 1 a test failed, 2 toolchain / extraction error.

param()

$ErrorActionPreference = 'Stop'
$here     = Split-Path -Parent $MyInvocation.MyCommand.Path
$repoRoot = (Resolve-Path (Join-Path $here '..\..')).Path

# Same VS 2022 roots build.ps1 / run-ai-tests.ps1 key off of.
$roots = @(
    'C:\Program Files\Microsoft Visual Studio\2022\Community',
    'C:\Program Files\Microsoft Visual Studio\2022\Enterprise',
    'C:\Program Files\Microsoft Visual Studio\2022\Professional'
)
$vs = $roots | Where-Object { Test-Path (Join-Path $_ 'VC\Auxiliary\Build\vcvars64.bat') } | Select-Object -First 1
if (-not $vs) { Write-Error 'VS 2022 vcvars64.bat not found (edit roots in run-net-tests.ps1).'; exit 2 }
$vcvars = Join-Path $vs 'VC\Auxiliary\Build\vcvars64.bat'

$outDir = 'd:\tmp\nettests'
New-Item -ItemType Directory -Force -Path $outDir | Out-Null

$tcpnetPath  = Join-Path $repoRoot 'tools\vdmplay\tcpnet.cpp'
$racedataCpp = Join-Path $repoRoot 'enations_latest\src\racedata.cpp'
$racedataH   = Join-Path $repoRoot 'enations_latest\src\racedata.h'

foreach ($p in @($tcpnetPath, $racedataCpp, $racedataH)) {
    if (-not (Test-Path $p)) { Write-Error "missing source: $p"; exit 2 }
}

# ---------------------------------------------------------------------------
# Brace-matched extraction: find the signature line, then walk forward
# counting '{'/'}' until balanced. Good enough here because none of these
# four functions has a brace inside a string or comment (checked by hand).
# ---------------------------------------------------------------------------
function Get-FunctionBody {
    param([string[]]$Lines, [string]$Pattern, [string]$Label)

    $startIdx = -1
    for ($i = 0; $i -lt $Lines.Count; $i++) {
        if ($Lines[$i] -match $Pattern) { $startIdx = $i; break }
    }
    if ($startIdx -lt 0) {
        throw "extraction failed ($Label): signature not found -- source may have moved or been renamed"
    }

    $depth = 0
    $started = $false
    $body = New-Object System.Collections.Generic.List[string]
    for ($i = $startIdx; $i -lt $Lines.Count; $i++) {
        $line = $Lines[$i]
        $body.Add($line)
        foreach ($ch in $line.ToCharArray()) {
            if ($ch -eq '{') { $depth++; $started = $true }
            elseif ($ch -eq '}') { $depth-- }
        }
        if ($started -and $depth -eq 0) { break }
    }
    if (-not $started -or $depth -ne 0) {
        throw "extraction failed ($Label): unbalanced braces while scanning from line $($startIdx+1)"
    }
    return ($body -join "`n")
}

$tcpnetLines      = Get-Content -LiteralPath $tcpnetPath
$racedataCppLines = Get-Content -LiteralPath $racedataCpp
$racedataHLines   = Get-Content -LiteralPath $racedataH

$extractHostPartBody = Get-FunctionBody -Lines $tcpnetLines -Pattern '^BOOL CTcpNet::TCPAddress::extractHostPart' -Label 'extractHostPart'
$extractPortPartBody = Get-FunctionBody -Lines $tcpnetLines -Pattern '^void CTcpNet::TCPAddress::extractPortPart' -Label 'extractPortPart'
$translateAddrBody   = Get-FunctionBody -Lines $tcpnetLines -Pattern '^BOOL CTcpNet::TCPAddress::TranslateAddressString' -Label 'TranslateAddressString'
$initDataSetBody     = Get-FunctionBody -Lines $racedataCppLines -Pattern '^void CInitData::Set' -Label 'CInitData::Set'

$numStartTypesLine = $racedataHLines | Where-Object { $_ -match '^const int NUM_START_TYPES\s*=' } | Select-Object -First 1
if (-not $numStartTypesLine) { Write-Error 'extraction failed: NUM_START_TYPES not found in racedata.h'; exit 2 }

# These three become free functions in the harness (no CTcpNet::TCPAddress
# class is reproduced) -- only the qualifier is stripped; everything from the
# opening brace on is untouched, verbatim shipped text.
$extractHostPartBody = $extractHostPartBody -replace '^BOOL CTcpNet::TCPAddress::extractHostPart', 'BOOL extractHostPart'
$extractPortPartBody = $extractPortPartBody -replace '^void CTcpNet::TCPAddress::extractPortPart', 'void extractPortPart'
$translateAddrBody   = $translateAddrBody   -replace '^BOOL CTcpNet::TCPAddress::TranslateAddressString', 'BOOL TranslateAddressString'
# CInitData::Set keeps its qualifier; the harness below declares a matching
# (mock) CInitData class with the same member names, so the extracted method
# definition binds to it exactly as it does to the real class.

Write-Host "[net-tests] extracted bodies: extractHostPart=$($extractHostPartBody.Length)ch extractPortPart=$($extractPortPartBody.Length)ch TranslateAddressString=$($translateAddrBody.Length)ch CInitData::Set=$($initDataSetBody.Length)ch"

# ---------------------------------------------------------------------------
# Assemble the generated harness .cpp
# ---------------------------------------------------------------------------
$template = @'
// GENERATED by tests/net/run-net-tests.ps1 -- do not hand-edit.
// The four function bodies below are extracted VERBATIM from the shipped
// source at test-run time (see the .ps1), so this test tracks the real fix
// rather than a hand-copied mirror that can silently drift.
#include <winsock2.h>
#include <windows.h>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cctype>
#pragma comment(lib, "ws2_32.lib")

#define VPTRACE(args) ((void)0)
#define _fmemcpy memcpy

struct tcpaddress_s {
    in_addr m_stationAddress;
    u_short m_streamPort;
    u_short m_dgPort;
};

/* ---- extractHostPart (tools/vdmplay/tcpnet.cpp) ---- */
__EXTRACT_HOST_PART__

/* ---- extractPortPart (tools/vdmplay/tcpnet.cpp) ---- */
__EXTRACT_PORT_PART__

/* ---- TranslateAddressString (tools/vdmplay/tcpnet.cpp) ---- */
__TRANSLATE_ADDR__

/* ---- NUM_START_TYPES (enations_latest/src/racedata.h) ---- */
__NUM_START_TYPES_LINE__

// Arbitrary "supplies" row width -- unrelated to the real CRaceDef::num_supplies.
// Only the FIRST-dimension bound (NUM_START_TYPES) is under test here; the row
// width just needs to be consistent between the two mock structs below so the
// memcpy sizes used inside the extracted Set body line up.
static const int TEST_ROW_WIDTH = 16;

struct CRaceDef {
    float         m_fRace[TEST_ROW_WIDTH];
    unsigned long m_iPos[NUM_START_TYPES][TEST_ROW_WIDTH];
};

struct CInitData {
    float         m_fRace[TEST_ROW_WIDTH];
    unsigned long m_iPos[TEST_ROW_WIDTH];
    void Set(CRaceDef const* pRd, int iTyp);
};

/* ---- CInitData::Set (enations_latest/src/racedata.cpp) ---- */
__INITDATA_SET__

// ===========================================================================
// Test harness
// ===========================================================================
static int g_checks = 0, g_fails = 0;
#define CHECK(expr) do { \
    ++g_checks; \
    if (!(expr)) { ++g_fails; std::printf("FAIL %s:%d  CHECK(%s)\n", __FILE__, __LINE__, #expr); } \
} while (0)

// The real overflow (tools/vdmplay/tcpnet.cpp:487, pre-fix) wrote hostPart[len]
// on a buffer of size len -- i.e. exactly ONE byte past hostpart[128]. Pack the
// canary directly against the buffer so there is no padding for that byte to
// hide in, and use several bytes so a slightly-larger regression still gets caught.
#pragma pack(push, 1)
struct CanaryBuf {
    char hostpart[128];
    unsigned char canary[8];
};
#pragma pack(pop)

static void FillCanary(CanaryBuf& b) {
    memset(b.hostpart, 'x', sizeof(b.hostpart));
    memset(b.canary, 0xAB, sizeof(b.canary));
}
static bool CanaryIntact(const CanaryBuf& b) {
    for (size_t i = 0; i < sizeof(b.canary); ++i)
        if (b.canary[i] != 0xAB) return false;
    return true;
}

static void test_extract_host_part_terminates_at_sLen_no_overflow() {
    struct Case { const char* input; const char* expectHost; };
    Case cases[] = {
        { "192.168.0.134",           "192.168.0.134" },  // no colon
        { "192.168.0.134:2347",      "192.168.0.134" },  // colon only
        { "192.168.0.134:2347,2346", "192.168.0.134" },  // colon + comma
    };
    for (const auto& c : cases) {
        CanaryBuf buf;
        FillCanary(buf);
        BOOL ok = extractHostPart(buf.hostpart, sizeof(buf.hostpart), c.input);
        CHECK(ok == TRUE);
        CHECK(CanaryIntact(buf));
        CHECK(strcmp(buf.hostpart, c.expectHost) == 0);
    }
}

static void test_translate_address_string_rejects_malformed() {
    // Mac3's second-witness list (mac3-inet-addr-second-witness.txt): every one
    // of these is digits-and-dots only, so it takes inet_addr's numeric branch;
    // every one is malformed, so it must be REJECTED (FALSE) rather than
    // silently accepted as the broadcast address.
    const char* malformed[] = {
        "54.219.190.", "192.168.0.", "1.2.3.4.5", "999.1.1.1",
        "1..2.3", ".", ".."
    };
    for (const char* s : malformed) {
        tcpaddress_s addr;
        BOOL ok = TranslateAddressString(addr, s);
        CHECK(ok == FALSE);
    }
}

static void test_translate_address_string_accepts_valid() {
    // Regression guard: don't overcorrect -- real addresses must still parse.
    const char* valid[] = { "54.219.190.35", "192.168.0.134" };
    for (const char* s : valid) {
        tcpaddress_s addr;
        BOOL ok = TranslateAddressString(addr, s);
        CHECK(ok == TRUE);
        CHECK(addr.m_stationAddress.s_addr != INADDR_NONE);
    }
}

static void test_initdata_set_clamps_bad_start_index() {
    CRaceDef rd;
    for (int t = 0; t < NUM_START_TYPES; ++t)
        for (int s = 0; s < TEST_ROW_WIDTH; ++s)
            rd.m_iPos[t][s] = (unsigned long)(t * 1000 + s);
    for (int i = 0; i < TEST_ROW_WIDTH; ++i)
        rd.m_fRace[i] = 1.0f + (float)i;

    CInitData base;
    base.Set(&rd, 0);   // valid index -- baseline row

    int badVals[] = { 5, 1000, -1 };
    for (int bad : badVals) {
        CInitData got;
        got.Set(&rd, bad);   // out of [0, NUM_START_TYPES) -- must clamp to 0
        // Reaching this comparison at all (rather than faulting) plus getting
        // EXACTLY row 0's contents is the proof: no clamp would either crash
        // (2D array indexed 1000 rows out) or read a different row's garbage.
        CHECK(memcmp(got.m_iPos, base.m_iPos, sizeof(base.m_iPos)) == 0);
    }
}

int main() {
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    test_extract_host_part_terminates_at_sLen_no_overflow();
    test_translate_address_string_rejects_malformed();
    test_translate_address_string_accepts_valid();
    test_initdata_set_clamps_bad_start_index();

    WSACleanup();

    std::printf("\n[net-tests] %d checks, %d failure%s\n", g_checks, g_fails, g_fails == 1 ? "" : "s");
    return g_fails == 0 ? 0 : 1;
}
'@

# Plain literal substitution (String.Replace, NOT -replace/regex) -- the
# extracted C++ text is inserted byte-for-byte with no regex/backreference
# interpretation of anything it happens to contain.
$generated = $template.
    Replace('__EXTRACT_HOST_PART__', $extractHostPartBody).
    Replace('__EXTRACT_PORT_PART__', $extractPortPartBody).
    Replace('__TRANSLATE_ADDR__', $translateAddrBody).
    Replace('__NUM_START_TYPES_LINE__', $numStartTypesLine).
    Replace('__INITDATA_SET__', $initDataSetBody)

$genPath = Join-Path $outDir 'net_tests_gen.cpp'
Set-Content -LiteralPath $genPath -Value $generated -Encoding utf8

# ---------------------------------------------------------------------------
# Compile at /Od and /O2, run both, combine results.
# ---------------------------------------------------------------------------
$overallExit = 0
foreach ($opt in @('/Od', '/O2')) {
    $tag     = $opt.TrimStart('/')
    $objPath = Join-Path $outDir "net_tests_$tag.obj"
    $exePath = Join-Path $outDir "net_tests_$tag.exe"

    $clCmd = "cl /nologo /EHsc /std:c++17 /W4 $opt `"$genPath`" /Fo`"$objPath`" /Fe`"$exePath`""
    cmd /c "`"$vcvars`" >nul 2>&1 && $clCmd"
    if ($LASTEXITCODE -ne 0) {
        Write-Host "[net-tests] COMPILE FAILED ($tag)"
        exit 2
    }

    Write-Host "--- running ($tag) ---"
    # Route through cmd /c rather than invoking the exe directly: the test exe
    # deliberately writes to stderr (the CInitData::Set clamp log line, working
    # as intended), and PowerShell's own stderr handling for native commands
    # turns that into a terminating NativeCommandError under $ErrorActionPreference
    # = 'Stop' -- cmd.exe does not re-interpret it, and $LASTEXITCODE still comes
    # through correctly.
    cmd /c "`"$exePath`""
    $runExit = $LASTEXITCODE
    if ($runExit -ne 0) { $overallExit = 1 }
}

exit $overallExit
