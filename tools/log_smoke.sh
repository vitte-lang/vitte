#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"
BIN="${BIN:-$ROOT_DIR/bin/vitte}"
SRC="${SRC:-$ROOT_DIR/tests/log/vitte_log_smoke.vit}"

if [ ! -x "$BIN" ]; then
  printf '[log-smoke] warning: missing %s, skipping\n' "$BIN"
  exit 0
fi

if [ ! -f "$SRC" ]; then
  printf '[log-smoke][error] missing %s\n' "$SRC" >&2
  exit 1
fi

if "$BIN" check --lang=en "$SRC" >/tmp/vitte-log-smoke.out 2>&1; then
  printf '[log-smoke] OK %s\n' "$SRC"
  exit 0
fi
cat /tmp/vitte-log-smoke.out >&2 || true
printf '[log-smoke][error] failed for %s\n' "$SRC" >&2
exit 1
