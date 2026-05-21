#!/usr/bin/env sh
set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
STAGE2_SH="$ROOT_DIR/toolchain/scripts/bootstrap/stage2.sh"

expect_root='COMPILER_SOURCE_ROOT="$ROOT_DIR/src/vitte/compiler"'
expect_entry='COMPILER_ENTRY_POINT="$COMPILER_SOURCE_ROOT/driver/compiler.vit"'

[ -f "$STAGE2_SH" ] || {
  echo "[stage2-source-of-truth][error] missing $STAGE2_SH" >&2
  exit 1
}

grep -F "$expect_root" "$STAGE2_SH" >/dev/null || {
  echo "[stage2-source-of-truth][error] stage2.sh compiler source root drifted" >&2
  echo "[stage2-source-of-truth][error] expected: $expect_root" >&2
  exit 1
}

grep -F "$expect_entry" "$STAGE2_SH" >/dev/null || {
  echo "[stage2-source-of-truth][error] stage2.sh compiler entry drifted" >&2
  echo "[stage2-source-of-truth][error] expected: $expect_entry" >&2
  exit 1
}

echo "[stage2-source-of-truth] ok"
