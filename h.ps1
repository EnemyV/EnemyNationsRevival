# h.ps1 — Enemy Nations test harness, ONE entry point with simple verbs.
# Goal: every drive action is the SAME shape ("& '...\h.ps1' <verb> [a] [b]") so a single
# allow-rule covers them all and they run without manual approval across long gaps.
#
# Coords below are derived from the SDL2 dialog SOURCE (SDL2Dialogs.cpp), not pixel-guessed,
# so they survive UI art changes. The game window is full-screen 2560x1440; dialog clicks use
# client coords of the named child window.
#
# Verbs:
#   h.ps1 windows                 list game windows
#   h.ps1 shot [role]             screen-grab a window (role: map/main/<title>) -> d:\tmp\h.png
#   h.ps1 zoom <in|out> <n>       wheel-zoom the Area Map n notches
#   h.ps1 rotate                  rotate the view (period key)
#   h.ps1 newgame [race]          menu -> Create -> OK -> pick race -> OK -> wait Area Map
#   h.ps1 waitmap [secs]          block until the Area Map (in-game) appears
#   h.ps1 perf                    print key terrain rebuild counters from perf.log

param([Parameter(Position=0)][string]$cmd='', [Parameter(Position=1)][string]$a1='', [Parameter(Position=2)][string]$a2='')
$ErrorActionPreference = 'Continue'
$SC  = 'd:\Enemy Nations\src'
$WD  = 'd:\Enemy Nations'

function _wins    { (tasklist /v /fi "imagename eq enations.exe" 2>$null | Out-String) }
function _has($t) { (_wins) -match [regex]::Escape($t) }
function _running { [bool](Get-Process enations -ErrorAction SilentlyContinue) }
function _click($win,$x,$y) { if($win){ & "$SC\click.ps1" -Window $win -X $x -Y $y 2>&1 | Out-Null } else { & "$SC\click.ps1" -X $x -Y $y 2>&1 | Out-Null } }

switch ($cmd) {

  'windows' { & "$SC\screenshot.ps1" -ListWindows 2>&1 | Select-Object -First 9 }

  'shot' {
    $role = if($a1){$a1}else{'map'}
    & "$SC\screenshot.ps1" -Window $role -Full -Screen -Out d:\tmp\h.png 2>&1 | Select-Object -Last 1
  }

  'zoom' {
    $n = if($a2){[int]$a2}else{1}
    if($a1 -eq 'in')  { & "$SC\zoom.ps1" -In  $n -Window map 2>&1 | Select-Object -Last 1 }
    else              { & "$SC\zoom.ps1" -Out $n -Window map 2>&1 | Select-Object -Last 1 }
  }

  'rotate' { & "$SC\keys.ps1" -Window map -Key OemPeriod 2>&1 | Select-Object -Last 1 }

  'waitmap' {
    $secs = if($a1){[int]$a1}else{180}
    $n=0; while(-not (_has 'Area Map') -and (_running) -and $n -lt ($secs/2)){ Start-Sleep 2; $n++ }
    if(_has 'Area Map'){ 'INGAME' } elseif(_running){ 'WAIT-TIMEOUT' } else { 'PROCESS-GONE' }
  }

  'newgame' {
    # Assumes the game is launched and at the main menu (h.ps1 doesn't launch dbgcatch).
    $n=0; while(-not (_has 'Game View') -and $n -lt 40){ Start-Sleep 2; $n++ }
    Start-Sleep 16                                   # let the menu art render
    _click $null 666 749                             # "Create Single Player Game" (menu, full-screen client)
    Start-Sleep 3
    if(_has 'Create Single'){ _click 'Create' 150 477; Start-Sleep 3 }   # Create dialog OK (SDL2Dialogs)
    if(_has 'Pick Your Race'){ _click 'Race' 40 92; Start-Sleep -Milliseconds 180; _click 'Race' 40 92 }  # double-click first race row = select+confirm (SDL2Dialogs.cpp:276)
    $n=0; while(-not (_has 'Area Map') -and (_running) -and $n -lt 240){ Start-Sleep 2; $n++ }   # worldgen can take >4min for a Large map
    if(_has 'Area Map'){ 'INGAME' } elseif(_running){ 'NEWGAME-TIMEOUT (worldgen?)' } else { 'PROCESS-GONE' }
  }

  'perf' {
    $p = "$WD\perf.log"
    if(-not (Test-Path $p)){ 'no perf.log'; break }
    $lines = Get-Content $p -Tail 60 | Where-Object { $_ -match 'rb\.hexes=[1-9]' -or $_ -match 'rebuild\.zoomreplay=1' }
    if(-not $lines){ 'no recent rebuild in perf.log'; break }
    $lines | Select-Object -Last 4 | ForEach-Object {
      $f=@{}; foreach($kv in ($_ -split '\|')){ if($kv -match '\s*([\w.]+)=\s*([-\d.]+)'){ $f[$matches[1]]=$matches[2] } }
      "fps={0} hexes={1} feather={2} tile={3} water={4} asm={5} t.rebuild={6}us replay={7}" -f `
        $f['fps'],$f['rb.hexes'],$f['rb.feather'],$f['rb.tile'],$f['rb.water'],$f['rb.asm'],$f['t.rebuild'],$f['rebuild.zoomreplay']
    }
  }

  default { "verbs: windows | shot [role] | zoom <in|out> N | rotate | newgame [race] | waitmap [secs] | perf" }
}
