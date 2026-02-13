#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"
BIN="${BIN:-$ROOT_DIR/bin/vitte}"

log() { printf "[core-build] %s\n" "$*"; }
die() { printf "[core-build][error] %s\n" "$*" >&2; exit 1; }

[ -x "$BIN" ] || die "missing binary: $BIN"

trap 'log "interrupted"; exit 130' INT TERM

total=0
ok=0
fail=0

for file in "$ROOT_DIR"/examples/core_project_*.vit; do
  total=$((total + 1))
  log "build: $file"
  if "$BIN" build "$file"; then
    ok=$((ok + 1))
  else
    fail=$((fail + 1))
  fi
done

log "summary: total=$total ok=$ok fail=$fail"
if [ "$fail" -ne 0 ]; then
  exit 1
fi
