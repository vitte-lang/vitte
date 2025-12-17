#!/usr/bin/env sh
# C:\Users\vince\Documents\GitHub\vitte\fuzz\scripts\sanitize_ubsan.sh
# Run a target binary with UBSan-focused runtime settings (portable sh).
#
# Usage:
#   ./fuzz/scripts/sanitize_ubsan.sh <target_bin> [args...]
#
# Examples:
#   ./fuzz/scripts/sanitize_ubsan.sh ./fuzz/out/parser_fuzz crash-123
#   ./fuzz/scripts/sanitize_ubsan.sh ./build/bin/vittec --check input.vitte
#
# Notes:
#  - This does not compile anything; it only sets env and execs.
#  - It enables UBSan stack traces and aborts on error (good for fuzzing).

set -eu

BIN="${1:-}"
if [ -z "$BIN" ]; then
  echo "usage: $(basename "$0") <target_bin> [args...]" >&2
  exit 2
fi
shift

if [ ! -x "$BIN" ]; then
  echo "error: target_bin not executable: $BIN" >&2
  exit 1
fi

# Resolve repo root (optional; mainly for log placement)
ROOT_DIR="${ROOT_DIR:-}"
if [ -z "$ROOT_DIR" ]; then
  SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
  ROOT_DIR="$(CDPATH= cd -- "$SCRIPT_DIR/../.." && pwd)"
fi

FUZZ_DIR="$ROOT_DIR/fuzz"
mkdir -p "$FUZZ_DIR/logs"

stamp="$(date +%Y%m%d_%H%M%S 2>/dev/null || echo now)"
log="$FUZZ_DIR/logs/ubsan_${stamp}.log"

# UBSan runtime configuration (override by exporting UBSAN_OPTIONS before calling)
export UBSAN_OPTIONS="${UBSAN_OPTIONS:-\
halt_on_error=1:\
abort_on_error=1:\
print_stacktrace=1:\
report_error_type=1:\
silence_unsigned_overflow=0:\
external_symbolizer_path=:\
symbolize=1:\
exitcode=77}"

# For completeness: disable ASan leak detection if running under UBSan only
export ASAN_OPTIONS="${ASAN_OPTIONS:-detect_leaks=0:abort_on_error=1:exitcode=77}"

# Helpful symbolizer settings
export LLVM_SYMBOLIZER_PATH="${LLVM_SYMBOLIZER_PATH:-}"
export ASAN_SYMBOLIZER_PATH="${ASAN_SYMBOLIZER_PATH:-}"

# Run and tee output to log (portable: redirect; if "tee" exists, use it)
if command -v tee >/dev/null 2>&1; then
  # shellcheck disable=SC2086
  "$BIN" "$@" 2>&1 | tee "$log"
  exit "${PIPESTATUS:-0}"
fi

# No tee available
# shellcheck disable=SC2068
"$BIN" "$@" >"$log" 2>&1
cat "$log"
