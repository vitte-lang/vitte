#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"
BIN="${BIN:-$ROOT_DIR/bin/vitte}"
EXAMPLE="${EXAMPLE:-$ROOT_DIR/examples/first_project.vit}"
OUT_DIR="${OUT_DIR:-$ROOT_DIR/target/quickstart}"
OUT_PATH="${OUT_PATH:-$OUT_DIR/first_project}"

log() { printf "[quickstart-check] %s\n" "$*"; }
die() { printf "[quickstart-check][error] %s\n" "$*" >&2; exit 1; }

[ -x "$BIN" ] || die "missing binary: $BIN"
[ -f "$EXAMPLE" ] || die "missing example: $EXAMPLE"
mkdir -p "$OUT_DIR"

log "check ${EXAMPLE#$ROOT_DIR/}"
"$BIN" check "$EXAMPLE"

log "build ${EXAMPLE#$ROOT_DIR/}"
"$BIN" build "$EXAMPLE" -o "$OUT_PATH"

log "OK"
