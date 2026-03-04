#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"
BIN="${BIN:-$ROOT_DIR/bin/vitte}"

log() { printf "[core-build] %s\n" "$*"; }
die() { printf "[core-build][error] %s\n" "$*" >&2; exit 1; }

[ -x "$BIN" ] || die "missing binary: $BIN"

trap 'log "interrupted"; exit 130' INT TERM

cxx_ready() {
  if ! command -v clang++ >/dev/null 2>&1; then
    return 1
  fi
  if ! printf '#include <cstdint>\n#include <cstddef>\nint main(){return 0;}\n' \
      | clang++ -x c++ -std=c++20 -fsyntax-only - >/dev/null 2>&1; then
    return 1
  fi
  return 0
}

if ! cxx_ready; then
  log "skip: C++ toolchain not ready (clang++/stdlib headers missing)"
  exit 0
fi

total=0
ok=0
fail=0

for file in "$ROOT_DIR"/examples/core_project_*.vit; do
  case "$file" in
    *.reduced.vit|*.reduce.tmp.vit) continue ;;
  esac
  total=$((total + 1))
  log "build: $file"
  if "$BIN" build "$file"; then
    ok=$((ok + 1))
  else
    fail=$((fail + 1))
  fi
done

log "summary: total=$total ok=$ok fail=$fail"
if [ "$fail" -ne 0 ]; then
  exit 1
fi
