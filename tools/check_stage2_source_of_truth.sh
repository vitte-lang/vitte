#!/usr/bin/env sh
set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
STAGE2_SH="$ROOT_DIR/toolchain/scripts/bootstrap/stage2.sh"

expect_root='COMPILER_SOURCE_ROOT="$ROOT_DIR/src/vitte/compiler"'
expect_entry='COMPILER_ENTRY_POINT="$COMPILER_SOURCE_ROOT/main.vit"'
expect_backend='VITTE_BACKEND_MODE="${VITTE_BACKEND_MODE:-native}"'
expect_fallback='VITTE_BACKEND_FALLBACK="${VITTE_BACKEND_FALLBACK:-0}"'
expect_bridge='VITTE_BOOTSTRAP_ALLOW_FULL_COMPILER_BRIDGE="${VITTE_BOOTSTRAP_ALLOW_FULL_COMPILER_BRIDGE:-0}"'

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

grep -F "$expect_backend" "$STAGE2_SH" >/dev/null || {
  echo "[stage2-source-of-truth][error] native backend must be the stage2 default" >&2
  exit 1
}

grep -F "$expect_fallback" "$STAGE2_SH" >/dev/null || {
  echo "[stage2-source-of-truth][error] native stage2 must not silently fall back to shell output" >&2
  exit 1
}

grep -F "$expect_bridge" "$STAGE2_SH" >/dev/null || {
  echo "[stage2-source-of-truth][error] native stage2 must keep the bootstrap bridge disabled by default" >&2
  exit 1
}

if grep -E 'build_native_launcher|VITTE_NATIVE_BRIDGE_COMPAT|native compat' "$STAGE2_SH" >/dev/null; then
  echo "[stage2-source-of-truth][error] native mode must not wrap a shell bootstrap artifact as a machine compiler" >&2
  exit 1
fi

echo "[stage2-source-of-truth] ok"
