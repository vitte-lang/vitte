#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SNAP_DIR="$ROOT_DIR/tests/tui_snapshots"
TMP_DIR="${TMPDIR:-/tmp}/vitte_tui_snapshots.$$"
mkdir -p "$SNAP_DIR" "$TMP_DIR"
trap 'rm -rf "$TMP_DIR"' EXIT

BIN="$ROOT_DIR/bin/vitte-ide"
if [ ! -x "$BIN" ]; then
  echo "[tui-ide-snapshots] building vitte-ide"
  make -C "$ROOT_DIR" vitte-ide >/dev/null
fi

if ! command -v script >/dev/null 2>&1; then
  echo "[tui-ide-snapshots][error] missing 'script' command"
  exit 1
fi

RAW="$TMP_DIR/raw.log"
NORM="$TMP_DIR/normalized.log"

# Launch IDE in PTY and immediately quit with 'q'
script -q -c "bash -lc \"cd '$ROOT_DIR' && printf 'q' | '$BIN' '$ROOT_DIR'\"" "$RAW" >/dev/null 2>&1 || true

# Strip ANSI control sequences and unstable absolute paths/timestamps.
perl -pe 's/\e\[[0-9;?]*[A-Za-z]//g; s/\r//g; s/\x1b\][^\a]*\a//g;' "$RAW" \
  | sed "s|$ROOT_DIR|<ROOT>|g" \
  | sed '/^Script started on /d;/^Script done on /d' \
  | sed 's/[[:space:]]\+$//' \
  | awk 'NF || NR<80 {print}' > "$NORM"

SNAP="$SNAP_DIR/vitte_ide_quit.must"
if [ "${1:-}" = "--update" ]; then
  cp "$NORM" "$SNAP"
  echo "[tui-ide-snapshots] updated $SNAP"
  exit 0
fi

if [ ! -f "$SNAP" ]; then
  echo "[tui-ide-snapshots][error] missing snapshot $SNAP (run --update)"
  exit 1
fi

if ! diff -u "$SNAP" "$NORM"; then
  echo "[tui-ide-snapshots][error] snapshot mismatch"
  exit 1
fi

echo "[tui-ide-snapshots] OK"
