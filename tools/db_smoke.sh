#!/usr/bin/env bash
set -euo pipefail
ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"
BIN="${BIN:-$ROOT_DIR/bin/vitte}"
SRC="${SRC:-$ROOT_DIR/tests/db/vitte_db_smoke.vit}"

if [ ! -x "$BIN" ]; then
  printf '[db-smoke] warning: missing %s, skipping\n' "$BIN"
  exit 0
fi

if [ ! -f "$SRC" ]; then
  printf '[db-smoke][error] missing %s\n' "$SRC" >&2
  exit 1
fi

if "$BIN" check --lang=en "$SRC" >/tmp/vitte-db-smoke.out 2>&1; then
  printf '[db-smoke] OK %s\n' "$SRC"
  exit 0
fi
cat /tmp/vitte-db-smoke.out >&2 || true
printf '[db-smoke][error] failed for %s\n' "$SRC" >&2
exit 1
