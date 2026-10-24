#!/bin/bash
# Release gate: the macOS Dock + menu bar must be hidden over the fullscreen game.
#
# The Dock/menu bar drawing over the game has regressed at least five times from five
# different causes, so this launches the real build at every UI scale and fails unless
# the game itself reports MAC_DISPLAY_CHECK: PASS. Do NOT delete this script or the
# workflow that runs it when stripping the repo (the 2026-10-06 strip removed the old tests).
#
# usage: tools/mac-display-check.sh <run-dir> [scale ...]     (default scales: 100 150 200 0)
#   <run-dir> holds the built `enations` binary, ENations.dat and assets, and is staged
#   with a sibling HOME at <run-dir>/../home/.config/enations/registry.base (as launch.sh
#   in the 3.01 staging dirs does). 0 = Auto scale.
# exit 0 = every scale PASSed; non-zero = a scale FAILed, DRIFTed or never reported.
set -u
RUN=${1:?usage: $0 <run-dir> [scale ...]}; shift
SCALES=${*:-100 150 200 0}
RUN=$(cd "$RUN" && pwd); HOMEDIR=$(cd "$RUN/../home" && pwd) || exit 2
REG=$HOMEDIR/.config/enations/registry.ini
[ -x "$RUN/enations" ] || { echo "FAIL: no executable $RUN/enations"; exit 2; }

# A leaked override is exactly how this bug shipped before; refuse to run polluted.
for v in EN_FULLSCREEN EN_WINDOWED EN_MAC_USABLE_FULLSCREEN; do
    [ -z "${!v-}" ] || { echo "FAIL: $v is set in the environment ($v=${!v}); unset it"; exit 2; }
done

rc=0
for S in $SCALES; do
    pkill -x enations 2>/dev/null; sleep 1
    cp "$HOMEDIR/.config/enations/registry.base" "$REG"
    printf '%s\n' \
      "HKCU\Software\Second Chance\Second Chance\Game\DataFile=1:$RUN/ENations.dat" \
      "HKCU\Software\Second Chance\Second Chance\Game\NoIntro=4:1" \
      "HKCU\Software\Second Chance\Second Chance\Game\UIScale=4:$S" >> "$REG"
    rm -f "$RUN/GameWindow_Debug.log"
    ( cd "$RUN" && HOME=$HOMEDIR ./enations >/dev/null 2>&1 & )
    line=""
    for _ in $(seq 1 30); do   # up to ~30 s for the window to come up
        sleep 1
        line=$(grep -h MAC_DISPLAY_CHECK "$RUN/GameWindow_Debug.log" 2>/dev/null | head -1)
        [ -n "$line" ] && break
    done
    sleep 5   # let the self-heal run a few cycles; a DRIFT line means something undid the hide
    drift=$(grep -c "MAC_DISPLAY_CHECK: DRIFT" "$RUN/GameWindow_Debug.log" 2>/dev/null)
    pkill -x enations 2>/dev/null
    case "$line" in
        *"MAC_DISPLAY_CHECK: PASS"*) res=PASS ;;
        "")                          res="FAIL (no MAC_DISPLAY_CHECK line)"; rc=1 ;;
        *)                           res="FAIL"; rc=1 ;;
    esac
    [ "${drift:-0}" -gt 0 ] && { res="$res+DRIFT($drift)"; rc=1; }
    echo "scale=$S $res :: ${line:-<none>}"
done
exit $rc
