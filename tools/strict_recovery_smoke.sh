#!/usr/bin/env sh
set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
BIN="${BIN:-$ROOT_DIR/bin/vitte}"
SRC="${SRC:-$ROOT_DIR/tests/diag_snapshots/composite_type_arity.vit}"

[ -x "$BIN" ] || { echo "[strict-recovery-smoke][error] missing binary: $BIN" >&2; exit 1; }
[ -f "$SRC" ] || { echo "[strict-recovery-smoke][error] missing source: $SRC" >&2; exit 1; }

set +e
out="$($BIN parse --lang=en "$SRC" 2>&1)"
rc=$?
set -e

[ "$rc" -ne 0 ] || { echo "[strict-recovery-smoke][error] expected parse failure" >&2; exit 1; }
printf '%s\n' "$out" | grep -Eq 'E_|error|diagnostic' || {
  echo "[strict-recovery-smoke][error] expected explicit diagnostics in parse output" >&2
  echo "$out" >&2
  exit 1
}

echo "[strict-recovery-smoke] OK"
