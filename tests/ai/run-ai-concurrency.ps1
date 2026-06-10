# run-ai-concurrency.ps1 -- compile & run the AI concurrency MODEL tests.
#
# Separate from run-ai-tests.ps1 because these spawn real threads and are slower
# / non-deterministic by nature (stress-based). Still fully self-contained: links
# no game code, touches no game build.
#
# Exit codes: 0 all pass, 1 a test failed, 2 toolchain / compile error.

$ErrorActionPreference = 'Stop'
$here = Split-Path -Parent $MyInvocation.MyCommand.Path

$roots = @(
    'C:\Program Files\Microsoft Visual Studio\2022\Community',
    'C:\Program Files\Microsoft Visual Studio\2022\Enterprise',
    'C:\Program Files\Microsoft Visual Studio\2022\Professional'
)
$vs = $roots | Where-Object { Test-Path (Join-Path $_ 'VC\Auxiliary\Build\vcvars64.bat') } | Select-Object -First 1
if (-not $vs) { Write-Error 'VS 2022 vcvars64.bat not found (edit roots in run-ai-concurrency.ps1).'; exit 2 }

$vcvars = Join-Path $vs 'VC\Auxiliary\Build\vcvars64.bat'
$src    = Join-Path $here 'test_ai_concurrency.cpp'
$outDir = 'd:\tmp\aitests'
New-Item -ItemType Directory -Force -Path $outDir | Out-Null
$obj = Join-Path $outDir 'ai_conc.obj'
$exe = Join-Path $outDir 'ai_conc.exe'

# /MD = multithreaded DLL runtime (std::thread).
$cl   = "cl /nologo /EHsc /std:c++17 /W4 /MD `"$src`" /Fo`"$obj`" /Fe`"$exe`""
$line = "`"$vcvars`" >nul 2>&1 && $cl && `"$exe`""

cmd /c $line
exit $LASTEXITCODE
