#!/usr/bin/env sh
set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
BIN="${BIN:-$ROOT_DIR/bin/vitte}"
SRC="${SRC:-$ROOT_DIR/src/vitte/compiler/main.vit}"
TMP_DIR="$(mktemp -d "$ROOT_DIR/target/determinism_smoke.XXXXXX")"
trap 'rm -rf "$TMP_DIR"' EXIT

[ -x "$BIN" ] || { echo "[determinism-smoke][error] missing binary: $BIN" >&2; exit 1; }
[ -f "$SRC" ] || { echo "[determinism-smoke][error] missing source: $SRC" >&2; exit 1; }

"$BIN" parse --parse-silent --deterministic --dump-ast-json --lang=en "$SRC" > "$TMP_DIR/a.out" 2> "$TMP_DIR/a.err"
"$BIN" parse --parse-silent --deterministic --dump-ast-json --lang=en "$SRC" > "$TMP_DIR/b.out" 2> "$TMP_DIR/b.err"

cmp "$TMP_DIR/a.out" "$TMP_DIR/b.out" >/dev/null
cmp "$TMP_DIR/a.err" "$TMP_DIR/b.err" >/dev/null

echo "[determinism-smoke] OK"
