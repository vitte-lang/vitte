#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"
REPORT_DIR="${REPORT_DIR:-$ROOT_DIR/target/reports/modules}"
REPORT_FILE="${REPORT_FILE:-$REPORT_DIR/summary.txt}"

mkdir -p "$REPORT_DIR"

if [ ! -d "$ROOT_DIR/tests/modules" ]; then
  printf "[modules-report] skip: tests/modules not present in this Vitte-only checkout\n"
  exit 0
fi

module_count="$(find "$ROOT_DIR/tests/modules" -type f -name '*.vit' | wc -l | tr -d '[:space:]')"
snapshot_count=0
if [ -d "$ROOT_DIR/tests/modules/snapshots" ]; then
  snapshot_count="$(find "$ROOT_DIR/tests/modules/snapshots" -type f -name '*.cmd' | wc -l | tr -d '[:space:]')"
fi

{
  printf "tests_modules=%s\n" "$module_count"
  printf "snapshot_cmds=%s\n" "$snapshot_count"
} > "$REPORT_FILE"

printf "[modules-report] wrote %s\n" "$REPORT_FILE"
