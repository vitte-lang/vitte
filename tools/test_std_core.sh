#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"
BIN="${BIN:-$ROOT_DIR/bin/vitte}"

log() { printf "[std-core-test] %s\n" "$*"; }
die() { printf "[std-core-test][error] %s\n" "$*" >&2; exit 1; }

[ -x "$BIN" ] || die "missing binary: $BIN"

files=(
  "$ROOT_DIR/tests/std_core_option.vit"
  "$ROOT_DIR/tests/std_core_result.vit"
  "$ROOT_DIR/tests/std_core_panic.vit"
  "$ROOT_DIR/tests/std_core_io_error.vit"
)

for f in "${files[@]}"; do
  [ -f "$f" ] || die "missing test file: $f"
  log "check: ${f#$ROOT_DIR/}"
  "$BIN" check "$f"
done

log "OK"
