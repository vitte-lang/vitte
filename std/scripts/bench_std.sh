

#!/usr/bin/env sh
# /Users/vincent/Documents/Github/vitte/std/scripts/bench_std.sh
# -----------------------------------------------------------------------------
# std/scripts/bench_std.sh
# -----------------------------------------------------------------------------
# Benchmark runner for Vitte stdlib.
#
# MAX goals:
# - Provide a single entrypoint to run std/bench (and std/bench/micro suites).
# - Work out-of-the-box from repo root or from std/.
# - Offer common toggles: release/debug, json/text output, filter, iterations,
#   warmup, and output file.
# - Avoid bashisms: POSIX sh.
# - Be CI-friendly (non-interactive), with strict error handling.
#
# Assumptions:
# - `muffin` is on PATH.
# - std-bench package exists at std/bench.
# - Optional micro benches under std/bench/micro.
#
# Usage:
#   ./std/scripts/bench_std.sh [options] [-- <bench-args>]
#
# Options:
#   -C <dir>        Change to directory before running (default: auto-detect repo)
#   -p <pkg>        Package to run (default: std-bench)
#   -t <target>     Muffin target to run (default: std-bench)
#   -r              Release mode (opt_level=3) if supported
#   -d              Debug mode (default)
#   --json          Prefer JSON output (pass-through)
#   --text          Prefer text output (pass-through)
#   --filter <pat>  Filter benches by pattern (pass-through)
#   --iters <n>     Iterations per bench (pass-through)
#   --warmup <n>    Warmup iterations (pass-through)
#   -o <file>       Write output to file (tee)
#   -v              Verbose
#   -h              Help
#
# Any remaining args after `--` are forwarded to the bench binary.
# -----------------------------------------------------------------------------

set -eu

# ----------------------------------------------------------------------------
# Helpers
# ----------------------------------------------------------------------------

say() { printf '%s\n' "$*"; }

die() {
  printf 'error: %s\n' "$*" >&2
  exit 1
}

need_cmd() {
  command -v "$1" >/dev/null 2>&1 || die "missing required command: $1"
}

# Resolve directory of this script (portable best-effort)
script_dir() {
  # Works for common invocations; avoids readlink -f (non-portable on macOS)
  d=$(CDPATH= cd -- "$(dirname -- "$0")" 2>/dev/null && pwd) || d=""
  printf '%s' "$d"
}

# Find repo root by walking up looking for VERSION or muffin.muf
find_repo_root() {
  start="$1"
  cur="$start"
  while :; do
    if [ -f "$cur/VERSION" ] && [ -d "$cur/std" ]; then
      printf '%s' "$cur"
      return 0
    fi
    if [ -f "$cur/muffin.muf" ] || [ -f "$cur/muffin.lock" ]; then
      printf '%s' "$cur"
      return 0
    fi
    parent=$(dirname -- "$cur")
    [ "$parent" = "$cur" ] && break
    cur="$parent"
  done
  return 1
}

usage() {
  cat <<'EOF'
std/scripts/bench_std.sh - run Vitte stdlib benchmarks

Usage:
  bench_std.sh [options] [-- <bench-args>]

Options:
  -C <dir>        Change to directory before running (default: auto-detect repo)
  -p <pkg>        Package to run (default: std-bench)
  -t <target>     Muffin target to run (default: std-bench)
  -r              Release mode (opt_level=3) if supported
  -d              Debug mode (default)
  --json          Prefer JSON output (forwarded)
  --text          Prefer text output (forwarded)
  --filter <pat>  Filter benches by pattern (forwarded)
  --iters <n>     Iterations per bench (forwarded)
  --warmup <n>    Warmup iterations (forwarded)
  -o <file>       Write output to file (tee)
  -v              Verbose
  -h              Help

Examples:
  ./std/scripts/bench_std.sh --text
  ./std/scripts/bench_std.sh --json -o out.json
  ./std/scripts/bench_std.sh --filter sort --iters 50 --warmup 5
  ./std/scripts/bench_std.sh -r -- --baseline
EOF
}

# ----------------------------------------------------------------------------
# Defaults
# ----------------------------------------------------------------------------

CHDIR=""
PKG="std-bench"
TARGET="std-bench"
MODE="debug"
OUT_FILE=""
VERBOSE=0

# Forwarded args to bench runner
FWD=""

# ----------------------------------------------------------------------------
# Parse args
# ----------------------------------------------------------------------------

# We keep a small state machine to support -- (stop parsing)
stop=0

while [ $# -gt 0 ]; do
  if [ "$stop" -eq 1 ]; then
    # shellcheck disable=SC2089
    FWD="$FWD $(printf '%s' "$1")"
    shift
    continue
  fi

  case "$1" in
    -C)
      [ $# -ge 2 ] || die "-C expects a directory"
      CHDIR="$2"
      shift 2
      ;;
    -p)
      [ $# -ge 2 ] || die "-p expects a package"
      PKG="$2"
      shift 2
      ;;
    -t)
      [ $# -ge 2 ] || die "-t expects a target"
      TARGET="$2"
      shift 2
      ;;
    -r)
      MODE="release"
      shift
      ;;
    -d)
      MODE="debug"
      shift
      ;;
    --json)
      # shellcheck disable=SC2089
      FWD="$FWD --json"
      shift
      ;;
    --text)
      # shellcheck disable=SC2089
      FWD="$FWD --text"
      shift
      ;;
    --filter)
      [ $# -ge 2 ] || die "--filter expects a pattern"
      # shellcheck disable=SC2089
      FWD="$FWD --filter $(printf '%s' "$2")"
      shift 2
      ;;
    --iters)
      [ $# -ge 2 ] || die "--iters expects a number"
      # shellcheck disable=SC2089
      FWD="$FWD --iters $(printf '%s' "$2")"
      shift 2
      ;;
    --warmup)
      [ $# -ge 2 ] || die "--warmup expects a number"
      # shellcheck disable=SC2089
      FWD="$FWD --warmup $(printf '%s' "$2")"
      shift 2
      ;;
    -o)
      [ $# -ge 2 ] || die "-o expects a file"
      OUT_FILE="$2"
      shift 2
      ;;
    -v)
      VERBOSE=1
      shift
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    --)
      stop=1
      shift
      ;;
    *)
      # Unknown arg: forward
      # shellcheck disable=SC2089
      FWD="$FWD $(printf '%s' "$1")"
      shift
      ;;
  esac
done

# ----------------------------------------------------------------------------
# Setup
# ----------------------------------------------------------------------------

need_cmd muffin

# Auto-detect repo root if -C not supplied
if [ -z "$CHDIR" ]; then
  sd=$(script_dir)
  if [ -n "$sd" ]; then
    if root=$(find_repo_root "$sd" 2>/dev/null); then
      CHDIR="$root"
    fi
  fi
  if [ -z "$CHDIR" ]; then
    # fallback: current dir
    CHDIR="$(pwd)"
  fi
fi

cd -- "$CHDIR"

# Determine std path (repo root or std subdir)
if [ -d "./std" ]; then
  STD_DIR="./std"
else
  # Maybe we are already inside std
  if [ -f "./mod.muf" ] && [ -d "./bench" ]; then
    STD_DIR="."
  else
    die "could not locate std directory from: $CHDIR"
  fi
fi

# Prepare muffin args
MUFFIN_ARGS=""
if [ "$MODE" = "release" ]; then
  # Many build systems accept --release; if Muffin differs, adjust here.
  MUFFIN_ARGS="$MUFFIN_ARGS --release"
fi

if [ "$VERBOSE" -eq 1 ]; then
  set -x
fi

# ----------------------------------------------------------------------------
# Run
# ----------------------------------------------------------------------------

say "bench: pkg=$PKG target=$TARGET mode=$MODE dir=$CHDIR"

# We run from repo root to keep dependency paths consistent.
# Users may pass additional args to the bench binary via FWD.

if [ -n "$OUT_FILE" ]; then
  # Ensure parent dir exists
  odir=$(dirname -- "$OUT_FILE")
  if [ "$odir" != "." ] && [ ! -d "$odir" ]; then
    mkdir -p -- "$odir"
  fi

  # shellcheck disable=SC2086
  muffin run $PKG $TARGET $MUFFIN_ARGS -- $FWD 2>&1 | tee "$OUT_FILE"
else
  # shellcheck disable=SC2086
  muffin run $PKG $TARGET $MUFFIN_ARGS -- $FWD
fi
