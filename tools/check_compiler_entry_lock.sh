#!/usr/bin/env sh
set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$ROOT_DIR"

expect_root='src/vitte/compiler'
expect_entry='src/vitte/compiler/driver/compiler.vit'

check_stage_file() {
  f="$1"
  root_value=$(awk -F= '/^[[:space:]]*const[[:space:]]+COMPILER_SOURCE_ROOT[[:space:]]*:/ {gsub(/^[[:space:]]*"|"[[:space:]]*$/, "", $2); gsub(/^[[:space:]]+|[[:space:]]+$/, "", $2); print $2; exit}' "$f")
  entry_value=$(awk -F= '/^[[:space:]]*const[[:space:]]+COMPILER_ENTRY_POINT[[:space:]]*:/ {gsub(/^[[:space:]]*"|"[[:space:]]*$/, "", $2); gsub(/^[[:space:]]+|[[:space:]]+$/, "", $2); print $2; exit}' "$f")
  [ "$root_value" = "$expect_root" ] || {
    echo "[compiler-entry-lock][error] $f COMPILER_SOURCE_ROOT=$root_value expected=$expect_root" >&2
    return 1
  }
  [ "$entry_value" = "$expect_entry" ] || {
    echo "[compiler-entry-lock][error] $f COMPILER_ENTRY_POINT=$entry_value expected=$expect_entry" >&2
    return 1
  }
}

check_stage_file toolchain/stage1/src/main.vit
check_stage_file toolchain/stage2/src/main.vit

extra_entries=$(rg -n "COMPILER_ENTRY_POINT[[:space:]]*:[[:space:]]*string[[:space:]]*=" src toolchain tools -S | rg -v "toolchain/stage1/src/main.vit|toolchain/stage2/src/main.vit|src/vitte/compiler/driver/compiler.vit" || true)
if [ -n "$extra_entries" ]; then
  echo "[compiler-entry-lock][error] additional COMPILER_ENTRY_POINT constants found:" >&2
  echo "$extra_entries" >&2
  exit 1
fi

echo "[compiler-entry-lock] ok"
