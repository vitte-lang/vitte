#!/usr/bin/env sh
# -----------------------------------------------------------------------------
# run_all.sh - convenience runner for Vitte bench data generators
# -----------------------------------------------------------------------------
# This is a thin wrapper around:
#   - Makefile targets (preferred)
#   - refresh_all.py orchestrator
#
# Usage:
#   ./run_all.sh --out ../generated --seed 0
#   ./run_all.sh --out ../generated --seed 0 --count 50 --stress 2 --emit-md
#   ./run_all.sh --out ../generated --seed 0 --only tokens,trivia
#
# Exit codes:
#   0 success
#   2 usage error
#   3 internal error
#   4 generator failure
# -----------------------------------------------------------------------------

set -eu

usage() {
  cat <<'EOF'
run_all.sh

Options:
  --out PATH           Output root directory (default: ../generated)
  --seed VALUE         Deterministic seed (default: 0)
  --count N            Random case count (default: 0)
  --stress N           Stress level (default: 0)
  --emit-md            Emit INDEX.md where supported

  --only CSV           Comma-separated generator names to run
  --skip CSV           Comma-separated generator names to skip

  --python PATH        Python interpreter (default: python3)
  --manifest PATH      Manifest path (default: manifest.json)

  --stats-inputs PATHS...     Pass-through to gen_stats.py
  --stats-include-ext CSV     Pass-through to gen_stats.py
  --stats-max-files N         Pass-through to gen_stats.py
  --stats-top N               Pass-through to gen_stats.py

  --make              Prefer Makefile (make all ...) instead of refresh_all.py
  --dry-run           Print commands only
  -h, --help          Show this help

Examples:
  ./run_all.sh --out ../generated --seed 0
  ./run_all.sh --out ../generated --seed 0 --count 50 --stress 2 --emit-md
  ./run_all.sh --out ../generated --seed 0 --only tokens,trivia
EOF
}

OUT="../generated"
SEED="0"
COUNT="0"
STRESS="0"
EMIT_MD="0"
ONLY=""
SKIP=""
PY="python3"
MANIFEST="manifest.json"

STATS_INPUTS=""
STATS_INCLUDE_EXT=""
STATS_MAX_FILES="0"
STATS_TOP="50"

PREFER_MAKE="0"
DRY_RUN="0"

# Parse args
while [ "$#" -gt 0 ]; do
  case "$1" in
    --out)
      [ "$#" -ge 2 ] || { echo "error: --out requires a value" >&2; exit 2; }
      OUT="$2"; shift 2 ;;
    --seed)
      [ "$#" -ge 2 ] || { echo "error: --seed requires a value" >&2; exit 2; }
      SEED="$2"; shift 2 ;;
    --count)
      [ "$#" -ge 2 ] || { echo "error: --count requires a value" >&2; exit 2; }
      COUNT="$2"; shift 2 ;;
    --stress)
      [ "$#" -ge 2 ] || { echo "error: --stress requires a value" >&2; exit 2; }
      STRESS="$2"; shift 2 ;;
    --emit-md)
      EMIT_MD="1"; shift 1 ;;

    --only)
      [ "$#" -ge 2 ] || { echo "error: --only requires a value" >&2; exit 2; }
      ONLY="$2"; shift 2 ;;
    --skip)
      [ "$#" -ge 2 ] || { echo "error: --skip requires a value" >&2; exit 2; }
      SKIP="$2"; shift 2 ;;

    --python)
      [ "$#" -ge 2 ] || { echo "error: --python requires a value" >&2; exit 2; }
      PY="$2"; shift 2 ;;
    --manifest)
      [ "$#" -ge 2 ] || { echo "error: --manifest requires a value" >&2; exit 2; }
      MANIFEST="$2"; shift 2 ;;

    --stats-inputs)
      shift 1
      # capture all following non-flag args
      while [ "$#" -gt 0 ] && [ "${1#--}" = "$1" ]; do
        if [ -z "$STATS_INPUTS" ]; then
          STATS_INPUTS="$1"
        else
          STATS_INPUTS="$STATS_INPUTS $1"
        fi
        shift 1
      done
      ;;
    --stats-include-ext)
      [ "$#" -ge 2 ] || { echo "error: --stats-include-ext requires a value" >&2; exit 2; }
      STATS_INCLUDE_EXT="$2"; shift 2 ;;
    --stats-max-files)
      [ "$#" -ge 2 ] || { echo "error: --stats-max-files requires a value" >&2; exit 2; }
      STATS_MAX_FILES="$2"; shift 2 ;;
    --stats-top)
      [ "$#" -ge 2 ] || { echo "error: --stats-top requires a value" >&2; exit 2; }
      STATS_TOP="$2"; shift 2 ;;

    --make)
      PREFER_MAKE="1"; shift 1 ;;

    --dry-run)
      DRY_RUN="1"; shift 1 ;;

    -h|--help)
      usage; exit 0 ;;

    *)
      echo "error: unknown arg: $1" >&2
      usage
      exit 2
      ;;
  esac
done

# Move to script dir
HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
cd "$HERE"

# Helper: print command if dry
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

if [ "$PREFER_MAKE" = "1" ]; then
  # Use Makefile; map args to Make vars.
  # stats inputs are not mapped here (use refresh_all.py for richer stats options).
  MAKE_ARGS="OUT=$OUT SEED=$SEED COUNT=$COUNT STRESS=$STRESS EMIT_MD=$EMIT_MD"
  if [ "$DRY_RUN" = "1" ]; then
    echo "[dry-run] make all $MAKE_ARGS"
    exit 0
  fi
  # shellcheck disable=SC2086
  exec make all $MAKE_ARGS
fi

# Otherwise call refresh_all.py
ARGS="--out $OUT --seed $SEED --count $COUNT --stress $STRESS"
if [ "$EMIT_MD" = "1" ]; then
  ARGS="$ARGS --emit-md"
fi
if [ -n "$ONLY" ]; then
  ARGS="$ARGS --only $ONLY"
fi
if [ -n "$SKIP" ]; then
  ARGS="$ARGS --skip $SKIP"
fi
if [ -n "$MANIFEST" ]; then
  ARGS="$ARGS --manifest $MANIFEST"
fi
if [ -n "$PY" ]; then
  ARGS="$ARGS --python $PY"
fi

# stats pass-through
if [ -n "$STATS_INPUTS" ]; then
  # shellcheck disable=SC2086
  ARGS="$ARGS --stats-inputs $STATS_INPUTS"
fi
if [ -n "$STATS_INCLUDE_EXT" ]; then
  ARGS="$ARGS --stats-include-ext $STATS_INCLUDE_EXT"
fi
ARGS="$ARGS --stats-max-files $STATS_MAX_FILES --stats-top $STATS_TOP"

# shellcheck disable=SC2086
run_cmd "$PY" ./refresh_all.py $ARGS
