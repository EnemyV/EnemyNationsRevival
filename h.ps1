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
#   h.ps1 load [Save7-Player]     menu -> Load -> pick save -> pick player -> wait Area Map (retries dropped clicks)
#   h.ps1 measure                 zoom IN then OUT, print recent full-z3 rebuild times (ms) + breakdown
#   h.ps1 zoom <in|out> <n>       wheel-zoom the Area Map n notches (the only reliably-driveable input)
#   h.ps1 shot [role]             wake screen, screen-grab a window (role: map/main/<title>) -> d:\tmp\h.png
#   h.ps1 wake                    jiggle the mouse to defeat the screensaver (captures go black under it)
#   h.ps1 status                  read-only: game alive? + memory + dbgcatch.log tail (crash markers)
#   h.ps1 fresh                   read-only: is the exe newer than SDL2Terrain.cpp (did it compile)?
#   h.ps1 windows                 list game windows
#   h.ps1 perf                    print key terrain rebuild counters from perf.log
#   h.ps1 pan <left|right|up|down> [n]   scroll the Area Map a quarter-screen per press (arrow keys)
#   h.ps1 click <x> <y> [role]    mouse-click client px on a window (default map)
#   h.ps1 rotate [cw|ccw] [n]     rotate the view ('.'=CW / ','=CCW in-game hotkeys, n times)
#   h.ps1 newgame [race]          menu -> Create -> OK -> pick race -> OK -> wait Area Map
#   h.ps1 waitmap [secs]          block until the Area Map (in-game) appears

param([Parameter(Position=0)][string]$cmd='', [Parameter(Position=1)][string]$a1='', [Parameter(Position=2)][string]$a2='')
$ErrorActionPreference = 'Continue'
$SC  = 'd:\Enemy Nations\src'
$WD  = 'd:\Enemy Nations'

# IMPORTANT: the game's dialogs/panels (Area Map, Radar, Load Game, Pick Your Player) are each
# their OWN child SDL window. tasklist /v only reports a process's MAIN window title, so it can't
# see them -- use screenshot.ps1 -ListWindows, which enumerates every game window by title.
function _wins    { (& "$SC\screenshot.ps1" -ListWindows 2>&1 | Out-String) }
function _has($t) { (_wins) -match [regex]::Escape($t) }
function _running { [bool](Get-Process enations -ErrorAction SilentlyContinue) }
# Tolerant click: click.ps1 throws if the target window vanished (common mid-transition, e.g. a
# dialog closing while a retry loop is still aiming at it) -- swallow that so the flow continues.
function _click($win,$x,$y) { try { if($win){ & "$SC\click.ps1" -Window $win -X $x -Y $y 2>&1 | Out-Null } else { & "$SC\click.ps1" -X $x -Y $y 2>&1 | Out-Null } } catch {} }

# Jiggle the mouse a few px (relative) to keep the screensaver/monitor-sleep off — otherwise
# -Screen captures come back all-black. -4 is passed as its uint32 two's-complement (PS has no
# signed overload). Harmless 0-net-displacement wiggle.
function _wake {
    if (-not ('WH.MM' -as [type])) {
        Add-Type -Name MM -Namespace WH -MemberDefinition '[DllImport("user32.dll")]public static extern void mouse_event(uint f,uint x,uint y,uint d,int e);'
    }
    $neg = [uint32]4294967292   # -4 as uint32
    for ($i=0; $i -lt 5; $i++) {
        [WH.MM]::mouse_event(1, 4,    0, 0, 0); Start-Sleep -Milliseconds 70
        [WH.MM]::mouse_event(1, $neg, 0, 0, 0); Start-Sleep -Milliseconds 70
    }
}

# Click (win,x,y) repeatedly until $doneCond (scriptblock -> bool) is true, up to $tries.
# Defeats the SDL quirk where the first click(s) on a freshly-shown dialog are eaten by
# window activation. Returns $true if the condition was met.
function _clickUntil($win,$x,$y,$doneCond,$tries,$gapMs=700) {
    for ($i=0; $i -lt $tries; $i++) {
        if (& $doneCond) { return $true }
        _click $win $x $y
        Start-Sleep -Milliseconds $gapMs
    }
    return (& $doneCond)
}

switch ($cmd) {

  'windows' { & "$SC\screenshot.ps1" -ListWindows 2>&1 | Select-Object -First 9 }

  'wake' { _wake; 'awake' }

  'status' {
    $g = Get-Process enations -ErrorAction SilentlyContinue
    if ($g) { "ALIVE pid=$($g.Id) WS=$([math]::Round($g.WorkingSet64/1MB))MB Priv=$([math]::Round($g.PrivateMemorySize64/1MB))MB" }
    else    { "GAME NOT RUNNING" }
    "--- dbgcatch.log (tail) ---"
    Get-Content 'd:\tmp\dbgcatch.log' -Tail 8 -ErrorAction SilentlyContinue
  }

  'fresh' {
    $src = Get-Item 'd:\Enemy Nations\src\enations_latest\src\SDL2Terrain.cpp' -ErrorAction SilentlyContinue
    $exe = Get-Item 'd:\Enemy Nations\src\cmakeBuild-x64\enations_latest\src\Debug\enations.exe' -ErrorAction SilentlyContinue
    if ($src -and $exe) { "exe newer than SDL2Terrain.cpp: $($exe.LastWriteTime -gt $src.LastWriteTime)  (exe $($exe.LastWriteTime), src $($src.LastWriteTime))" }
    else { 'src or exe missing' }
  }

  'shot' {
    $role = if($a1){$a1}else{'map'}
    _wake
    & "$SC\screenshot.ps1" -Window $role -Full -Screen -Out d:\tmp\h.png 2>&1 | Select-Object -Last 1
  }

  'zoom' {
    $n = if($a2){[int]$a2}else{1}
    if($a1 -eq 'in')  { & "$SC\zoom.ps1" -In  $n -Window map 2>&1 | Select-Object -Last 1 }
    else              { & "$SC\zoom.ps1" -Out $n -Window map 2>&1 | Select-Object -Last 1 }
  }

  'measure' {
    # Force a fresh full warm rebuild: zoom IN to z0 then OUT to z3, then report the recent
    # full-z3 rebuilds (zoomreplay=0). Reads the tail so it's fast on the multi-MB perf.log.
    if (-not (_has 'Area Map')) { 'NOT IN-GAME (run: h.ps1 load)'; break }
    # Generous notch spacing: under heavy AI load a rebuild can outlast the gap and the next wheel
    # notch is dropped, so the view never reaches max zoom-out. Over-zoom (6 notches) + long settle.
    & "$SC\zoom.ps1" -In  6 -Window map -DelayMs 1500 2>&1 | Out-Null; Start-Sleep 3
    & "$SC\zoom.ps1" -Out 6 -Window map -DelayMs 2500 2>&1 | Out-Null; Start-Sleep 4
    $p = "$WD\perf.log"
    if (-not (Test-Path $p)) { 'no perf.log'; break }
    $lines = Get-Content $p -Tail 400 | Where-Object { $_ -match 'rebuild\.zoomreplay=0' -and $_ -match 'rb\.hexes=[1-9]' } | Select-Object -Last 6
    if (-not $lines) { 'no full rebuild captured (try again, or zoom was a replay)'; break }
    'recent full z3 rebuilds (most recent last):'
    $lines | ForEach-Object {
      $f=@{}; foreach($m in [regex]::Matches($_, '([\w.]+)=\s*([-\d.]+)')){ $f[$m.Groups[1].Value]=$m.Groups[2].Value }
      "  t.rebuild={0,5}ms  hexes={1}  edit={2}  | asm={3} fog={4} water={5} tile={6} feather={7} (ms)" -f `
        [int]([double]$f['t.rebuild']/1000), $f['rb.hexes'], $f['rebuild.edit'], `
        [int]([double]$f['rb.asm']/1000),  [int]([double]$f['rb.fog']/1000), [int]([double]$f['rb.water']/1000), `
        [int]([double]$f['rb.tile']/1000), [int]([double]$f['rb.feather']/1000)
    }
  }

  'rotate' {
    # View rotate: '.' = CW, ',' = CCW (keys.ps1 injects the scancode so these OEM hotkeys
    # resolve in-game).  h.ps1 rotate [cw|ccw] [n]
    $k = if($a1 -eq 'ccw'){ ',' } else { '.' }
    $n = if($a2){[int]$a2}else{1}
    for($i=0;$i -lt $n;$i++){ & "$SC\keys.ps1" -Window map -Key $k 2>&1 | Out-Null; Start-Sleep -Milliseconds 300 }
    "rotated $k x$n"
  }

  'click' {
    $cx = [int]$a1; $cy = [int]$a2
    $win = if($args.Count -ge 1){ $args[0] } else { 'map' }
    & "$SC\click.ps1" -Window $win -X $cx -Y $cy 2>&1 | Select-Object -Last 1
  }

  'pan' {
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
    # Menu -> Load Save7-Player -> pick player -> in-game. Every click step RETRIES until the UI
    # actually advances, because the first click(s) on a freshly-shown SDL dialog are eaten by
    # window activation (the long-standing flakiness). Coords from SDL2Dialogs.cpp.
    $n=0; while(-not (_has 'Game View') -and $n -lt 40){ Start-Sleep 2; $n++ }
    Start-Sleep 5
    # 1) open the Load dialog (retry the menu button)
    $ok = _clickUntil 'main' 1800 95 { _has 'Load Game' } 6 1500
    if (-not $ok) { 'FAIL: Load dialog never opened'; break }
    Start-Sleep 2
    # 2a) select Save7-Player (row 3 @ 110,132) + Open (186,378) until the Load dialog CLOSES
    #     (= Open registered). Don't gate on Pick Player here -- loading the save then takes time.
    for ($i=0; $i -lt 8 -and (_has 'Load Game'); $i++) {
        _click 'Load Game' 110 132; Start-Sleep -Milliseconds 500
        _click 'Load Game' 186 378; Start-Sleep -Milliseconds 1000
    }
    # 2b) now wait (generously) for the save to load -> Pick Player dialog
    $n=0; while(-not (_has 'Pick Your Player') -and (_running) -and $n -lt 40){ Start-Sleep 2; $n++ }
    if (-not (_has 'Pick Your Player')) { 'FAIL: Pick Player never appeared (save load?)'; break }
    # 3) select the player row (110,222 enables OK) + OK (235,470) until the dialog closes
    for ($i=0; $i -lt 8 -and (_has 'Pick Your Player'); $i++) {
        _click 'Pick Your Player' 110 222; Start-Sleep -Milliseconds 500
        _click 'Pick Your Player' 235 470; Start-Sleep -Milliseconds 1000
    }
    # 4) wait for the in-game Area Map
    $n=0; while(-not (_has 'Area Map') -and (_running) -and $n -lt 120){ Start-Sleep 2; $n++ }
    if(_has 'Area Map'){ 'INGAME' } elseif(_running){ 'LOAD-TIMEOUT' } else { 'PROCESS-GONE' }
  }

  'waitmap' {
    $secs = if($a1){[int]$a1}else{180}
    $n=0; while(-not (_has 'Area Map') -and (_running) -and $n -lt ($secs/2)){ Start-Sleep 2; $n++ }
    if(_has 'Area Map'){ 'INGAME' } elseif(_running){ 'WAIT-TIMEOUT' } else { 'PROCESS-GONE' }
  }

  'newgame' {
    $n=0; while(-not (_has 'Game View') -and $n -lt 40){ Start-Sleep 2; $n++ }
    Start-Sleep 22
    _click $null 666 749
    $ok = _clickUntil 'Create' 185 470 { _has 'Pick Your Race' } 6 1500   # OK (size remembered=Large)
    if (_has 'Pick Your Race'){ Start-Sleep 2; _click 'Race' 40 92; Start-Sleep -Milliseconds 700; _click 'Race' 235 430 }
    $n=0; while(-not (_has 'Area Map') -and (_running) -and $n -lt 240){ Start-Sleep 2; $n++ }
    if(_has 'Area Map'){ 'INGAME' } elseif(_running){ 'NEWGAME-TIMEOUT (worldgen?)' } else { 'PROCESS-GONE' }
  }

  'perf' {
    $p = "$WD\perf.log"
    if(-not (Test-Path $p)){ 'no perf.log'; break }
    $lines = Get-Content $p -Tail 60 | Where-Object { $_ -match 'rb\.hexes=[1-9]' -or $_ -match 'rebuild\.zoomreplay=1' }
    if(-not $lines){ 'no recent rebuild in perf.log'; break }
    $lines | Select-Object -Last 4 | ForEach-Object {
      $f=@{}; foreach($m in [regex]::Matches($_, '([\w.]+)=\s*([-\d.]+)')){ $f[$m.Groups[1].Value]=$m.Groups[2].Value }
      "t.rebuild={0}ms hexes={1} asm={2} fog={3} water={4} tile={5} feather={6} replay={7}" -f `
        [int]([double]$f['t.rebuild']/1000),$f['rb.hexes'],[int]([double]$f['rb.asm']/1000),[int]([double]$f['rb.fog']/1000),`
        [int]([double]$f['rb.water']/1000),[int]([double]$f['rb.tile']/1000),[int]([double]$f['rb.feather']/1000),$f['rebuild.zoomreplay']
    }
  }

  default { "verbs: launch | load | measure | zoom <in|out> N | shot [role] | wake | status | fresh | windows | perf | pan | click | rotate | newgame | waitmap" }
}
