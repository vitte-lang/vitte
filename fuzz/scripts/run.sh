#!/usr/bin/env sh
# run.sh
# Run a fuzz target in "campaign" mode (libFuzzer or AFL++ conventions).
#
# This script is intentionally generic:
#  - For libFuzzer binaries, it passes corpus dir(s) + -artifact_prefix + limits.
#  - For AFL++ binaries, it can run afl-fuzz if AFL_RUN=1.
#
# Usage (libFuzzer default):
#   ./fuzz/scripts/run.sh <target> <target_bin> [--corpus DIR] [--dict FILE] [--jobs N] [--timeout N]
#
# Usage (AFL++):
#   AFL_RUN=1 ./fuzz/scripts/run.sh <target> <target_bin> --corpus fuzz/corpora/<target>
#
# Options:
#   --corpus DIR         corpus directory (default: fuzz/corpora/<target>)
#   --out DIR            output/findings directory (default: fuzz/findings/<target>)
#   --dict FILE          dictionary file to pass (libFuzzer: -dict=..., AFL: -x ...)
#   --timeout N          per-input timeout seconds (libFuzzer: -timeout=N)
#   --rss-limit-mb N     RSS limit (libFuzzer: -rss_limit_mb=N)
#   --max-len N          max input size (libFuzzer: -max_len=N)
#   --runs N             number of runs (0 = infinite)
#   --jobs N             libFuzzer -jobs / -workers (default: 1)
#   --artifact-prefix D  where to write crash artifacts (default under out)
#   --extra "..."        extra args to pass to target binary (shell-split)
#   --dry-run
#
# Environment:
#   AFL_RUN=1            use afl-fuzz (requires afl-fuzz in PATH)
#   AFL_BIN=afl-fuzz     override afl-fuzz binary
#   FUZZ_EXTRA_ARGS      appended to --extra
#
# Notes:
#  - For libFuzzer parallel mode, we use -jobs=N -workers=N by default.
#  - For AFL you typically need an instrumented binary; this script does not build it.

set -eu

TARGET="${1:-}"
BIN="${2:-}"
if [ -z "$TARGET" ] || [ -z "$BIN" ]; then
  echo "usage: $(basename "$0") <target> <target_bin> [options...]" >&2
  exit 2
fi
shift 2

ROOT_DIR="${ROOT_DIR:-}"
if [ -z "$ROOT_DIR" ]; then
  SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
  ROOT_DIR="$(CDPATH= cd -- "$SCRIPT_DIR/../.." && pwd)"
fi

FUZZ_DIR="$ROOT_DIR/fuzz"

CORPUS="$FUZZ_DIR/corpora/$TARGET"
OUTDIR="$FUZZ_DIR/findings/$TARGET"
DICT=""
TIMEOUT="${FUZZ_TIMEOUT:-25}"
RSS_LIMIT_MB="${FUZZ_RSS_LIMIT_MB:-0}"
MAX_LEN="${FUZZ_MAX_LEN:-0}"
RUNS="${FUZZ_RUNS:-0}"
JOBS="1"
ART_PREFIX=""
EXTRA=""
DRY_RUN=0

while [ $# -gt 0 ]; do
  case "$1" in
    --corpus) shift; CORPUS="${1:-}"; [ -n "$CORPUS" ] || exit 2 ;;
    --out) shift; OUTDIR="${1:-}"; [ -n "$OUTDIR" ] || exit 2 ;;
    --dict) shift; DICT="${1:-}"; [ -n "$DICT" ] || exit 2 ;;
    --timeout) shift; TIMEOUT="${1:-}"; [ -n "$TIMEOUT" ] || exit 2 ;;
    --rss-limit-mb) shift; RSS_LIMIT_MB="${1:-}"; [ -n "$RSS_LIMIT_MB" ] || exit 2 ;;
    --max-len) shift; MAX_LEN="${1:-}"; [ -n "$MAX_LEN" ] || exit 2 ;;
    --runs) shift; RUNS="${1:-}"; [ -n "$RUNS" ] || exit 2 ;;
    --jobs) shift; JOBS="${1:-}"; [ -n "$JOBS" ] || exit 2 ;;
    --artifact-prefix) shift; ART_PREFIX="${1:-}"; [ -n "$ART_PREFIX" ] || exit 2 ;;
    --extra) shift; EXTRA="${1:-}"; [ -n "$EXTRA" ] || EXTRA="" ;;
    --dry-run) DRY_RUN=1 ;;
    -h|--help)
      echo "usage: $(basename "$0") <target> <target_bin> [--corpus DIR] [--dict FILE] [--jobs N] [--timeout N] [--dry-run]" >&2
      exit 0
      ;;
    *) echo "error: unknown arg: $1" >&2; exit 2 ;;
  esac
  shift
done

say() { printf '%s\n' "$*"; }
die() { echo "error: $*" >&2; exit 1; }

if [ ! -x "$BIN" ]; then
  die "target_bin not executable: $BIN"
fi

mkdir -p "$CORPUS" "$OUTDIR"

if [ -z "$ART_PREFIX" ]; then
  ART_PREFIX="$OUTDIR/artifacts/"
fi
mkdir -p "$ART_PREFIX"

# Merge extra args from env
if [ -n "${FUZZ_EXTRA_ARGS:-}" ]; then
  if [ -n "$EXTRA" ]; then
    EXTRA="$EXTRA ${FUZZ_EXTRA_ARGS}"
  else
    EXTRA="${FUZZ_EXTRA_ARGS}"
  fi
fi

AFL_RUN="${AFL_RUN:-0}"
AFL_BIN="${AFL_BIN:-afl-fuzz}"

if [ "$AFL_RUN" = "1" ]; then
  # AFL++ mode
  command -v "$AFL_BIN" >/dev/null 2>&1 || die "AFL_RUN=1 but afl-fuzz not found: $AFL_BIN"
  [ -d "$CORPUS" ] || die "corpus dir missing: $CORPUS"

  AFL_ARGS="-i $CORPUS -o $OUTDIR"
  if [ -n "$DICT" ]; then
    AFL_ARGS="$AFL_ARGS -x $DICT"
  fi

  say "[run] mode=AFL target=$TARGET"
  say "[run] bin=$BIN"
  say "[run] corpus=$CORPUS"
  say "[run] out=$OUTDIR"
  say "[run] dict=$DICT"
  say "[run] extra=$EXTRA"

  if [ "$DRY_RUN" -eq 1 ]; then
    say "[dry-run] $AFL_BIN $AFL_ARGS -- $BIN $EXTRA @@"
    exit 0
  fi

  # shellcheck disable=SC2086
  exec "$AFL_BIN" $AFL_ARGS -- "$BIN" $EXTRA @@
fi

# libFuzzer mode (default)
LF_ARGS=""
LF_ARGS="$LF_ARGS -artifact_prefix=$ART_PREFIX"
LF_ARGS="$LF_ARGS -timeout=$TIMEOUT"
LF_ARGS="$LF_ARGS -runs=$RUNS"

# parallel
if [ "$JOBS" -gt 1 ] 2>/dev/null; then
  LF_ARGS="$LF_ARGS -jobs=$JOBS -workers=$JOBS"
fi

if [ "$RSS_LIMIT_MB" != "0" ]; then
  LF_ARGS="$LF_ARGS -rss_limit_mb=$RSS_LIMIT_MB"
fi
if [ "$MAX_LEN" != "0" ]; then
  LF_ARGS="$LF_ARGS -max_len=$MAX_LEN"
fi
if [ -n "$DICT" ]; then
  LF_ARGS="$LF_ARGS -dict=$DICT"
fi

say "[run] mode=libFuzzer target=$TARGET"
say "[run] bin=$BIN"
say "[run] corpus=$CORPUS"
say "[run] out=$OUTDIR"
say "[run] artifact_prefix=$ART_PREFIX"
say "[run] dict=$DICT"
say "[run] timeout=$TIMEOUT runs=$RUNS jobs=$JOBS"
say "[run] extra=$EXTRA"

if [ "$DRY_RUN" -eq 1 ]; then
  say "[dry-run] $BIN $LF_ARGS $EXTRA $CORPUS"
  exit 0
fi

# shellcheck disable=SC2086
exec "$BIN" $LF_ARGS $EXTRA "$CORPUS"
