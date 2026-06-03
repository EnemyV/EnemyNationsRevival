# mfc-status.ps1 — MFC removal progress dashboard.
# Run this after every commit to see how close we are to "no MFC linked".
#
# Usage:
#   ./mfc-status.ps1            # human-readable table
#   ./mfc-status.ps1 -Json      # machine-readable
#   ./mfc-status.ps1 -Verbose   # include file lists
#
# Exit code: always 0 (this is a report tool).

[CmdletBinding()]
param(
    [switch]$Json
)

$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $MyInvocation.MyCommand.Path

# Resolve dumpbin (lives in VC tools)
function Resolve-Dumpbin {
    $vcRoot = 'C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC'
    if (-not (Test-Path $vcRoot)) { return $null }
    $latest = Get-ChildItem $vcRoot -Directory | Sort-Object Name -Descending | Select-Object -First 1
    if (-not $latest) { return $null }
    $candidates = @(
        (Join-Path $latest.FullName 'bin\Hostx64\x86\dumpbin.exe'),
        (Join-Path $latest.FullName 'bin\Hostx86\x86\dumpbin.exe'),
        (Join-Path $latest.FullName 'bin\Hostx64\x64\dumpbin.exe')
    )
    foreach ($c in $candidates) { if (Test-Path $c) { return $c } }
    return $null
}

# Count MFC imports in the latest enations.exe we can find
function Get-MfcImportInfo {
    $dumpbin = Resolve-Dumpbin
    $result = [ordered]@{
        exe_path = $null
        exe_config = $null
        mfc_dll_found = $false
        mfc_dll_name = $null
        mfc_import_count = 0
    }
    if (-not $dumpbin) { return $result }

    $candidates = @(
        @{ path = Join-Path $repoRoot 'cmakeBuild\enations_latest\src\Release\enations.exe'; cfg = 'Release' },
        @{ path = Join-Path $repoRoot 'cmakeBuild\enations_latest\src\Debug\enations.exe'; cfg = 'Debug' }
    )
    $exe = $null
    foreach ($c in $candidates) {
        if (Test-Path $c.path) { $exe = $c; break }
    }
    if (-not $exe) { return $result }
    $result.exe_path = $exe.path
    $result.exe_config = $exe.cfg

    $imports = & $dumpbin /imports $exe.path 2>$null | Out-String
    # Look for mfc*.dll line
    if ($imports -match '(?im)^\s+(mfc\d+u?d?\.dll)') {
        $result.mfc_dll_found = $true
        $result.mfc_dll_name = $matches[1]
        # Count the function names listed under that section
        # Imports look like:
        #     mfc140.dll
        #         <hint>  <funcname>
        #         <hint>  <funcname>
        #     next.dll
        $lines = $imports -split "`r?`n"
        $inMfcSection = $false
        $count = 0
        foreach ($line in $lines) {
            if ($line -match '(?i)^\s+(mfc\d+u?d?\.dll)') { $inMfcSection = $true; continue }
            if ($inMfcSection) {
                if ($line -match '^\s+(\w+\.dll)') { break }  # next DLL
                if ($line -match '^\s+[0-9A-Fa-f]+\s+\S+') { $count++ }
            }
        }
        $result.mfc_import_count = $count
    }
    return $result
}

# Search roots — live code only. enations/src is the old snapshot, ignored.
$liveRoots = @(
    (Join-Path $repoRoot 'enations_latest\src'),
    (Join-Path $repoRoot 'windward\wind22\src'),
    (Join-Path $repoRoot 'windward\wind22\include')
)

# Files to consider: .cpp / .h, excluding cmakeBuild output
function Get-LiveFiles {
    $files = @()
    foreach ($r in $liveRoots) {
        if (-not (Test-Path $r)) { continue }
        $files += Get-ChildItem -Path $r -Recurse -File -Include *.cpp,*.h -ErrorAction SilentlyContinue |
                  Where-Object { $_.FullName -notmatch '\\cmakeBuild\\' }
    }
    return $files
}

# Count regex matches across a set of files; returns @{ total = N; files = M }
function Count-Pattern {
    param([string]$pattern, [array]$files, [switch]$WholeWord)
    $effective = if ($WholeWord) { "\b$pattern\b" } else { $pattern }
    $total = 0
    $matched = 0
    foreach ($f in $files) {
        try {
            $hits = (Select-String -Path $f.FullName -Pattern $effective -AllMatches -ErrorAction SilentlyContinue).Matches.Count
            if ($hits -gt 0) { $matched++; $total += $hits }
        } catch { }
    }
    return @{ total = $total; files = $matched }
}

Write-Verbose 'Scanning live source tree...'
$liveFiles = Get-LiveFiles
Write-Verbose ("Found {0} live files" -f $liveFiles.Count)

$mfcImports = Get-MfcImportInfo
$cstring   = Count-Pattern -pattern 'CString'        -files $liveFiles -WholeWord
$cfile     = Count-Pattern -pattern 'CFile'          -files $liveFiles -WholeWord
$crect     = Count-Pattern -pattern 'CRect'          -files $liveFiles -WholeWord
$cdialog   = Count-Pattern -pattern 'CDialog'        -files $liveFiles -WholeWord
$afxmsg    = Count-Pattern -pattern 'afx_msg'        -files $liveFiles
$msgmap    = Count-Pattern -pattern 'BEGIN_MESSAGE_MAP' -files $liveFiles
$afxinc    = Count-Pattern -pattern '#include\s*<afx\w*\.h>' -files $liveFiles
$clist     = Count-Pattern -pattern 'CList<'         -files $liveFiles
$cmap      = Count-Pattern -pattern 'CMap<'          -files $liveFiles
$carray    = Count-Pattern -pattern 'CArray<'        -files $liveFiles
$cwinapp   = Count-Pattern -pattern ':\s*public\s+CWinApp' -files $liveFiles

# Build report
$report = [ordered]@{
    timestamp = (Get-Date -Format 'yyyy-MM-dd HH:mm:ss')
    binary = [ordered]@{
        exe            = $mfcImports.exe_path
        config         = $mfcImports.exe_config
        mfc_linked     = $mfcImports.mfc_dll_found
        mfc_dll        = $mfcImports.mfc_dll_name
        mfc_imports    = $mfcImports.mfc_import_count
    }
    source_counts = [ordered]@{
        # The finish-line metrics
        afx_includes        = $afxinc
        message_maps        = $msgmap
        afx_msg_decorations = $afxmsg
        cwinapp_subclasses  = $cwinapp
        # Type usage
        CString             = $cstring
        CFile               = $cfile
        CRect               = $crect
        CDialog             = $cdialog
        CList               = $clist
        CMap                = $cmap
        CArray              = $carray
    }
    files_scanned = $liveFiles.Count
}

if ($Json) {
    $report | ConvertTo-Json -Depth 5
    exit 0
}

# Human-readable
Write-Output ''
Write-Output '=== MFC removal status ==='
Write-Output "  scanned at:  $($report.timestamp)"
Write-Output "  live files:  $($report.files_scanned)"
Write-Output ''
Write-Output '--- Binary (the finish line) ---'
if ($mfcImports.exe_path) {
    Write-Output ("  exe:                 {0} ({1})" -f $mfcImports.exe_path, $mfcImports.exe_config)
    if ($mfcImports.mfc_dll_found) {
        Write-Output ("  mfc linked:          YES  ({0}, {1} imports)" -f $mfcImports.mfc_dll_name, $mfcImports.mfc_import_count)
    } else {
        Write-Output  '  mfc linked:          NO  *** finish line crossed ***'
    }
} else {
    Write-Output '  exe:                 (not built yet — run ./build.ps1 first)'
}
Write-Output ''
Write-Output '--- Source surface (live tree only) ---'
$rows = @(
    @{ label = '#include <afx*.h>';      data = $afxinc },
    @{ label = 'BEGIN_MESSAGE_MAP';      data = $msgmap },
    @{ label = 'afx_msg decorations';    data = $afxmsg },
    @{ label = ': public CWinApp';       data = $cwinapp },
    @{ label = 'CString';                data = $cstring },
    @{ label = 'CFile';                  data = $cfile },
    @{ label = 'CRect';                  data = $crect },
    @{ label = 'CDialog';                data = $cdialog },
    @{ label = 'CList<';                 data = $clist },
    @{ label = 'CMap<';                  data = $cmap },
    @{ label = 'CArray<';                data = $carray }
)
foreach ($r in $rows) {
    Write-Output ("  {0,-25} {1,5} refs in {2,3} files" -f $r.label, $r.data.total, $r.data.files)
}
Write-Output ''
Write-Output 'Tip: run ./build.ps1 -Release first if exe is stale, then re-run.'
exit 0
