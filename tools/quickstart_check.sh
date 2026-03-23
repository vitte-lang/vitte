#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"
BIN="${BIN:-$ROOT_DIR/bin/vitte}"
EXAMPLE="${EXAMPLE:-$ROOT_DIR/examples/first_project.vit}"

log() { printf "[quickstart-check] %s\n" "$*"; }
die() { printf "[quickstart-check][error] %s\n" "$*" >&2; exit 1; }

[ -x "$BIN" ] || die "missing binary: $BIN"
[ -f "$EXAMPLE" ] || die "missing example: $EXAMPLE"

log "check ${EXAMPLE#$ROOT_DIR/}"
"$BIN" check "$EXAMPLE"

log "build ${EXAMPLE#$ROOT_DIR/}"
"$BIN" build "$EXAMPLE"

log "OK"
