#!/usr/bin/env sh
set -eu

ROOT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
BIN="$ROOT_DIR/build/vittec"
TEST_DIR="$ROOT_DIR/tests"

if [ ! -x "$BIN" ]; then
  "$ROOT_DIR/scripts/build.sh"
fi

FAIL=0

# Token smoke
OUT="$("$BIN" --tokens "$TEST_DIR/smoke/hello.vitte")" || FAIL=1
echo "$OUT" | grep -q "TK_KW_FN" || FAIL=1
echo "$OUT" | grep -q "TK_KW_RET" || FAIL=1

# Emit C smoke
TMP="$ROOT_DIR/build/_hello.c"
"$BIN" --emit-c "$TEST_DIR/smoke/hello.vitte" -o "$TMP"
grep -q "int main" "$TMP" || FAIL=1

if [ "$FAIL" -ne 0 ]; then
  echo "tests: FAIL"
  exit 1
fi

echo "tests: OK"
