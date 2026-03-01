#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"
BIN="${BIN:-$ROOT_DIR/bin/vitte}"
SRC="${SRC:-$ROOT_DIR/tests/std/vitte_std_smoke.vit}"

if [ ! -x "$BIN" ]; then
  printf '[std-smoke] warning: missing %s, skipping\n' "$BIN"
  exit 0
fi

if [ ! -f "$SRC" ]; then
  printf '[std-smoke][error] missing %s\n' "$SRC" >&2
  exit 1
fi

if "$BIN" check --lang=en "$SRC" >/tmp/vitte-std-smoke.out 2>&1; then
  printf '[std-smoke] OK %s\n' "$SRC"
  exit 0
fi
cat /tmp/vitte-std-smoke.out >&2 || true
printf '[std-smoke][error] failed for %s\n' "$SRC" >&2
exit 1
