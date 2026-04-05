#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"
BIN="${BIN:-$ROOT_DIR/bin/vitte}"
SRC="${SRC:-$ROOT_DIR/examples/first_project.vit}"

log() { printf "[incremental-cache-smoke] %s\n" "$*"; }
die() { printf "[incremental-cache-smoke][error] %s\n" "$*" >&2; exit 1; }

[ -x "$BIN" ] || die "missing binary: $BIN"
[ -f "$SRC" ] || die "missing source: $SRC"

out1="$(mktemp "${TMPDIR:-/tmp}/vitte-cache-smoke1-XXXXXX")"
out2="$(mktemp "${TMPDIR:-/tmp}/vitte-cache-smoke2-XXXXXX")"
trap 'rm -f "$out1" "$out2"' EXIT

"$BIN" build --cache-report "$SRC" >"$out1" 2>&1
"$BIN" build --cache-report "$SRC" >"$out2" 2>&1

grep -q "\[cache\] parse=" "$out2" || die "missing cache report in second run"
grep -q "incremental cache hit" "$out2" || die "missing incremental cache hit on second run"

log "OK"
