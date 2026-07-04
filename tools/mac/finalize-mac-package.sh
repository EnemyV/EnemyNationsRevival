#!/usr/bin/env bash
# finalize-mac-package.sh — MUST be the LAST step of any macOS package re-cut,
# run AFTER all install_name_tool @loader_path rewrites and BEFORE zipping.
#
# WHY: our mac ship bundles a ~27-dylib Homebrew closure next to `enations`,
# rewritten to @loader_path via `install_name_tool`. ANY install_name_tool edit
# invalidates a Mach-O's embedded (ad-hoc) code signature. On Apple Silicon a
# dylib/binary with an invalid signature faults in as cs_invalid_page and the
# kernel SIGKILLs the process on first launch from a clean/quarantined extract.
# Build-green and even an on-box smoke can MISS this (amfid caches cdhashes, and
# a smoke may load the valid build-tree dylibs, not the rewritten closure copies).
# The only fix is to re-sign every Mach-O in the package AFTER the rewrites.
#
# Usage: finalize-mac-package.sh <package-dir>
#   e.g. tools/mac/finalize-mac-package.sh release-artifacts/EnemyNations-3.00.000-mac-arm64
# Exits non-zero if any Mach-O still fails `codesign --verify` — never ship on failure.
set -euo pipefail

DIR="${1:?usage: finalize-mac-package.sh <package-dir>}"
[ -d "$DIR" ] || { echo "not a dir: $DIR" >&2; exit 2; }
cd "$DIR"

echo "==> Re-signing all Mach-O (dylibs first, then executables) in $DIR"
# Sign dependencies before dependents; ad-hoc sigs don't chain, but this order is conventional.
shopt -s nullglob
signed=0
for f in *.dylib *.so; do
  codesign --force --sign - --timestamp=none "$f" >/dev/null 2>&1 && signed=$((signed+1))
done
# Main executable(s): anything Mach-O and executable that isn't a lib.
for f in enations iserve; do
  [ -f "$f" ] && { codesign --force --sign - --timestamp=none "$f" >/dev/null 2>&1 && signed=$((signed+1)); }
done
echo "    signed $signed Mach-O file(s)"

echo "==> Verifying (strict) — the ship gate"
fail=0
for f in enations iserve *.dylib *.so; do
  [ -f "$f" ] || continue
  if ! codesign --verify --strict "$f" >/dev/null 2>&1; then
    echo "    INVALID SIGNATURE (would SIGKILL on arm64): $f" >&2
    fail=$((fail+1))
  fi
done
if [ "$fail" -ne 0 ]; then
  echo "==> FAIL: $fail file(s) still invalid — DO NOT ZIP/SHIP." >&2
  exit 1
fi
echo "==> OK: every Mach-O in the package has a valid signature. Safe to zip."
echo "    Zip with:  ditto -c -k --keepParent \"$DIR\" \"$DIR.zip\"  (ditto preserves the signatures)"
