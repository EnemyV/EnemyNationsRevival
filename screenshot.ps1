# screenshot.ps1 — capture the running Enemy Nations window to an image.
#
# Low-res by DEFAULT (50% scale) to keep vision-token cost down; pass -Full for
# native resolution. Targets the right SDL window by role so coords match clicks.
#
# Usage:
#   ./screenshot.ps1                       # auto window, 50% scale -> screenshot.png
#   ./screenshot.ps1 -Window map           # the in-game Area Map child window
#   ./screenshot.ps1 -Window main          # main window (toolbar/chrome only in-game)
#   ./screenshot.ps1 -Scale 25             # quarter res (cheapest readable)
#   ./screenshot.ps1 -Full                 # native resolution (100%)
#   ./screenshot.ps1 -Composite            # main + all child windows stitched (true view)
#   ./screenshot.ps1 -Region 100,50,400,300 # crop client rect (x,y,w,h) before scaling
#   ./screenshot.ps1 -Gray -Jpeg 60        # grayscale JPEG q60 (smallest)
#   ./screenshot.ps1 -Screen               # screen-grab fallback (window must be visible)
#   ./screenshot.ps1 -ListWindows          # list the game's SDL windows + roles
#   ./screenshot.ps1 -Hwnd 12345           # capture a specific HWND
#
# Window roles for -Window: main/menu, map/area, radar, vehicles, buildings,
# research, pick/player — or any substring of a window title.
#
# Exit codes: 0 ok, 1 game not running / window not found, 2 capture failed.

[CmdletBinding()]
param(
    [string]$Out = 'screenshot.png',
    [string]$Window,
    [string]$ProcessName,
    [int]$Hwnd,
    [int]$Scale = 50,              # percent of native; default 50% to save tokens
    [switch]$Full,                 # == -Scale 100
    [switch]$Composite,            # stitch main + child windows into one image
    [string]$Region,               # "x,y,w,h" client px, applied before scaling
    [switch]$Gray,
    [int]$Jpeg = 0,                # 1..100 => JPEG at that quality; 0 => PNG
    [switch]$Screen,
    [switch]$ListWindows
)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'harness-common.ps1')
Add-Type -AssemblyName System.Drawing

if ($Full) { $Scale = 100 }
if ($Scale -lt 1)   { $Scale = 1 }
if ($Scale -gt 100) { $Scale = 100 }

# ---- -ListWindows --------------------------------------------------------
if ($ListWindows) {
    $proc = Find-GameProcess -ExplicitName $ProcessName
    if (-not $proc) { Write-Error "Game not running."; exit 1 }
    $wins = Get-GameSdlWindows -Process $proc
    if ($wins.Count -eq 0) { Write-Output "No visible SDL_app windows."; exit 1 }
    Write-Output ("Process: {0} (PID {1})" -f $proc.ProcessName, $proc.Id)
    $wins | ForEach-Object {
        $role = if ($_.Parent -eq [IntPtr]::Zero) { 'main' } else { '' }
        [pscustomobject]@{ Hwnd = $_.Hwnd.ToInt64(); Client = ("{0}x{1}" -f $_.CW, $_.CH); Title = $_.Title; Kind = $role }
    } | Format-Table -AutoSize | Out-String | Write-Output
    exit 0
}

# ---- Capture -------------------------------------------------------------
$srcLabel = ''
try {
    if ($Composite) {
        $proc = Find-GameProcess -ExplicitName $ProcessName
        if (-not $proc) { Write-Error "Game not running."; exit 1 }
        $img = Get-CompositeBitmap -Process $proc
        $srcLabel = 'composite'
    } else {
        $info = Resolve-GameTarget -Hwnd $Hwnd -Window $Window -ProcessName $ProcessName
        $img = Get-WindowBitmap -Info $info -Screen:$Screen
        $srcLabel = "'$($info.Title)'"
    }
} catch {
    Write-Error $_
    exit 2
}

$nativeW = $img.Width; $nativeH = $img.Height

# ---- Optional crop (client px, pre-scale) --------------------------------
if ($Region) {
    $p = $Region -split '[, ]+' | Where-Object { $_ -ne '' } | ForEach-Object { [int]$_ }
    if ($p.Count -ne 4) { Write-Error "Region must be 'x,y,w,h'"; exit 2 }
    $rx = [Math]::Max(0, $p[0]); $ry = [Math]::Max(0, $p[1])
    $rw = [Math]::Min($p[2], $img.Width  - $rx)
    $rh = [Math]::Min($p[3], $img.Height - $ry)
    $crop = New-Object System.Drawing.Bitmap $rw, $rh
    $g = [System.Drawing.Graphics]::FromImage($crop)
    $g.DrawImage($img, (New-Object System.Drawing.Rectangle 0,0,$rw,$rh), (New-Object System.Drawing.Rectangle $rx,$ry,$rw,$rh), [System.Drawing.GraphicsUnit]::Pixel)
    $g.Dispose(); $img.Dispose(); $img = $crop
}

# ---- Scale ---------------------------------------------------------------
if ($Scale -lt 100) {
    $nw = [Math]::Max(1, [int]($img.Width  * $Scale / 100))
    $nh = [Math]::Max(1, [int]($img.Height * $Scale / 100))
    $scaled = New-Object System.Drawing.Bitmap $nw, $nh
    $g = [System.Drawing.Graphics]::FromImage($scaled)
    $g.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
    $g.DrawImage($img, 0, 0, $nw, $nh)
    $g.Dispose(); $img.Dispose(); $img = $scaled
}

# ---- Grayscale -----------------------------------------------------------
if ($Gray) {
    $cm = New-Object System.Drawing.Imaging.ColorMatrix
    $cm.Matrix00 = 0.299; $cm.Matrix01 = 0.299; $cm.Matrix02 = 0.299
    $cm.Matrix10 = 0.587; $cm.Matrix11 = 0.587; $cm.Matrix12 = 0.587
    $cm.Matrix20 = 0.114; $cm.Matrix21 = 0.114; $cm.Matrix22 = 0.114
    $ia = New-Object System.Drawing.Imaging.ImageAttributes
    $ia.SetColorMatrix($cm)
    $gimg = New-Object System.Drawing.Bitmap $img.Width, $img.Height
    $g = [System.Drawing.Graphics]::FromImage($gimg)
    $g.DrawImage($img, (New-Object System.Drawing.Rectangle 0,0,$img.Width,$img.Height), 0,0,$img.Width,$img.Height, [System.Drawing.GraphicsUnit]::Pixel, $ia)
    $g.Dispose(); $img.Dispose(); $img = $gimg
}

# ---- Save ----------------------------------------------------------------
if (-not [System.IO.Path]::IsPathRooted($Out)) { $Out = Join-Path (Get-Location).Path $Out }
$outDir = Split-Path -Parent $Out
if ($outDir -and -not (Test-Path $outDir)) { New-Item -ItemType Directory -Force -Path $outDir | Out-Null }

if ($Jpeg -gt 0) {
    if ([System.IO.Path]::GetExtension($Out) -notmatch '\.jpe?g$') { $Out = [System.IO.Path]::ChangeExtension($Out, '.jpg') }
    $enc = [System.Drawing.Imaging.ImageCodecInfo]::GetImageEncoders() | Where-Object { $_.MimeType -eq 'image/jpeg' }
    $ep = New-Object System.Drawing.Imaging.EncoderParameters 1
    $ep.Param[0] = New-Object System.Drawing.Imaging.EncoderParameter ([System.Drawing.Imaging.Encoder]::Quality, [long]([Math]::Min(100,[Math]::Max(1,$Jpeg))))
    $img.Save($Out, $enc, $ep)
} else {
    $img.Save($Out, [System.Drawing.Imaging.ImageFormat]::Png)
}

$fw = $img.Width; $fh = $img.Height
$img.Dispose()
$kb = [Math]::Round((Get-Item $Out).Length / 1KB, 1)
# Rough Claude vision-token estimate: ~(w*h)/750, long edge capped at 1568.
$ew = $fw; $eh = $fh
if ([Math]::Max($ew,$eh) -gt 1568) { $r = 1568 / [Math]::Max($ew,$eh); $ew = [int]($ew*$r); $eh = [int]($eh*$r) }
$tok = [int][Math]::Ceiling(($ew * $eh) / 750)
Write-Output ("shot {0}x{1} ({2}% of {3}x{4}) {5} KB  ~{6} tok  src={7} -> {8}" -f $fw, $fh, $Scale, $nativeW, $nativeH, $kb, $tok, $srcLabel, $Out)
exit 0
