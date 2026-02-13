#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"
BIN="${BIN:-$ROOT_DIR/bin/vitte}"
OUT_DIR="${OUT_DIR:-$ROOT_DIR/build/stress/alloc_examples}"

log() { printf "[stress-alloc] %s\n" "$*"; }
die() { printf "[stress-alloc][error] %s\n" "$*" >&2; exit 1; }

[ -x "$BIN" ] || die "missing binary: $BIN"
mkdir -p "$OUT_DIR"

FILES=(
  "$ROOT_DIR/examples/alloc_logistics_control_huge.vit"
  "$ROOT_DIR/examples/alloc_greenhouse_ops_huge.vit"
  "$ROOT_DIR/tests/repro/alloc_hir_break_index.vit"
  "$ROOT_DIR/tests/repro/alloc_hir_string_index.vit"
)

STEPS=(
  "parse|parse"
  "check|check"
  "hir|build --hir-only"
  "mir|build --mir-only"
  "emit_cpp|emit"
  "emit_obj|build --emit-obj -o $OUT_DIR/tmp.o"
)

summary_file="$OUT_DIR/summary.txt"
: > "$summary_file"

for file in "${FILES[@]}"; do
  base="$(basename "$file" .vit)"
  log "file: $base"
  for step in "${STEPS[@]}"; do
    name="${step%%|*}"
    args="${step#*|}"
    log_path="$OUT_DIR/${base}.${name}.log"
    set +e
    "$BIN" $args "$file" >"$log_path" 2>&1
    code=$?
    set -e
    printf "%-40s %-10s exit=%d\n" "$base" "$name" "$code" | tee -a "$summary_file"
  done
done

rm -f "$OUT_DIR/tmp.o"
log "summary: $summary_file"
