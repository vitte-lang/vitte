#!/usr/bin/env bash
set -euo pipefail

if [ "$#" -ne 3 ]; then
  echo "usage: $0 <vittec-bin> <source.vitte> <expected.txt>" >&2
  exit 2
fi

VITTEC_BIN=$1
SOURCE_PATH=$2
EXPECTED=$3

if [ ! -x "$VITTEC_BIN" ]; then
  echo "error: vittec binary not executable: $VITTEC_BIN" >&2
  exit 1
fi

if [ ! -f "$SOURCE_PATH" ]; then
  echo "error: missing source file: $SOURCE_PATH" >&2
  exit 1
fi

if [ ! -f "$EXPECTED" ]; then
  echo "error: missing expected output: $EXPECTED" >&2
  exit 1
fi

TMP_OUT=$(mktemp)
cleanup() {
  rm -f "$TMP_OUT"
}
trap cleanup EXIT

"$VITTEC_BIN" --tokens "$SOURCE_PATH" >"$TMP_OUT"

diff -u "$EXPECTED" "$TMP_OUT"
