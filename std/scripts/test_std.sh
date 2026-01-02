

#!/usr/bin/env sh
# /Users/vincent/Documents/Github/vitte/std/scripts/test_std.sh
# -----------------------------------------------------------------------------
# std/scripts/test_std.sh
# -----------------------------------------------------------------------------
# Test runner for Vitte stdlib.
#
# MAX goals:
# - One entrypoint to run std package tests (subset or default set).
# - Works from repo root or from std/.
# - POSIX sh (no bashisms), strict mode.
# - CI-friendly.
# - Supports: release/debug, clean, features, no-default-features, jobs.
#
# Usage:
#   ./std/scripts/test_std.sh [options] [packages...]
#
# Options:
#   -C <dir>            cd before running (auto-detect repo root by default)
#   -r                  release mode
#   -d                  debug mode (default)
#   --clean             run muffin clean first
#   --features <list>   comma-separated features (forwarded)
#   --no-default        disable default features (forwarded)
#   -j <n>              jobs (forwarded if supported)
#   -v                  verbose
#   -h                  help
# -----------------------------------------------------------------------------

set -eu

say() { printf '%s\n' "$*"; }

die() {
  printf 'error: %s\n' "$*" >&2
  exit 1
}

need_cmd() {
  command -v "$1" >/dev/null 2>&1 || die "missing required command: $1"
}

script_dir() {
  d=$(CDPATH= cd -- "$(dirname -- "$0")" 2>/dev/null && pwd) || d=""
  printf '%s' "$d"
}

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
std/scripts/test_std.sh - run Vitte stdlib tests

Usage:
  test_std.sh [options] [packages...]

Options:
  -C <dir>            Change directory before running
  -r                  Release mode
  -d                  Debug mode (default)
  --clean             Clean artifacts first
  --features <list>   Comma-separated features (forwarded)
  --no-default        Disable default features (forwarded)
  -j <n>              Parallel jobs (forwarded if supported)
  -v                  Verbose
  -h                  Help

If no packages are given, tests a default set.
EOF
}

# ----------------------------------------------------------------------------
# Defaults
# ----------------------------------------------------------------------------

CHDIR=""
MODE="debug"
DO_CLEAN=0
FEATURES=""
NO_DEFAULT=0
JOBS=""
VERBOSE=0
PKGS=""

# ----------------------------------------------------------------------------
# Parse args
# ----------------------------------------------------------------------------

while [ $# -gt 0 ]; do
  case "$1" in
    -C)
      [ $# -ge 2 ] || die "-C expects a directory"
      CHDIR="$2"
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
    --clean)
      DO_CLEAN=1
      shift
      ;;
    --features)
      [ $# -ge 2 ] || die "--features expects a list"
      FEATURES="$2"
      shift 2
      ;;
    --no-default)
      NO_DEFAULT=1
      shift
      ;;
    -j)
      [ $# -ge 2 ] || die "-j expects a number"
      JOBS="$2"
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
      shift
      break
      ;;
    -*)
      die "unknown option: $1"
      ;;
    *)
      if [ -z "$PKGS" ]; then
        PKGS="$1"
      else
        PKGS="$PKGS $1"
      fi
      shift
      ;;
  esac
done

# Remaining args after -- are treated as packages too
while [ $# -gt 0 ]; do
  if [ -z "$PKGS" ]; then
    PKGS="$1"
  else
    PKGS="$PKGS $1"
  fi
  shift
done

need_cmd muffin

# Auto-detect repo root
if [ -z "$CHDIR" ]; then
  sd=$(script_dir)
  if [ -n "$sd" ] && root=$(find_repo_root "$sd" 2>/dev/null); then
    CHDIR="$root"
  else
    CHDIR="$(pwd)"
  fi
fi

cd -- "$CHDIR"

if [ "$VERBOSE" -eq 1 ]; then
  set -x
fi

# Locate std directory (root or already in std)
if [ -d "./std" ]; then
  STD_DIR="./std"
else
  if [ -d "./scripts" ] && [ -f "./mod.muf" ]; then
    STD_DIR="."
  else
    die "could not locate std directory from: $CHDIR"
  fi
fi

# Default package set (ordered)
DEFAULT_PKGS="std-core std-runtime std-mem std-collections std-string std-io std-fs std-cli"

if [ -z "$PKGS" ]; then
  PKGS="$DEFAULT_PKGS"
fi

# Muffin flags (best-effort)
FLAGS=""
if [ "$MODE" = "release" ]; then
  FLAGS="$FLAGS --release"
fi

if [ -n "$JOBS" ]; then
  FLAGS="$FLAGS -j $JOBS"
fi

if [ -n "$FEATURES" ]; then
  FLAGS="$FLAGS --features $FEATURES"
fi

if [ "$NO_DEFAULT" -eq 1 ]; then
  FLAGS="$FLAGS --no-default-features"
fi

say "test: mode=$MODE clean=$DO_CLEAN dir=$CHDIR"

if [ "$DO_CLEAN" -eq 1 ]; then
  say "clean: muffin clean"
  # shellcheck disable=SC2086
  muffin clean $FLAGS
fi

for pkg in $PKGS; do
  say "test: $pkg"
  # shellcheck disable=SC2086
  muffin test $pkg $FLAGS
done

say "done"