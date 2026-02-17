#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"
BIN="${BIN:-$ROOT_DIR/bin/vitte}"
SRC="${SRC:-$ROOT_DIR/tests/modules/mod_graph/main.vit}"
OUT1="${OUT1:-$ROOT_DIR/.vitte-hash-1.cpp}"
OUT2="${OUT2:-$ROOT_DIR/.vitte-hash-2.cpp}"

log() { printf "[same-output-hash] %s\n" "$*"; }
die() { printf "[same-output-hash][error] %s\n" "$*" >&2; exit 1; }

[ -x "$BIN" ] || die "missing binary: $BIN"
[ -f "$SRC" ] || die "missing source: $SRC"

run_emit() {
  local out="$1"
  local tmp="$ROOT_DIR/vitte_out.cpp"
  "$BIN" emit --lang=en --deterministic --repro --repro-strict "$SRC" >/dev/null 2>&1
  [ -f "$tmp" ] || die "expected generated vitte_out.cpp"
  cp "$tmp" "$out"
}

run_emit "$OUT1"
run_emit "$OUT2"

h1="$(shasum -a 256 "$OUT1" | awk '{print $1}')"
h2="$(shasum -a 256 "$OUT2" | awk '{print $1}')"

if [ "$h1" != "$h2" ]; then
  die "hash mismatch: $h1 != $h2"
fi

log "OK ($h1)"
