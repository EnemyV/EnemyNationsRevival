param(
    [Parameter(Mandatory=$true)][string]$Executable,
    [Parameter(Mandatory=$true)][string]$RunDir,
    [string]$Manifest,
    [string]$Fixture,
    [string]$FixtureSha256,
    [Parameter(Mandatory=$true)][string]$Output,
    [double]$Seconds = 60,
    [double]$Warmup = 0,
    [string]$Label = 'unlabelled',
    [string[]]$EnvOverride = @(),
    [Nullable[int]]$CameraX,
    [Nullable[int]]$CameraY,
    [Nullable[int]]$Speed,
    [switch]$DummySdl
)
$arguments = @("$PSScriptRoot/run_fixture.py", '--executable', $Executable,
    '--run-dir', $RunDir, '--output', $Output, '--label', $Label,
    '--seconds', $Seconds.ToString([Globalization.CultureInfo]::InvariantCulture),
    '--warmup', $Warmup.ToString([Globalization.CultureInfo]::InvariantCulture))
if ($Manifest) { $arguments += @('--manifest', $Manifest) }
if ($Fixture) { $arguments += @('--fixture', $Fixture) }
if ($FixtureSha256) { $arguments += @('--fixture-sha256', $FixtureSha256) }
if ($null -ne $CameraX) { $arguments += @('--camera-x', $CameraX.ToString()) }
if ($null -ne $CameraY) { $arguments += @('--camera-y', $CameraY.ToString()) }
if ($null -ne $Speed) { $arguments += @('--speed', $Speed.ToString()) }
foreach ($value in $EnvOverride) { $arguments += @('--env', $value) }
if ($DummySdl) { $arguments += '--dummy-sdl' }
& python @arguments
exit $LASTEXITCODE
