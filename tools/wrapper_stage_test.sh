#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"
VITTE_BIN="${VITTE_BIN:-$ROOT_DIR/bin/vitte}"
S1="${S1:-$ROOT_DIR/toolchain/stage1/build/vittec1}"
S2="${S2:-$ROOT_DIR/toolchain/stage2/build/vittec}"
SRC="${SRC:-$ROOT_DIR/tests/strict_ok.vit}"

log() { printf "[wrapper-stage-test] %s\n" "$*"; }
die() { printf "[wrapper-stage-test][error] %s\n" "$*" >&2; exit 1; }

[ -x "$VITTE_BIN" ] || die "missing vitte binary: $VITTE_BIN"
[ -f "$SRC" ] || die "missing input file: $SRC"
[ -x "$S1" ] || die "missing wrapper: $S1"
[ -x "$S2" ] || die "missing wrapper: $S2"

for w in "$S1" "$S2"; do
  log "$(basename "$w") parse --stage parse"
  out="$(VITTE_BIN="$VITTE_BIN" "$w" parse --stage parse "$SRC" 2>&1)"
  if ! grep -Fq "[driver] parse ok" <<<"$out"; then
    printf "%s\n" "$out"
    die "wrapper did not forward --stage parse correctly: $w"
  fi
done

log "OK"
