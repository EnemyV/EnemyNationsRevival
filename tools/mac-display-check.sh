#!/bin/bash
# Release gate: on macOS the game must start fullscreen with the Dock and menu bar hidden.
#
# This has regressed many times from different causes (a leaked EN_WINDOWED /
# EN_FULLSCREEN / EN_MAC_USABLE_FULLSCREEN override, an auto-hidden instead of hidden
# Dock, a self-heal that stopped running). The first version of this gate (fb8e8c64,
# 2026-10-24) was never merged, so nothing caught it. tools/package_release.py runs this
# on macOS and refuses to package unless it passes. Do NOT delete it when stripping the repo.
#
# It launches the real build with a clean environment and fails unless the game itself
# logs MAC_DISPLAY_CHECK: PASS (GameWindow.cpp) and no DRIFT line follows within 5 s.
#
# usage: tools/mac-display-check.sh <run-dir>     (the dir holding the built `enations`)
# exit 0 = PASS; non-zero = FAIL, DRIFT, or no report.
set -u
RUN=${1:?usage: $0 <run-dir>}
RUN=$(cd "$RUN" && pwd) || exit 2
[ "$(uname)" = Darwin ] || { echo "mac-display-check: not macOS, skipped"; exit 0; }
[ -x "$RUN/enations" ] || { echo "FAIL: no executable $RUN/enations"; exit 2; }

# A leaked override is how this bug shipped before; never test or package polluted.
for v in EN_FULLSCREEN EN_WINDOWED EN_MAC_USABLE_FULLSCREEN; do
    [ -z "${!v-}" ] || { echo "FAIL: $v is set in the environment ($v=${!v}); unset it"; exit 2; }
done
for f in "$RUN/Info.plist" "$RUN"/*.sh "$RUN"/../Info.plist; do
    [ -f "$f" ] && grep -q -E "EN_(FULLSCREEN|WINDOWED|MAC_USABLE_FULLSCREEN)" "$f" &&
        { echo "FAIL: $f sets a display override"; exit 2; }
done

if pgrep -x enations >/dev/null; then echo "FAIL: another enations is running; quit it first"; exit 2; fi
LOG=$RUN/GameWindow_Debug.log
rm -f "$LOG"
( cd "$RUN" && exec ./enations >/dev/null 2>&1 ) &
PID=$!
line=""
for _ in $(seq 1 30); do   # up to ~30 s for the window to come up
    sleep 1
    line=$(grep -h "MAC_DISPLAY_CHECK" "$LOG" 2>/dev/null | head -1)
    [ -n "$line" ] && break
    kill -0 $PID 2>/dev/null || break
done
sleep 5   # let the self-heal run a few cycles; a DRIFT line means something undid the hide
drift=$(grep -c "MAC_DISPLAY_CHECK: DRIFT" "$LOG" 2>/dev/null)
kill $PID 2>/dev/null; sleep 1; kill -9 $PID 2>/dev/null
case "$line" in
    *"MAC_DISPLAY_CHECK: PASS"*) res=PASS; rc=0 ;;
    "")                          res="FAIL (no MAC_DISPLAY_CHECK line)"; rc=1 ;;
    *)                           res=FAIL; rc=1 ;;
esac
[ "${drift:-0}" -gt 0 ] && { res="$res+DRIFT($drift)"; rc=1; }
echo "mac-display-check: $res :: ${line:-<none>}"
exit $rc
