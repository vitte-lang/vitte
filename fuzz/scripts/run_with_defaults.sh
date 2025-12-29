#!/usr/bin/env sh
# run_with_defaults.sh - wrapper around fuzz/scripts/run.sh with per-target defaults.
#
# Usage:
#   ./fuzz/scripts/run_with_defaults.sh <target> <target_bin> [extra run.sh args...]
#
# Examples:
#   ./fuzz/scripts/run_with_defaults.sh fuzz_parser ./build/fuzz_parser_fuzz
#   ./fuzz/scripts/run_with_defaults.sh fuzz_lexer  ./build/fuzz_lexer_fuzz --jobs 4

set -eu

TARGET="${1:-}"
BIN="${2:-}"
if [ -z "$TARGET" ] || [ -z "$BIN" ]; then
  echo "usage: $(basename "$0") <target> <target_bin> [run.sh options...]" >&2
  exit 2
fi
shift 2

DICT="$(python3 ./fuzz/scripts/target_map.py dict "$TARGET" 2>/dev/null || python ./fuzz/scripts/target_map.py dict "$TARGET")"

if [ -n "$DICT" ] && [ -f "$DICT" ]; then
  exec ./fuzz/scripts/run.sh "$TARGET" "$BIN" --dict "$DICT" "$@"
fi
exec ./fuzz/scripts/run.sh "$TARGET" "$BIN" "$@"
