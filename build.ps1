# build.ps1 — structured MSBuild wrapper for Enemy Nations.
# Returns parsed errors + warnings. Designed for LLM consumption (small,
# parseable output) but readable on the terminal too.
#
# Usage:
#   ./build.ps1                       # Debug Win32, first 10 errors, human output
#   ./build.ps1 -Release              # Release Win32
#   ./build.ps1 -File wndbase.cpp     # Compile one file only (much faster)
#   ./build.ps1 -First 5              # Cap at 5 errors
#   ./build.ps1 -Json                 # Machine-readable JSON
#   ./build.ps1 -Quiet                # Just the one-line summary
#   ./build.ps1 -NoContext            # Skip source context lookup
#
# Exit codes: 0 success, 1 build failed, 2 script error.

[CmdletBinding()]
param(
    [switch]$Release,
    [string]$File,
    [int]$First = 10,
    [switch]$Json,
    [switch]$Quiet,
    [switch]$NoContext,
    [switch]$Rebuild,
    [switch]$x64        # build the 64-bit target (cmakeBuild-x64) instead of Win32
)

$ErrorActionPreference = 'Stop'

# Resolve MSBuild. Prefer 32-bit (matches Win32 target).
$msbuild = 'C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe'
if (-not (Test-Path $msbuild)) {
    $msbuild = 'C:\Program Files\Microsoft Visual Studio\2022\Enterprise\MSBuild\Current\Bin\MSBuild.exe'
}
if (-not (Test-Path $msbuild)) {
    $msbuild = 'C:\Program Files\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe'
}
if (-not (Test-Path $msbuild)) {
    Write-Error "MSBuild.exe not found. Edit build.ps1 to add your VS install path."
    exit 2
}

$repoRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
# Win32 builds from cmakeBuild/ (Platform=Win32); x64 from cmakeBuild-x64/ (Platform=x64).
$buildDir = if ($x64) { 'cmakeBuild-x64' } else { 'cmakeBuild' }
$platform = if ($x64) { 'x64' } else { 'Win32' }
$vcxproj = Join-Path $repoRoot "$buildDir\enations_latest\src\enations.vcxproj"
if (-not (Test-Path $vcxproj)) {
    Write-Error "Project not found: $vcxproj. Run cmake first (cmake -B $buildDir -A $platform)."
    exit 2
}

# Resolve config. Default Debug.
$config = if ($Release) { 'Release' } else { 'Debug' }

# Assemble args
$msbArgs = @(
    $vcxproj,
    "/p:Configuration=$config",
    "/p:Platform=$platform",
    '/nologo',
    '/verbosity:minimal',
    '/m:1'  # single-process for clean output ordering
)
if ($Rebuild) { $msbArgs += '/t:Rebuild' }
if ($File) {
    $msbArgs += '/t:ClCompile'
    $msbArgs += "/p:SelectedFiles=$File"
}

$startTime = Get-Date
$rawOutput = & $msbuild @msbArgs 2>&1 | Out-String -Stream
$elapsed = ((Get-Date) - $startTime).TotalSeconds
$exitCode = $LASTEXITCODE

# Parse MSBuild output. Pattern handles:
#   foo.cpp(123,5): error C2065: ...
#   foo.cpp(123): warning C4800: ...
#   1>foo.cpp(123): fatal error C1xxx: ...
#   foo.obj : error LNK2019: ...
$errPattern = '^(?:\s*\d+>)?\s*(?<file>[^()\r\n]+?)(?:\((?<line>\d+)(?:,(?<col>\d+))?\))?\s*:\s*(?<sev>error|warning|fatal error)\s+(?<code>[A-Z]+\d+):\s*(?<msg>.*?)\s*(?:\[[^\]]+\])?$'

$errors = New-Object System.Collections.ArrayList
$warnings = New-Object System.Collections.ArrayList
$contextCache = @{}

function Get-SourceContext {
    param([string]$path, [int]$line)
    if ($NoContext -or -not $line -or $line -le 0) { return $null }
    if (-not (Test-Path $path)) {
        $alt = Join-Path $repoRoot $path
        if (Test-Path $alt) { $path = $alt } else { return $null }
    }
    $cacheKey = "$path::$line"
    if ($contextCache.ContainsKey($cacheKey)) { return $contextCache[$cacheKey] }
    try {
        $allLines = Get-Content $path -ErrorAction Stop
        $start = [Math]::Max(0, $line - 3)
        $end = [Math]::Min($allLines.Count - 1, $line + 1)
        $ctx = @()
        for ($i = $start; $i -le $end; $i++) {
            $marker = if (($i + 1) -eq $line) { '>>' } else { '  ' }
            $ctx += ('{0} {1,4}: {2}' -f $marker, ($i + 1), $allLines[$i])
        }
        $result = $ctx -join "`n"
        $contextCache[$cacheKey] = $result
        return $result
    } catch {
        return $null
    }
}

foreach ($line in $rawOutput) {
    if ($line -match $errPattern) {
        $entry = [ordered]@{
            file = $matches['file'].Trim()
            line = if ($matches['line']) { [int]$matches['line'] } else { $null }
            col  = if ($matches['col'])  { [int]$matches['col']  } else { $null }
            code = $matches['code']
            msg  = $matches['msg'].Trim()
        }
        if ($matches['sev'] -match 'error') {
            if (-not $NoContext -and $entry.line) {
                $entry['context'] = Get-SourceContext -path $entry.file -line $entry.line
            }
            [void]$errors.Add($entry)
        } else {
            [void]$warnings.Add($entry)
        }
    }
}

# Truncate errors to first N
$truncatedErrors = $errors | Select-Object -First $First
$success = ($exitCode -eq 0)

# Build result object
$result = [ordered]@{
    success         = $success
    config          = $config
    file_filter     = if ($File) { $File } else { $null }
    elapsed_sec     = [Math]::Round($elapsed, 1)
    error_count     = $errors.Count
    warning_count   = $warnings.Count
    errors_shown    = $truncatedErrors.Count
    errors          = @($truncatedErrors)
}

# Output
if ($Json) {
    $result | ConvertTo-Json -Depth 6
} elseif ($Quiet) {
    if ($success) {
        Write-Output ("OK  $config in {0}s, {1} warnings" -f $result.elapsed_sec, $warnings.Count)
    } else {
        Write-Output ("FAIL $config in {0}s, {1} errors, {2} warnings" -f $result.elapsed_sec, $errors.Count, $warnings.Count)
    }
} else {
    $status = if ($success) { 'BUILD OK' } else { 'BUILD FAILED' }
    Write-Output ''
    Write-Output ("=== $status === $config / $platform / {0}s / {1} errors / {2} warnings" -f $result.elapsed_sec, $errors.Count, $warnings.Count)
    if ($File) { Write-Output "    file filter: $File" }
    Write-Output ''
    if ($errors.Count -gt 0) {
        Write-Output ("--- First {0} of {1} errors ---" -f $truncatedErrors.Count, $errors.Count)
        foreach ($e in $truncatedErrors) {
            $loc = if ($e.line) { "$($e.file)($($e.line))" } else { $e.file }
            Write-Output ""
            Write-Output ("  $loc : $($e.code): $($e.msg)")
            if ($e.context) {
                $e.context -split "`n" | ForEach-Object { Write-Output "    $_" }
            }
        }
        Write-Output ''
        if ($errors.Count -gt $First) {
            Write-Output "  ... $($errors.Count - $First) more errors hidden. Use -First $($errors.Count) to see all."
        }
    }
    if ($warnings.Count -gt 0 -and $errors.Count -eq 0) {
        Write-Output "    (warnings suppressed in default mode; rerun with -Json to see them)"
    }
}

if (-not $success) { exit 1 }
exit 0
