#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"
BIN="${BIN:-$ROOT_DIR/bin/vitte}"

log() { printf "[check-tests] %s\n" "$*"; }
die() { printf "[check-tests][error] %s\n" "$*" >&2; exit 1; }

[ -x "$BIN" ] || die "missing binary: $BIN"

log "module_smoke"
"$BIN" check "$ROOT_DIR/tests/check/main.vit"

log "OK"
