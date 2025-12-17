#!/usr/bin/env sh
# reproduce.sh
# Reproduce a crash/hang with a fuzz target binary (libFuzzer/AFL/standalone runner).
#
# Features:
#  - runs a target binary against one input (or a directory of inputs)
#  - optional per-input timeout (best-effort, uses "timeout" if available)
#  - writes a reproduction log under fuzz/repro/<target>/
#
# Usage:
#   ./fuzz/scripts/reproduce.sh <target> <target_bin> <input_path> [--timeout N] [--repeat N] [--dry-run]
#
# Examples:
#   ./fuzz/scripts/reproduce.sh lexer ./fuzz/out/lexer_fuzz crash-123
#   ./fuzz/scripts/reproduce.sh parser ./fuzz/out/parser_fuzz ./fuzz/findings/parser/crashes --timeout 30
#   ./fuzz/scripts/reproduce.sh ir    ./fuzz/out/ir_fuzz    minimized.bin --repeat 100
#
# Notes:
#  - For libFuzzer binaries, you can pass extra flags via FUZZ_EXTRA_ARGS.
#  - For AFL-style binaries that take a file argument, this script still works.

set -eu

TARGET="${1:-}"
BIN="${2:-}"
INPUT_PATH="${3:-}"
if [ -z "$TARGET" ] || [ -z "$BIN" ] || [ -z "$INPUT_PATH" ]; then
  echo "usage: $(basename "$0") <target> <target_bin> <input_path> [--timeout N] [--repeat N] [--dry-run]" >&2
  exit 2
fi
shift 3

TIMEOUT="25"
REPEAT="1"
DRY_RUN=0

while [ $# -gt 0 ]; do
  case "$1" in
    --timeout)
      shift
      TIMEOUT="${1:-}"
      [ -n "$TIMEOUT" ] || { echo "error: --timeout requires value" >&2; exit 2; }
      ;;
    --repeat)
      shift
      REPEAT="${1:-}"
      [ -n "$REPEAT" ] || { echo "error: --repeat requires value" >&2; exit 2; }
      ;;
    --dry-run) DRY_RUN=1 ;;
    -h|--help)
      echo "usage: $(basename "$0") <target> <target_bin> <input_path> [--timeout N] [--repeat N] [--dry-run]" >&2
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
if [ ! -e "$INPUT_PATH" ]; then
  die "input_path not found: $INPUT_PATH"
fi

FUZZ_DIR="$ROOT_DIR/fuzz"
OUT_DIR="$FUZZ_DIR/repro/$TARGET"
mkdir -p "$OUT_DIR"

stamp="$(date +%Y%m%d_%H%M%S 2>/dev/null || echo now)"
log="$OUT_DIR/repro_${stamp}.log"

# Timeout wrapper if available
TIMEOUT_CMD=""
if command -v timeout >/dev/null 2>&1; then
  TIMEOUT_CMD="timeout $TIMEOUT"
elif command -v gtimeout >/dev/null 2>&1; then
  TIMEOUT_CMD="gtimeout $TIMEOUT"
fi

# Extra args for libFuzzer etc.
EXTRA="${FUZZ_EXTRA_ARGS:-}"

run_one() {
  f="$1"
  i=1
  while [ "$i" -le "$REPEAT" ]; do
    if [ "$DRY_RUN" -eq 1 ]; then
      say "[dry-run] run $i/$REPEAT: $BIN $EXTRA $f"
      i=$((i + 1))
      continue
    fi

    {
      echo "=== run $i/$REPEAT ==="
      echo "bin: $BIN"
      echo "input: $f"
      echo "timeout: $TIMEOUT"
      echo "extra: $EXTRA"
      echo "---"
    } >>"$log"

    if [ -n "$TIMEOUT_CMD" ]; then
      # shellcheck disable=SC2086
      $TIMEOUT_CMD "$BIN" $EXTRA "$f" >>"$log" 2>&1 || return 1
    else
      # shellcheck disable=SC2086
      "$BIN" $EXTRA "$f" >>"$log" 2>&1 || return 1
    fi

    i=$((i + 1))
  done
  return 0
}

say "[reproduce] target=$TARGET timeout=$TIMEOUT repeat=$REPEAT"
say "[reproduce] bin=$BIN"
say "[reproduce] input=$INPUT_PATH"
say "[reproduce] log=$log"

# If a directory, run all files inside (shallow)
if [ -d "$INPUT_PATH" ]; then
  found=0
  for f in "$INPUT_PATH"/*; do
    [ -f "$f" ] || continue
    found=1
    if ! run_one "$f"; then
      say "[reproduce] failed on: $f"
      exit 1
    fi
  done
  if [ "$found" -eq 0 ]; then
    die "directory has no files: $INPUT_PATH"
  fi
  say "[reproduce] OK (all inputs)"
  exit 0
fi

# Single file
if run_one "$INPUT_PATH"; then
  say "[reproduce] OK"
  exit 0
fi

say "[reproduce] FAIL (non-zero exit). See log: $log"
exit 1
