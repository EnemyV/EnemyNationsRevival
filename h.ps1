# h.ps1 — Enemy Nations test harness, ONE entry point with simple verbs.
# Goal: every drive action is the SAME shape ("& '...\h.ps1' <verb> [a] [b]") so a single
# allow-rule covers them all and they run without manual approval across long gaps.
#
# Coords below are derived from the SDL2 dialog SOURCE (SDL2Dialogs.cpp), not pixel-guessed,
# so they survive UI art changes. The game window is full-screen 2560x1440; dialog clicks use
# client coords of the named child window.
#
# Verbs:
#   h.ps1 launch [0]              kill+relaunch x64 Debug under dbgcatch w/ profiling (0 = EN_RETAIN off)
#   h.ps1 load [Save7-Player]     menu -> Load -> pick save -> pick player -> wait Area Map
#   h.ps1 windows                 list game windows
#   h.ps1 shot [role]             screen-grab a window (role: map/main/<title>) -> d:\tmp\h.png
#   h.ps1 zoom <in|out> <n>       wheel-zoom the Area Map n notches
#   h.ps1 pan <left|right|up|down> [n]   scroll the Area Map a quarter-screen per press
#   h.ps1 click <x> <y> [role]    mouse-click client px on a window (default map)
#   h.ps1 rotate [cw|ccw]         rotate the view ('.' / ',')
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

  'rotate' {
    # rotate the view: '.' (OemPeriod) = one way, ',' (OemComma) = the other.
    $k = if($a1 -eq 'ccw' -or $a1 -eq ','){ 'OemComma' } else { 'OemPeriod' }
    & "$SC\keys.ps1" -Window map -Key $k 2>&1 | Select-Object -Last 1
  }

  'click' {
    # mouse-click client px on a window (default map). Mouse events are window-targeted (no focus
    # needed), so this is the reliable driver:  h.ps1 click <x> <y> [role]
    $cx = [int]$a1; $cy = [int]$a2
    $win = if($args.Count -ge 1){ $args[0] } else { 'map' }
    & "$SC\click.ps1" -Window $win -X $cx -Y $cy 2>&1 | Select-Object -Last 1
  }

  'pan' {
    # scroll the Area Map a quarter-screen per press via arrow keys (CWndArea::CurLeft/etc.).
    #   h.ps1 pan <left|right|up|down> [n]
    $dir = switch($a1){ 'right'{'Right'} 'up'{'Up'} 'down'{'Down'} default{'Left'} }
    $n = if($a2){[int]$a2}else{1}
    for($i=0;$i -lt $n;$i++){ & "$SC\keys.ps1" -Window map -Key $dir 2>&1 | Out-Null; Start-Sleep -Milliseconds 200 }
    "panned $dir x$n"
  }

  'launch' {
    # Kill any running game, then launch x64 Debug under dbgcatch with the profiling env,
    # DETACHED so this returns immediately. Constant shape -> a single "always allow" covers it.
    #   h.ps1 launch        EN_RETAIN=1 (default, the build the user runs)
    #   h.ps1 launch 0      EN_RETAIN=0 (caching off)
    Get-Process enations -ErrorAction SilentlyContinue | Stop-Process -Force
    Start-Sleep 1
    $env:EN_PERF = '1'
    $env:EN_RETAIN = if ($a1 -eq '0') { '0' } else { '1' }
    $env:EN_RETAIN_ZOUT = '0'
    $exe = 'd:\Enemy Nations\src\cmakeBuild-x64\enations_latest\src\Debug\enations.exe'
    $pargs = "-NoProfile -File `"$SC\dbgcatch.ps1`" -Exe `"$exe`" -WorkDir `"$WD`" -Seconds 28800"
    Start-Process powershell -WindowStyle Hidden -RedirectStandardOutput 'd:\tmp\dbgcatch.log' -ArgumentList $pargs
    Start-Sleep 3
    if (_running) { "LAUNCHED (EN_RETAIN=$($env:EN_RETAIN)) -> log d:\tmp\dbgcatch.log" } else { 'FAILED' }
  }

  'load' {
    # Assumes game launched + at main menu. Loads Save7-Player (the fixed perf map).
    # Coords from SDL2Dialogs.cpp: PickPlayer OK = (235,470) client, disabled until a row is clicked.
    $save = if($a1){$a1}else{'Save7-Player'}
    $n=0; while(-not (_has 'Game View') -and $n -lt 40){ Start-Sleep 2; $n++ }
    Start-Sleep 6                                      # menu art must FULLY render or the click misses
    _click 'main' 1800 95                              # "Load Single Player Game"
    $n=0; while(-not (_has 'Load Game') -and $n -lt 15){ Start-Sleep 1; $n++ }
    if(_has 'Load Game'){ Start-Sleep 3; _click 'Load Game' 110 132; Start-Sleep -Milliseconds 700; _click 'Load Game' 186 378 }  # settle (list must populate), select row3=Save7-Player, Open
    $n=0; while(-not (_has 'Pick Your Player') -and $n -lt 30){ Start-Sleep 1; $n++ }
    if(_has 'Pick Your Player'){ Start-Sleep 2; _click 'Pick Your Player' 110 222; Start-Sleep -Milliseconds 700; _click 'Pick Your Player' 235 470 }  # settle, select 'vter' row -> enable+click OK
    $n=0; while(-not (_has 'Area Map') -and (_running) -and $n -lt 120){ Start-Sleep 2; $n++ }
    if(_has 'Area Map'){ 'INGAME' } elseif(_running){ 'LOAD-TIMEOUT' } else { 'PROCESS-GONE' }
  }

  'waitmap' {
    $secs = if($a1){[int]$a1}else{180}
    $n=0; while(-not (_has 'Area Map') -and (_running) -and $n -lt ($secs/2)){ Start-Sleep 2; $n++ }
    if(_has 'Area Map'){ 'INGAME' } elseif(_running){ 'WAIT-TIMEOUT' } else { 'PROCESS-GONE' }
  }

  'newgame' {
    # Assumes the game is launched and at the main menu (h.ps1 doesn't launch dbgcatch).
    $n=0; while(-not (_has 'Game View') -and $n -lt 40){ Start-Sleep 2; $n++ }
    Start-Sleep 22                                   # let the menu art FULLY render (clicking too soon misses)
    _click $null 666 749                             # "Create Single Player Game" (menu, full-screen client)
    $n=0; while(-not (_has 'Create Single') -and $n -lt 10){ Start-Sleep 1; $n++ }   # wait for the dialog, don't click into the void
    if(_has 'Create Single'){ Start-Sleep 2; _click 'Create' 185 470; Start-Sleep 3 }   # settle THEN OK (size remembered=Large). SDL2UI.cpp AddOKCancelButtons -> (185,470)
    $n=0; while(-not (_has 'Pick Your Race') -and $n -lt 10){ Start-Sleep 1; $n++ }
    if(_has 'Pick Your Race'){ Start-Sleep 2; _click 'Race' 40 92; Start-Sleep -Milliseconds 700; _click 'Race' 235 430 }  # settle, select Human, settle, OK (SDL2Dialogs.cpp:305 -> 235,430)
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
