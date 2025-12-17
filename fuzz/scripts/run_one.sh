#!/usr/bin/env sh
# run_one.sh
# Run a fuzz target binary against a single input (or stdin) with sane defaults.
#
# Usage:
#   ./fuzz/scripts/run_one.sh <target_bin> <input_file>
#   ./fuzz/scripts/run_one.sh <target_bin> -        # read input from stdin into a temp file
#
# Options:
#   --timeout N        per-run timeout (best-effort; uses timeout/gtimeout if available)
#   --repeat N         run the same input N times
#   --args "..."       extra args passed to target binary (split by shell)
#   --dry-run
#
# Notes:
#  - For libFuzzer binaries, extra args can include "-timeout=.." etc.
#  - For "stdin" mode, we materialize stdin into a temp file to keep behavior stable.

set -eu

BIN="${1:-}"
INPUT="${2:-}"
if [ -z "$BIN" ] || [ -z "$INPUT" ]; then
  echo "usage: $(basename "$0") <target_bin> <input_file|-> [--timeout N] [--repeat N] [--args \"...\"] [--dry-run]" >&2
  exit 2
fi
shift 2

TIMEOUT="25"
REPEAT="1"
EXTRA_ARGS=""
DRY_RUN=0

while [ $# -gt 0 ]; do
  case "$1" in
    --timeout) shift; TIMEOUT="${1:-}"; [ -n "$TIMEOUT" ] || exit 2 ;;
    --repeat)  shift; REPEAT="${1:-}";  [ -n "$REPEAT" ] || exit 2 ;;
    --args)    shift; EXTRA_ARGS="${1:-}"; [ -n "$EXTRA_ARGS" ] || EXTRA_ARGS="" ;;
    --dry-run) DRY_RUN=1 ;;
    -h|--help)
      echo "usage: $(basename "$0") <target_bin> <input_file|-> [--timeout N] [--repeat N] [--args \"...\"] [--dry-run]" >&2
      exit 0
      ;;
    *) echo "error: unknown arg: $1" >&2; exit 2 ;;
  esac
  shift
done

die() { echo "error: $*" >&2; exit 1; }
say() { printf '%s\n' "$*"; }

if [ ! -x "$BIN" ]; then
  die "target_bin not executable: $BIN"
fi

# Timeout wrapper if available
TIMEOUT_CMD=""
if command -v timeout >/dev/null 2>&1; then
  TIMEOUT_CMD="timeout $TIMEOUT"
elif command -v gtimeout >/dev/null 2>&1; then
  TIMEOUT_CMD="gtimeout $TIMEOUT"
fi

TMP_IN=""
cleanup() {
  if [ -n "$TMP_IN" ] && [ -f "$TMP_IN" ]; then
    rm -f -- "$TMP_IN" || true
  fi
}
trap cleanup EXIT INT TERM

if [ "$INPUT" = "-" ]; then
  # materialize stdin
  if command -v mktemp >/dev/null 2>&1; then
    TMP_IN="$(mktemp 2>/dev/null || mktemp -t vitte_fuzz_in)"
  else
    TMP_IN="./.fuzz_stdin_input.$$"
  fi
  if [ "$DRY_RUN" -eq 1 ]; then
    say "[dry-run] cat <stdin> > $TMP_IN"
  else
    cat >"$TMP_IN"
  fi
  INPUT="$TMP_IN"
else
  if [ ! -f "$INPUT" ]; then
    die "input_file not found: $INPUT"
  fi
fi

say "[run_one] bin=$BIN"
say "[run_one] in =$INPUT"
say "[run_one] timeout=$TIMEOUT repeat=$REPEAT"
say "[run_one] extra_args=$EXTRA_ARGS"

i=1
while [ "$i" -le "$REPEAT" ]; do
  if [ "$DRY_RUN" -eq 1 ]; then
    if [ -n "$TIMEOUT_CMD" ]; then
      say "[dry-run] $TIMEOUT_CMD $BIN $EXTRA_ARGS $INPUT"
    else
      say "[dry-run] $BIN $EXTRA_ARGS $INPUT"
    fi
    i=$((i + 1))
    continue
  fi

  if [ -n "$TIMEOUT_CMD" ]; then
    # shellcheck disable=SC2086
    $TIMEOUT_CMD "$BIN" $EXTRA_ARGS "$INPUT"
  else
    # shellcheck disable=SC2086
    "$BIN" $EXTRA_ARGS "$INPUT"
  fi

  i=$((i + 1))
done
