#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"
BIN="${BIN:-$ROOT_DIR/bin/vitte}"

log() { printf "[check-tests] %s\n" "$*"; }
die() { printf "[check-tests][error] %s\n" "$*" >&2; exit 1; }

[ -x "$BIN" ] || die "missing binary: $BIN"

log "module_smoke"
"$BIN" check "$ROOT_DIR/tests/check/main.vit"

log "hir_mir_repro_alloc_break_index"
"$BIN" build --mir-only "$ROOT_DIR/tests/repro/alloc_hir_break_index.vit"

log "hir_mir_repro_alloc_string_index"
"$BIN" build --mir-only "$ROOT_DIR/tests/repro/alloc_hir_string_index.vit"

log "OK"
