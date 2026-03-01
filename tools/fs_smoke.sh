#!/usr/bin/env bash
set -euo pipefail
ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"
BIN="${BIN:-$ROOT_DIR/bin/vitte}"
SRC="${SRC:-$ROOT_DIR/tests/fs/vitte_fs_smoke.vit}"

if [ ! -x "$BIN" ]; then
  printf '[fs-smoke] warning: missing %s, skipping\n' "$BIN"
  exit 0
fi

if [ ! -f "$SRC" ]; then
  printf '[fs-smoke][error] missing %s\n' "$SRC" >&2
  exit 1
fi

if "$BIN" check --lang=en "$SRC" >/tmp/vitte-fs-smoke.out 2>&1; then
  printf '[fs-smoke] OK %s\n' "$SRC"
  exit 0
fi
cat /tmp/vitte-fs-smoke.out >&2 || true
printf '[fs-smoke][error] failed for %s\n' "$SRC" >&2
exit 1
