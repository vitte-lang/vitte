#!/usr/bin/env sh
# minimize.sh
# Minimize a crashing/hanging input for a fuzz target (libFuzzer-compatible runner).
#
# This script is a thin wrapper that:
#  - creates a working directory under fuzz/minimized/<target>/
#  - runs target binary with libFuzzer minimization flags when available
#  - falls back to "copy as-is" if the target isn't a libFuzzer binary
#
# Usage:
#   ./fuzz/scripts/minimize.sh <target> <target_bin> <input_file> [--mode crash|hang] [--timeout N]
#
# Examples:
#   ./fuzz/scripts/minimize.sh lexer ./fuzz/out/lexer_fuzz crash-123
#   ./fuzz/scripts/minimize.sh parser ./fuzz/out/parser_fuzz timeout-9 --mode hang --timeout 30
#
# Notes:
#  - For libFuzzer binaries: uses -minimize_crash=1 (and -timeout=) to produce minimized input.
#  - If you use AFL++, consider afl-tmin separately; this script is libFuzzer-centric.

set -eu

TARGET="${1:-}"
BIN="${2:-}"
INPUT="${3:-}"
if [ -z "$TARGET" ] || [ -z "$BIN" ] || [ -z "$INPUT" ]; then
  echo "usage: $(basename "$0") <target> <target_bin> <input_file> [--mode crash|hang] [--timeout N]" >&2
  exit 2
fi
shift 3

MODE="crash"
TIMEOUT="25"
DRY_RUN=0

while [ $# -gt 0 ]; do
  case "$1" in
    --mode)
      shift
      MODE="${1:-}"
      [ -n "$MODE" ] || { echo "error: --mode requires value" >&2; exit 2; }
      ;;
    --timeout)
      shift
      TIMEOUT="${1:-}"
      [ -n "$TIMEOUT" ] || { echo "error: --timeout requires value" >&2; exit 2; }
      ;;
    --dry-run)
      DRY_RUN=1
      ;;
    -h|--help)
      echo "usage: $(basename "$0") <target> <target_bin> <input_file> [--mode crash|hang] [--timeout N]" >&2
      exit 0
      ;;
    *)
      echo "error: unknown arg: $1" >&2
      exit 2
      ;;
  esac
  shift
done

ROOT_DIR="${ROOT_DIR:-}"
if [ -z "$ROOT_DIR" ]; then
  SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
  ROOT_DIR="$(CDPATH= cd -- "$SCRIPT_DIR/../.." && pwd)"
fi

say() { printf '%s\n' "$*"; }
die() { echo "error: $*" >&2; exit 1; }

if [ ! -x "$BIN" ]; then
  die "target_bin not executable: $BIN"
fi
if [ ! -f "$INPUT" ]; then
  die "input_file not found: $INPUT"
fi

FUZZ_DIR="$ROOT_DIR/fuzz"
OUT_DIR="$FUZZ_DIR/minimized/$TARGET"
mkdir_p() { [ "$DRY_RUN" -eq 1 ] && say "[dry-run] mkdir -p $1" || mkdir -p "$1"; }
cp_file() { [ "$DRY_RUN" -eq 1 ] && say "[dry-run] cp $1 $2" || cp -- "$1" "$2"; }

mkdir_p "$OUT_DIR"

# Output filenames
base="$(basename "$INPUT")"
stamp="$(date +%Y%m%d_%H%M%S 2>/dev/null || echo now)"
orig="$OUT_DIR/${base}.orig"
min="$OUT_DIR/${base}.min"
log="$OUT_DIR/${base}.${stamp}.log"

# Keep original alongside
if [ ! -f "$orig" ]; then
  cp_file "$INPUT" "$orig"
fi

say "[minimize] target=$TARGET mode=$MODE timeout=$TIMEOUT"
say "[minimize] bin=$BIN"
say "[minimize] in =$INPUT"
say "[minimize] out=$min"
say "[minimize] log=$log"

# Attempt libFuzzer minimization.
# libFuzzer expects to be run with a single input file and will write minimized
# output by rewriting the file in-place if using -exact_artifact_path.
# We use -exact_artifact_path to control output path deterministically.

LF_ARGS="-timeout=$TIMEOUT"
case "$MODE" in
  crash) LF_ARGS="$LF_ARGS -minimize_crash=1" ;;
  hang)  LF_ARGS="$LF_ARGS -minimize_crash=1" ;; # hang minimization still uses crash minimizer
  *) die "unknown mode: $MODE (expected crash|hang)" ;;
esac

# Always keep artifacts in our controlled file
LF_ARGS="$LF_ARGS -exact_artifact_path=$min"

run_cmd() {
  if [ "$DRY_RUN" -eq 1 ]; then
    say "[dry-run] $*"
    return 0
  fi
  # redirect both to log
  # shellcheck disable=SC2086
  "$@" >"$log" 2>&1 || return 1
  return 0
}

# First attempt: run as libFuzzer with minimization flags
if run_cmd "$BIN" $LF_ARGS "$INPUT"; then
  if [ -f "$min" ]; then
    say "[minimize] minimized artifact produced: $min"
    exit 0
  fi
fi

# Fallback: just copy the input as "min"
say "[minimize] libFuzzer minimization not available/failed; copying input as-is"
cp_file "$INPUT" "$min"
exit 0
