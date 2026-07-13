#!/usr/bin/env sh
set -eu

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
SOURCE="$ROOT_DIR/tests/bootstrap_native/named_consts.vit"
SEED="$ROOT_DIR/toolchain/seed/vittec0.seed"
TMP_DIR="$(mktemp -d "${TMPDIR:-/tmp}/vitte-bootstrap-shell-fixed-point.XXXXXX")"
trap 'rm -rf "$TMP_DIR"' EXIT HUP INT TERM

FIRST="$TMP_DIR/first"
SECOND="$TMP_DIR/second"

"$SEED" dump-native-shell --src "$SOURCE" > "$FIRST"
chmod +x "$FIRST"
"$FIRST" dump-native-shell --src "$SOURCE" > "$SECOND"

if ! cmp -s "$FIRST" "$SECOND"; then
    diff -u "$FIRST" "$SECOND" >&2 || true
    printf '[bootstrap-shell-fixed-point][error] generated shell is not a fixed point\n' >&2
    exit 1
fi

printf '[bootstrap-shell-fixed-point] ok\n'
