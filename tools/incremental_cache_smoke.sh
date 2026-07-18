#!/usr/bin/env sh
set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
BIN="${BIN:-$ROOT_DIR/bin/vitte}"
SRC="${SRC:-$ROOT_DIR/src/vitte/compiler/main.vit}"
TMP_DIR="$(mktemp -d "$ROOT_DIR/target/incremental_cache_smoke.XXXXXX")"
trap 'rm -rf "$TMP_DIR"' EXIT

[ -x "$BIN" ] || { echo "[incremental-cache-smoke][error] missing binary: $BIN" >&2; exit 1; }
[ -f "$SRC" ] || { echo "[incremental-cache-smoke][error] missing source: $SRC" >&2; exit 1; }

"$BIN" check --lang=en --deterministic "$SRC" > "$TMP_DIR/first.out" 2> "$TMP_DIR/first.err"
"$BIN" check --lang=en --deterministic "$SRC" > "$TMP_DIR/second.out" 2> "$TMP_DIR/second.err"

cmp "$TMP_DIR/first.out" "$TMP_DIR/second.out" >/dev/null
cmp "$TMP_DIR/first.err" "$TMP_DIR/second.err" >/dev/null

echo "[incremental-cache-smoke] OK"
