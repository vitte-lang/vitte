#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
REPORT_DIR="$ROOT_DIR/target/reports"
SUMMARY="$REPORT_DIR/packages_check_all_summary.txt"
FAILURES="$REPORT_DIR/packages_check_all_failures.log"
CHECK_SCRIPT="$ROOT_DIR/tools/package_check_portable.sh"

mkdir -p "$REPORT_DIR"
: >"$SUMMARY"
: >"$FAILURES"

total=0
ok=0
fail=0

legacy_structural_ok() {
  local src="$1"
  if rg -q '^[[:space:]]*space[[:space:]]+' "$src"; then
    return 0
  fi
  return 1
}

while IFS= read -r src; do
  total=$((total + 1))
  if rg -q '^[[:space:]]*package[[:space:]]+vitte/' "$src"; then
    if "$CHECK_SCRIPT" "$src" >/dev/null 2>&1; then
      ok=$((ok + 1))
      printf '[ok] %s\n' "$src" >>"$SUMMARY"
    else
      fail=$((fail + 1))
      printf '[fail] %s\n' "$src" >>"$SUMMARY"
      {
        printf '=== %s ===\n' "$src"
        "$CHECK_SCRIPT" "$src" 2>&1 || true
        printf '\n'
      } >>"$FAILURES"
    fi
  else
    if "$ROOT_DIR/bin/vitte" check --lang=en --allow-internal --resolve-only "$src" >/dev/null 2>&1 || legacy_structural_ok "$src"; then
      ok=$((ok + 1))
      printf '[ok][legacy] %s\n' "$src" >>"$SUMMARY"
    else
      fail=$((fail + 1))
      printf '[fail][legacy] %s\n' "$src" >>"$SUMMARY"
      {
        printf '=== %s ===\n' "$src"
        "$ROOT_DIR/bin/vitte" check --lang=en --allow-internal --resolve-only "$src" 2>&1 || true
        printf '\n'
      } >>"$FAILURES"
    fi
  fi
done < <(find "$ROOT_DIR/src/vitte/packages" -name mod.vit | sort)

{
  printf 'total=%d\n' "$total"
  printf 'ok=%d\n' "$ok"
  printf 'fail=%d\n' "$fail"
  printf 'summary=%s\n' "$SUMMARY"
  printf 'failures=%s\n' "$FAILURES"
} >>"$SUMMARY"

printf '[packages-check-all] total=%d ok=%d fail=%d\n' "$total" "$ok" "$fail"
printf '[packages-check-all] summary: %s\n' "$SUMMARY"
if [ "$fail" -gt 0 ]; then
  printf '[packages-check-all] failures: %s\n' "$FAILURES" >&2
  exit 1
fi
