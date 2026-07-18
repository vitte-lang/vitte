#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"
VITTE_BIN="${VITTE_BIN:-$ROOT_DIR/bin/vitte}"
VITTEC_BIN="${VITTEC_BIN:-$ROOT_DIR/bin/vittec}"
SRC="${SRC:-$ROOT_DIR/tests/strict_ok.vit}"

log() { printf "[wrapper-stage-test] %s\n" "$*"; }
die() { printf "[wrapper-stage-test][error] %s\n" "$*" >&2; exit 1; }

[ -x "$VITTE_BIN" ] || die "missing vitte binary: $VITTE_BIN"
[ -x "$VITTEC_BIN" ] || die "missing vittec binary: $VITTEC_BIN"
[ -f "$SRC" ] || die "missing input file: $SRC"

for wrapper in "$VITTE_BIN" "$VITTEC_BIN"; do
  log "$(basename "$wrapper") parse --stage parse"
  out="$("$wrapper" parse --stage parse "$SRC" 2>&1)"
  if [ "$out" != "parse ok: $SRC" ]; then
    printf "%s\n" "$out"
    die "active wrapper did not forward --stage parse correctly: $wrapper"
  fi
done

log "OK"
