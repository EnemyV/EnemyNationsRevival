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
#   h.ps1 log [name] [n]          tail a known log (perf|dbg|terrain|list). list = paths+ages
#   h.ps1 grep <pattern> [name] [n]   regex-search a known log's tail (default perf, 400 lines)
#   h.ps1 pvshot [n]              hold the zoom-gesture PREVIEW open (out/in notch stream in a
#                                 background job; each notch resets the 120ms settle) and grab n
#                                 mid-preview screenshots -> d:\tmp\pv<i>.png. Verifies via pv.frame.
#   h.ps1 share [file] [ttl]      upload a capture to litterbox.catbox.moe (temp host) -> URL
#                                 default d:\tmp\h.png, ttl 12h (1h|12h|24h|72h)
#   h.ps1 save [name]             save the running game (Ctrl+O -> Save Game -> type -> Save).
#                                 NOTE: silently impossible during the rocket phase (by design).
#   h.ps1 land [x] [y]            land the starting rocket via the REAL mouse (the build cursor
#                                 only follows real WM_MOUSEMOVE; PostMessage clicks can't move it)
#   h.ps1 mouse <x> <y> [role]    REAL-mouse click at client coords (SetCursorPos+SendInput) —
#                                 for the few paths PostMessage can't drive (build cursor etc.)
#   h.ps1 saves                   list save files (exe dir + run dir), newest first
#   h.ps1 profile [n]             n dbgstack samples (~3s apart) -> top-frame histogram
#   h.ps1 pan <left|right|up|down> [n]   scroll the Area Map a quarter-screen per press (arrow keys)
#   h.ps1 click <x> <y> [role]    mouse-click client px on a window (default map)
#   h.ps1 rotate [cw|ccw] [n]     rotate the view ('.'=CW / ','=CCW in-game hotkeys, n times)
#   h.ps1 newgame [race]          menu -> Create -> OK -> pick race -> OK -> wait Area Map
#   h.ps1 waitmap [secs]          block until the Area Map (in-game) appears

param([Parameter(Position=0)][string]$cmd='', [Parameter(Position=1)][string]$a1='', [Parameter(Position=2)][string]$a2='', [Parameter(Position=3)][string]$a3='')
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

  'fps' {
    # Average fps + worst frame over the last N perf lines (1 line/sec).  h.ps1 fps [secs]
    $secs = if($a1){[int]$a1}else{10}
    $p = "$WD\perf.log"
    if(-not (Test-Path $p)){ 'no perf.log'; break }
    $rows = Get-Content $p -Tail $secs | ForEach-Object {
      $f=@{}; foreach($m in [regex]::Matches($_, '([\w.]+)=\s*([-\d.]+)')){ $f[$m.Groups[1].Value]=$m.Groups[2].Value }; $f }
    $fpsVals = $rows | ForEach-Object { [double]$_['fps'] } | Where-Object { $_ -gt 0 }
    if(-not $fpsVals){ 'no fps samples'; break }
    $avg = ($fpsVals | Measure-Object -Average).Average
    $min = ($fpsVals | Measure-Object -Minimum).Minimum
    $worst = ($rows | ForEach-Object { [double]$_['max'] } | Measure-Object -Maximum).Maximum
    "fps over last $($fpsVals.Count)s: avg={0:N1} min={1:N1}  worst-frame={2:N0}ms" -f $avg, $min, $worst
  }

  'gesture' {
    # Fire a RAPID multi-notch zoom gesture (like a user flicking the wheel), then report how
    # many full rebuilds it cost (coalescing => 1) and the preview/defer counters.
    #   h.ps1 gesture [out|in] [notches]
    if (-not (_has 'Area Map')) { 'NOT IN-GAME (run: h.ps1 load)'; break }
    $dirIn = ($a1 -eq 'in')
    $n = if($a2){[int]$a2}else{4}
    $p = "$WD\perf.log"
    # -Tail 1 can race a log write and return 2 lines -> $before becomes an array and every
    # later [int] comparison throws. Keep only the LAST t= value, coerced scalar.
    $before = 0
    if (Test-Path $p) {
      $bv = @(Get-Content $p -Tail 2 | ForEach-Object { if($_ -match '^t=(\d+)'){ [int]$matches[1] } }) | Select-Object -Last 1
      if ($bv) { $before = [int]$bv }
    }
    # 60ms notch spacing = a real wheel flick (the settle window is 120ms, so notches must
    # arrive faster than that to coalesce -- they do for any human flick).
    if($dirIn){ & "$SC\zoom.ps1" -In $n -Window map -DelayMs 60 2>&1 | Out-Null }
    else      { & "$SC\zoom.ps1" -Out $n -Window map -DelayMs 60 2>&1 | Out-Null }
    Start-Sleep 6   # let the settle-rebuild land and a perf line flush
    $rows = Get-Content $p -Tail 30 | ForEach-Object {
      $f=@{}; foreach($m in [regex]::Matches($_, '([\w.]+)=\s*([-\d.]+)')){ $f[$m.Groups[1].Value]=$m.Groups[2].Value }; $f } |
      Where-Object { [int]$_['t'] -gt $before }
    # rebuild.key = rebuilds caused by a view-key (zoom/dir) change — what the gesture costs.
    # rebuild.cnt also counts pan/edge-scroll incremental rebuilds (mouse near a screen edge
    # auto-scrolls the map), which used to inflate this to "5 rebuilds" for a 1-notch zoom.
    $rebuilds = ($rows | ForEach-Object { [int]$_['rebuild.key'] } | Measure-Object -Sum).Sum
    $panRebs  = ($rows | ForEach-Object { [int]$_['rebuild.pan'] } | Measure-Object -Sum).Sum
    $defers   = ($rows | ForEach-Object { [int]$_['pv.defer'] }    | Measure-Object -Sum).Sum
    $rbTimes  = $rows | Where-Object { [int]$_['t.rebuild'] -gt 0 } | ForEach-Object { [int]([double]$_['t.rebuild']/1000) }
    $worst    = ($rows | ForEach-Object { [double]$_['max'] } | Measure-Object -Maximum).Maximum
    "gesture: $n notches $(if($dirIn){'IN'}else{'OUT'}) | key-rebuilds=$rebuilds (want 1) | pan-rebuilds=$panRebs | defer-frames=$defers | rebuild-times(ms)=$($rbTimes -join ',') | worst-frame={0:N0}ms" -f $worst
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
    # Menu -> Load <save> -> pick player -> in-game. Every click step RETRIES until the UI
    # actually advances, because the first click(s) on a freshly-shown SDL dialog are eaten by
    # window activation (the long-standing flakiness). Coords from SDL2Dialogs.cpp.
    #   h.ps1 load                 -> Save7-Player (default)
    #   h.ps1 load Save13-Player   -> any save: TYPED into the Filename field (no row hunting)
    $save = if($a1){$a1}else{'Save7-Player'}
    if ($save -notmatch '\.en$') { $save += '.en' }
    $n=0; while(-not (_has 'Game View') -and $n -lt 40){ Start-Sleep 2; $n++ }
    Start-Sleep 5
    # 1) open the Load dialog (retry the menu button)
    $ok = _clickUntil 'main' 1800 95 { _has 'Load Game' } 6 1500
    if (-not $ok) { 'FAIL: Load dialog never opened'; break }
    Start-Sleep 2
    # 2a) type the filename (field at y=345; clear leftovers first) + Open (135,377) until
    #     the Load dialog CLOSES (= Open registered). Typing beats row-clicking: row position
    #     depends on the directory listing, the filename field doesn't.
    for ($i=0; $i -lt 8 -and (_has 'Load Game'); $i++) {
        _click 'Load Game' 200 345; Start-Sleep -Milliseconds 300
        & "$SC\keys.ps1" -Window 'Load Game' -Key Backspace -Times 40 -DelayMs 10 2>&1 | Out-Null
        & "$SC\keys.ps1" -Window 'Load Game' -Text $save -DelayMs 15 2>&1 | Out-Null
        Start-Sleep -Milliseconds 300
        _click 'Load Game' 135 377; Start-Sleep -Milliseconds 1200
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
    # Create dialog (480x530): OK is at (176,494) — settings (AI count/size/world type)
    # are REMEMBERED from the registry (Create\AiOpponents etc.), set them beforehand.
    $ok = _clickUntil 'Create' 176 494 { _has 'Pick Your Race' } 6 1500
    if (_has 'Pick Your Race'){ Start-Sleep 2; _click 'Race' 40 92; Start-Sleep -Milliseconds 700; _click 'Race' 235 430 }
    $n=0; while(-not (_has 'Area Map') -and (_running) -and $n -lt 240){ Start-Sleep 2; $n++ }
    if(_has 'Area Map'){ 'INGAME' } elseif(_running){ 'NEWGAME-TIMEOUT (worldgen?)' } else { 'PROCESS-GONE' }
  }

  'log' {
    # Tail a known game log by short name — no path hunting, allow-listable.
    #   h.ps1 log              -> same as 'log list'
    #   h.ps1 log perf 20      -> last 20 lines of perf.log
    $logs = [ordered]@{
      perf    = "$WD\perf.log"
      dbg     = 'd:\tmp\dbgcatch.log'
      terrain = "$WD\SDL2Terrain.log"
      leaks   = "$WD\leakstacks.txt"
    }
    if (-not $a1 -or $a1 -eq 'list') {
      foreach ($k in $logs.Keys) {
        $f = Get-Item $logs[$k] -ErrorAction SilentlyContinue
        if ($f) { "{0,-8} {1}  {2:N0} KB  {3}" -f $k, $f.FullName, ($f.Length/1KB), $f.LastWriteTime }
        else    { "{0,-8} {1}  (missing)" -f $k, $logs[$k] }
      }
      break
    }
    if (-not $logs.Contains($a1)) { "unknown log '$a1' (use: $($logs.Keys -join ' | ') | list)"; break }
    $n = if($a2){[int]$a2}else{15}
    Get-Content $logs[$a1] -Tail $n -ErrorAction SilentlyContinue
  }

  'grep' {
    # Regex-search the tail of a known log.  h.ps1 grep <pattern> [perf|dbg|terrain|leaks] [tailN]
    if (-not $a1) { 'usage: h.ps1 grep <pattern> [perf|dbg|terrain|leaks] [tailN]'; break }
    $logs = @{ perf = "$WD\perf.log"; dbg = 'd:\tmp\dbgcatch.log'; terrain = "$WD\SDL2Terrain.log"; leaks = "$WD\leakstacks.txt" }
    $name = if($a2 -and $logs.ContainsKey($a2)){ $a2 } else { 'perf' }
    $n = if($a3){[int]$a3} elseif($a2 -and -not $logs.ContainsKey($a2)){[int]$a2} else {400}
    $p = $logs[$name]
    if (-not (Test-Path $p)) { "no $name log at $p"; break }
    $hits = Get-Content $p -Tail $n | Select-String -Pattern $a1
    if ($hits) { $hits | Select-Object -Last 12 | ForEach-Object { $_.Line } } else { "no match for '$a1' in last $n lines of $name" }
  }

  'pvshot' {
    # Capture the TRANSIENT gesture-preview frames (the ones a normal shot always misses
    # because the settle rebuild lands first). A background job streams alternating out/in
    # notches with 80ms gaps -- each notch resets the 120ms settle window, so the preview
    # stays up for the whole stream -- while the foreground takes screenshots mid-stream.
    if (-not (_has 'Area Map')) { 'NOT IN-GAME (run: h.ps1 load)'; break }
    $n = if($a1){[int]$a1}else{2}
    $p = "$WD\perf.log"
    # t-stamp of the last flushed line BEFORE the stream; the check below sums pv.frame
    # over lines NEWER than this (a before/after sliding-window sum mis-verified: by the
    # time the after-sum ran, the before-lines had scrolled out of the window).
    $tStart = 0
    if (Test-Path $p) {
      $tv = @(Get-Content $p -Tail 2 | ForEach-Object { if($_ -match '^t=(\d+)'){ [int]$matches[1] } }) | Select-Object -Last 1
      if ($tv) { $tStart = [int]$tv }
    }
    # ONE zoom.ps1 process alternating out/in at 80ms (-Wiggle) — separate single-notch
    # invocations pay ~1s startup each and never hold the 120ms settle window open.
    $job = Start-Job -ScriptBlock {
      & 'd:\Enemy Nations\src\zoom.ps1' -Wiggle 60 -DelayMs 80 -Window map 2>$null | Out-Null
    }
    _wake                            # -Screen captures go black under the screensaver
    Start-Sleep -Milliseconds 1500   # job spin-up + the stream begins
    for ($i = 1; $i -le $n; $i++) {
      & "$SC\screenshot.ps1" -Window map -Full -Screen -Out "d:\tmp\pv$i.png" 2>&1 | Select-Object -Last 1
      Start-Sleep -Milliseconds 400
    }
    Wait-Job $job -Timeout 20 | Out-Null; Remove-Job $job -Force
    Start-Sleep 4   # perf.log flushes ~1 line/sec with buffering lag — don't race it
    $pvSum = 0; $zoomSum = 0
    if (Test-Path $p) {
      Get-Content $p -Tail 60 | ForEach-Object {
        if ($_ -match '^t=(\d+)' -and [int]$matches[1] -gt $tStart) {
          if ($_ -match 'pv\.frame=(\d+)')   { $pvSum   += [int]$matches[1] }
          if ($_ -match 'rb\.zoomchg=(\d+)') { $zoomSum += [int]$matches[1] }
        }
      }
    }
    if ($pvSum -gt 0) { "preview frames during stream: $pvSum (zoom changes: $zoomSum) -- shots are MID-PREVIEW candidates" }
    else { "WARNING: no preview frames during stream (zoom changes: $zoomSum) -- shots are settled frames" }
  }

  'mouse' {
    # REAL mouse click (cursor actually moves): needed where the game reads true cursor
    # position / WM_MOUSEMOVE (the build/landing cursor) — PostMessage clicks can't move it.
    # Safe only when nobody is at the machine (it steals the pointer briefly).
    if (-not $a1 -or -not $a2) { 'usage: h.ps1 mouse <x> <y> [role]'; break }
    $role = if($a3){$a3}else{'map'}
    . (Join-Path $SC 'harness-common.ps1')
    $info = Resolve-GameTarget -Window $role
    if (-not ('HXM.RM' -as [type])) {
      Add-Type -Name RM -Namespace HXM -MemberDefinition '[DllImport("user32.dll")]public static extern bool SetCursorPos(int x, int y); [DllImport("user32.dll")]public static extern void mouse_event(uint f, uint x, uint y, uint d, int e); [DllImport("user32.dll")]public static extern bool ClientToScreen(IntPtr h, ref HXM.PT p); [DllImport("user32.dll")]public static extern bool SetForegroundWindow(IntPtr h); public struct PT { public int X; public int Y; }'
    }
    $p = New-Object HXM.PT; $p.X = [int]$a1; $p.Y = [int]$a2
    [void][HXM.RM]::ClientToScreen($info.Hwnd, [ref]$p)
    [void][HXM.RM]::SetForegroundWindow($info.Hwnd)
    Start-Sleep -Milliseconds 300
    [void][HXM.RM]::SetCursorPos($p.X - 25, $p.Y - 25)   # approach move so a MOUSEMOVE fires
    Start-Sleep -Milliseconds 200
    [void][HXM.RM]::SetCursorPos($p.X, $p.Y)
    Start-Sleep -Milliseconds 400
    [HXM.RM]::mouse_event(2, 0, 0, 0, 0)   # LEFTDOWN
    Start-Sleep -Milliseconds 120
    [HXM.RM]::mouse_event(4, 0, 0, 0, 0)   # LEFTUP
    "real-mouse click at client ($a1,$a2) of '$($info.Title)'"
  }

  'land' {
    # Land the starting rocket: real-mouse move+click on the Area Map (the landing cursor
    # follows only the real pointer). Defaults to just left of window centre (the initial
    # cursor spot is centre; moving off then clicking validates fresh foundation state).
    $lx = if($a1){[int]$a1}else{1000}
    $ly = if($a2){[int]$a2}else{650}
    & $PSCommandPath mouse $lx $ly map
    Start-Sleep 4
    'landed (verify with: h.ps1 shot map)'
  }

  'save' {
    # Save the running game. Flow: Ctrl+O (Game Options) -> Save Game (button 100,184) ->
    # focus filename (200,345) -> clear -> type -> Save (185,376). Verifies the file.
    # SaveGame silently refuses during the rocket phase (rocket_ready/pos/wait) - land first.
    $name = if($a1){$a1}else{'savegame'}
    if (-not (_has 'Area Map')) { 'NOT IN-GAME'; break }
    & "$SC\keys.ps1" -Window main -Key O -Ctrl 2>&1 | Out-Null
    Start-Sleep 2
    _click 'Game Options' 100 184; Start-Sleep 2
    if (-not (_has 'Save Game')) { 'FAIL: Save dialog never opened'; break }
    _click 'Save Game' 200 345; Start-Sleep -Milliseconds 300
    & "$SC\keys.ps1" -Window 'Save Game' -Key Backspace -Times 30 -DelayMs 10 2>&1 | Out-Null
    & "$SC\keys.ps1" -Window 'Save Game' -Text $name -DelayMs 20 2>&1 | Out-Null
    Start-Sleep -Milliseconds 300
    _click 'Save Game' 185 376
    Start-Sleep 12
    $f = Get-Item "D:\Enemy Nations\src\cmakeBuild-x64\enations_latest\src\Debug\$name.en" -ErrorAction SilentlyContinue
    if ($f) { "SAVED $($f.Name) ($([math]::Round($f.Length/1MB,2)) MB)" }
    else { 'SAVE NOT WRITTEN (rocket phase? dialog flow broke?) - check h.ps1 shot main' }
  }

  'profile' {
    # Poor-man's CPU profiler: N dbgstack samples (default 6, ~3s apart) -> histogram of
    # the top-of-stack frames (game modules only), plus the raw samples in d:\tmp\prof-*.txt.
    #   h.ps1 profile [samples]
    $n = if($a1){[int]$a1}else{6}
    Remove-Item 'd:\tmp\prof-*.txt' -ErrorAction SilentlyContinue
    for ($i = 1; $i -le $n; $i++) {
      & "$SC\dbgstack.ps1" 2>&1 | Out-File "d:\tmp\prof-$i.txt"
      if ($i -lt $n) { Start-Sleep 3 }
    }
    "== top-frame histogram over $n samples (frames [0]-[2], game code only) =="
    Select-String -Path 'd:\tmp\prof-*.txt' -Pattern '^\s+\[\s*[012]\]' | ForEach-Object {
      if ($_.Line -match '\]\s+0x[0-9A-Fa-f]+\s+(.+?)(\s+\(|$)') { $matches[1].Trim() }
    } | Group-Object | Sort-Object Count -Descending | Select-Object -First 14 |
      ForEach-Object { '{0,3}x  {1}' -f $_.Count, $_.Name }
    "(full stacks: d:\tmp\prof-1..$n.txt)"
  }

  'saves' {
    # List save files in the two locations the game uses, newest first.
    Get-ChildItem 'D:\Enemy Nations\src\cmakeBuild-x64\enations_latest\src\Debug\*.en','d:\Enemy Nations\*.en' -ErrorAction SilentlyContinue |
      Sort-Object LastWriteTime -Descending | Select-Object -First 12 |
      ForEach-Object { '{0,-28} {1,8:N0} KB  {2}' -f $_.Name, ($_.Length/1KB), $_.LastWriteTime }
  }

  'share' {
    # Upload a screenshot (or any file) to litterbox.catbox.moe (1h temp host) and print
    # the URL — so the user can SEE a capture without being at the machine.
    #   h.ps1 share                -> uploads d:\tmp\h.png (the default shot target)
    #   h.ps1 share d:\tmp\pv1.png [24h]
    $f = if($a1){$a1}else{'d:\tmp\h.png'}
    if (-not (Test-Path $f)) { "no file: $f"; break }
    $ttl = if($a2 -and @('1h','12h','24h','72h') -contains $a2){$a2}else{'12h'}
    $url = & curl.exe -s -F "reqtype=fileupload" -F "time=$ttl" -F "fileToUpload=@$f" https://litterbox.catbox.moe/resources/internals/api.php
    if ($url -match '^https://') { "$url  (expires $ttl)" } else { "upload failed: $url" }
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

  default { "verbs: launch | load [save] | save [name] | saves | land [x y] | mouse <x y> [role] | newgame | measure | zoom <in|out> N | pvshot [n] | shot [role] | share [file] | wake | status | fresh | windows | perf | log [name] [n] | grep <pat> [name] | pan | click | rotate | waitmap" }
}
