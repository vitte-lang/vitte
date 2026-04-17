#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"
BIN="${BIN:-$ROOT_DIR/bin/vitte}"
REPORT_DIR="${REPORT_DIR:-$ROOT_DIR/target/reports}"
OUT_DIR="${OUT_DIR:-$ROOT_DIR/target/crab_obj}"
MODE="${1:-check}"

mkdir -p "$REPORT_DIR" "$OUT_DIR"

case "$MODE" in
  check) FLAGS=(check --lang=en --allow-internal --resolve-only) ;;
  check-full) FLAGS=(check --lang=en --allow-internal) ;;
  emit-obj) FLAGS=(build --lang=en --allow-internal --emit-obj) ;;
  *)
    echo "usage: tools/crab_workspace.sh [check|check-full|emit-obj]" >&2
    exit 2
    ;;
esac

LOG="$REPORT_DIR/crab_workspace_${MODE}.log"
: > "$LOG"
total=0
fail=0

while IFS= read -r mod; do
  total=$((total + 1))
  pkg="$(basename "$(dirname "$mod")")"
  if [ "$MODE" = "emit-obj" ]; then
    out="$OUT_DIR/$pkg/lib${pkg}.o"
    mkdir -p "$(dirname "$out")"
    if "$BIN" "${FLAGS[@]}" "$mod" -o "$out" >/dev/null 2>>"$LOG"; then
      printf "[ok] %s -> %s\n" "$mod" "$out" >>"$LOG"
    else
      fail=$((fail + 1))
      printf "[fail] %s\n" "$mod" >>"$LOG"
    fi
  else
    if "$BIN" "${FLAGS[@]}" "$mod" >/dev/null 2>>"$LOG"; then
      printf "[ok] %s\n" "$mod" >>"$LOG"
    else
      fail=$((fail + 1))
      printf "[fail] %s\n" "$mod" >>"$LOG"
    fi
  fi
done < <(find "$ROOT_DIR/src/vitte/packages" -maxdepth 2 -type f -path "*/crab*/mod.vit" | sort)

printf "[crab-workspace:%s] total=%d fail=%d pass=%d\n" "$MODE" "$total" "$fail" "$((total - fail))"
printf "[crab-workspace:%s] log=%s\n" "$MODE" "$LOG"
if [ "$fail" -ne 0 ]; then
  exit 1
fi

