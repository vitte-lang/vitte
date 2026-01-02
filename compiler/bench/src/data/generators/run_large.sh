#!/usr/bin/env sh
# -----------------------------------------------------------------------------
# run_large.sh - Regenerate a large/stress dataset (POSIX sh)
# -----------------------------------------------------------------------------
# This runner is intended for local perf/stress refreshes and long CI jobs.
# It runs the generator driver with a "large" preset and validates the result.
#
# Usage examples:
#   ./run_large.sh
#   ./run_large.sh --preset ./presets/large.json --generated ../generated_large
#   ./run_large.sh --python python3
#   ./run_large.sh --extra "--seed 0 --count 500 --stress 6 --emit-md"
#   ./run_large.sh --regen-only
#   ./run_large.sh --validate-only
#   ./run_large.sh --dry-run
# -----------------------------------------------------------------------------

set -eu

usage() {
  cat <<'EOF'
run_large.sh

Options:
  --python PATH        Python interpreter (default: python3)
  --preset PATH        Preset JSON (default: ./presets/large.json)
  --generated PATH     Output directory (default: ../generated_large)

  --regen-only         Only regenerate fixtures (skip validation)
  --validate-only      Only validate dataset (skip regeneration)

  --extra STRING       Extra args passed to driver.py (split by whitespace)
  --dry-run            Print commands but do not execute

  -h, --help           Show this help
EOF
}

PY="python3"
PRESET="./presets/large.json"
GENERATED="../generated_large"

REGEN_ONLY=0
VALIDATE_ONLY=0
EXTRA=""
DRY_RUN=0

# Parse args
while [ "$#" -gt 0 ]; do
  case "$1" in
    --python)
      [ "$#" -ge 2 ] || { echo "error: --python requires a value" >&2; exit 2; }
      PY="$2"; shift 2 ;;
    --preset)
      [ "$#" -ge 2 ] || { echo "error: --preset requires a value" >&2; exit 2; }
      PRESET="$2"; shift 2 ;;
    --generated)
      [ "$#" -ge 2 ] || { echo "error: --generated requires a value" >&2; exit 2; }
      GENERATED="$2"; shift 2 ;;

    --regen-only)
      REGEN_ONLY=1; shift 1 ;;
    --validate-only)
      VALIDATE_ONLY=1; shift 1 ;;

    --extra)
      [ "$#" -ge 2 ] || { echo "error: --extra requires a value" >&2; exit 2; }
      EXTRA="$2"; shift 2 ;;

    --dry-run)
      DRY_RUN=1; shift 1 ;;

    -h|--help)
      usage; exit 0 ;;

    *)
      echo "error: unknown arg: $1" >&2
      usage
      exit 2
      ;;
  esac
done

if [ "$REGEN_ONLY" = "1" ] && [ "$VALIDATE_ONLY" = "1" ]; then
  echo "error: choose either --regen-only or --validate-only (not both)" >&2
  exit 2
fi

DO_REGEN=1
DO_VALIDATE=1
if [ "$REGEN_ONLY" = "1" ]; then
  DO_VALIDATE=0
fi
if [ "$VALIDATE_ONLY" = "1" ]; then
  DO_REGEN=0
fi

# Move to script dir for stable relative paths
HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
cd "$HERE"

run_cmd() {
  if [ "$DRY_RUN" = "1" ]; then
    printf '[dry-run]'
    for a in "$@"; do
      printf ' %s' "$a"
    done
    printf '\n'
    return 0
  fi
  "$@"
}

# Regenerate
if [ "$DO_REGEN" = "1" ]; then
  echo "=== Regenerate large/stress fixtures ==="
  # shellcheck disable=SC2086
  run_cmd "$PY" ./driver.py --preset "$PRESET" --generated "$GENERATED" $EXTRA
fi

# Validate
if [ "$DO_VALIDATE" = "1" ]; then
  echo "=== Validate dataset ==="
  run_cmd "$PY" ./validate_dataset.py --root "$GENERATED"
fi

echo "=== OK ==="
echo "generated: $GENERATED"
