#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
VITTE="$ROOT_DIR/bin/vitte"

log() { printf "[arduino-build] %s\n" "$*"; }
die() { printf "[arduino-build][error] %s\n" "$*" >&2; exit 1; }

[ -x "$VITTE" ] || die "missing compiler: $VITTE"

total=0
ok=0
fail=0
interrupted=0

on_interrupt() {
  interrupted=1
  log "interrupted"
}
trap on_interrupt INT

for file in "$ROOT_DIR"/examples/arduino_*.vit; do
  [ -f "$file" ] || continue
  total=$((total + 1))
  if [ "$interrupted" -ne 0 ]; then
    break
  fi

  log "emit: $file"
  if "$VITTE" emit --target arduino-uno "$file"; then
    ok=$((ok + 1))
  else
    fail=$((fail + 1))
  fi
done

log "summary: total=$total ok=$ok fail=$fail"
[ "$interrupted" -eq 0 ] || exit 130
[ "$fail" -eq 0 ]
