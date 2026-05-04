#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"
cd "$ROOT_DIR"

BIN="${BIN:-$ROOT_DIR/bin/vitte}"
TEST_DIR="$ROOT_DIR/tests/repro"

log() { printf "[repro-gen] %s\n" "$*"; }
die() { printf "[repro-gen][error] %s\n" "$*" >&2; exit 1; }

[ -x "$BIN" ] || die "missing binary: $BIN (run make build)"
[ -d "$TEST_DIR" ] || die "missing dir: $TEST_DIR"

shopt -s nullglob
files=("$TEST_DIR"/*.vit)
[ "${#files[@]}" -gt 0 ] || die "no .vit files in $TEST_DIR"

for src in "${files[@]}"; do
  name="$(basename "$src" .vit)"
  dst="$TEST_DIR/$name.repro.vit"
  log "emit $name -> $dst"
  "$BIN" emit --stdout --repro "$src" > "$dst"
done

log "done"
