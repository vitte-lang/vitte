#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"
BIN="${BIN:-$ROOT_DIR/bin/vittec}"
LOG_DIR="${LOG_DIR:-$ROOT_DIR/build/logs}"

mkdir -p "$LOG_DIR"

log() { printf "[sanitizers] %s\n" "$*"; }
die() { printf "[sanitizers][error] %s\n" "$*" >&2; exit 1; }

[ -x "$BIN" ] || die "missing binary: $BIN"

run_and_log() {
  local name="$1"
  local outfile="$2"
  shift 2
  log "running $name -> $outfile"
  set +e
  "$@" >"$outfile" 2>&1
  local rc=$?
  set -e
  if [ "$rc" -ne 0 ]; then
    printf "[sanitizers] command failed (%s) rc=%s\n" "$name" "$rc" >>"$outfile"
  fi
}

run_and_log "debug-asan test" "$LOG_DIR/asan.txt" "$BIN" test --profile debug-asan
run_and_log "debug-ubsan test" "$LOG_DIR/ubsan.txt" "$BIN" test --profile debug-ubsan
run_and_log "debug-tsan test" "$LOG_DIR/tsan.txt" "$BIN" test --profile debug-tsan
run_and_log "debug-asan e2e" "$LOG_DIR/asan.txt" "$BIN" e2e --profile debug-asan

fatal_pattern='(AddressSanitizer|UndefinedBehaviorSanitizer|ThreadSanitizer|runtime error:|heap-use-after-free|double free|data race|SEGV|invalid read|invalid write|LeakSanitizer)'

if grep -Eiq "$fatal_pattern" "$LOG_DIR/asan.txt"; then
  die "critical ASAN failure detected (see $LOG_DIR/asan.txt)"
fi
if grep -Eiq "$fatal_pattern" "$LOG_DIR/ubsan.txt"; then
  die "critical UBSAN failure detected (see $LOG_DIR/ubsan.txt)"
fi
if grep -Eiq "$fatal_pattern" "$LOG_DIR/tsan.txt"; then
  die "critical TSAN failure detected (see $LOG_DIR/tsan.txt)"
fi

log "OK"
