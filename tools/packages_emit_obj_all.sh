#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"
BIN="${BIN:-$ROOT_DIR/bin/vitte}"
OUT_ROOT="${OUT_ROOT:-$ROOT_DIR/target/packages_obj}"
REPORT_DIR="${REPORT_DIR:-$ROOT_DIR/target/reports}"
SUMMARY="$REPORT_DIR/packages_emit_obj_summary.txt"
FAILURES="$REPORT_DIR/packages_emit_obj_failures.log"

mkdir -p "$OUT_ROOT" "$REPORT_DIR"
: > "$SUMMARY"
: > "$FAILURES"

if [ ! -x "$BIN" ]; then
  echo "[packages-emit-obj][error] missing compiler binary: $BIN" >&2
  exit 2
fi

total=0
ok=0
fail=0

while IFS= read -r src; do
  total=$((total + 1))

  rel="${src#$ROOT_DIR/src/vitte/packages/}"
  pkg="${rel%/mod.vit}"
  leaf="${pkg##*/}"
  pkg_out_dir="$OUT_ROOT/$pkg"
  out_obj="$pkg_out_dir/lib${leaf}.o"
  mkdir -p "$pkg_out_dir"

  if "$BIN" build --lang=en --allow-internal --emit-obj "$src" -o "$out_obj" >/dev/null 2>>"$FAILURES"; then
    ok=$((ok + 1))
    printf "[ok] %s -> %s\n" "$src" "$out_obj" >> "$SUMMARY"
  else
    fail=$((fail + 1))
    printf "[fail] %s\n" "$src" >> "$SUMMARY"
    printf "[fail] %s\n" "$src" >> "$FAILURES"
  fi
done < <(find "$ROOT_DIR/src/vitte/packages" -name mod.vit | sort)

printf "[packages-emit-obj] total=%d ok=%d fail=%d\n" "$total" "$ok" "$fail" | tee -a "$SUMMARY"
printf "[packages-emit-obj] summary: %s\n" "$SUMMARY"
if [ "$fail" -ne 0 ]; then
  printf "[packages-emit-obj] failures: %s\n" "$FAILURES" >&2
  exit 1
fi

