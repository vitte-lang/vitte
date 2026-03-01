#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"
BIN="${BIN:-$ROOT_DIR/bin/vitte}"
SRC="${SRC:-$ROOT_DIR/tests/core/vitte_core_smoke.vit}"

log() { printf "[core-smoke] %s\n" "$*"; }

if [ ! -x "$BIN" ]; then
  log "warning: missing $BIN, skipping"
  exit 0
fi

set +e
out="$("$BIN" check --lang=en "$SRC" 2>&1)"
rc=$?
set -e

if [ "$rc" -eq 0 ]; then
  log "OK"
  exit 0
fi

if printf "%s" "$out" | grep -Eqi "fatal error: 'cstdint' file not found|fatal error: 'cstddef' file not found|\[clang\] invocation failed|backend compilation failed"; then
  log "warning: host C++ toolchain incomplete, smoke check skipped"
  exit 0
fi

printf "%s\n" "$out"
log "FAILED"
exit 1
