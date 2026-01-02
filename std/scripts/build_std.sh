#!/usr/bin/env sh
# /Users/vincent/Documents/Github/vitte/std/scripts/build_std.sh
# -----------------------------------------------------------------------------
# std/scripts/build_std.sh
# -----------------------------------------------------------------------------
# Build helper for Vitte stdlib packages.
#
# MAX goals:
# - One entrypoint to build all std packages or a subset.
# - Works from repo root or from std/.
# - POSIX sh (no bashisms).
# - CI-friendly, deterministic, strict mode.
# - Supports: release/debug, clean, check/test, feature toggles, and parallelism
#   (best-effort, depending on Muffin).
#
# Assumptions:
# - `muffin` is on PATH.
# - Workspace manifest exists (repo root: muffin.muf) or std/ has member manifests.
#
# Usage:
#   ./std/scripts/build_std.sh [options] [packages...]
#
# If no packages are provided, builds a default std set.
#
# Options:
#   -C <dir>            Change to directory before running (default: auto-detect)
#   -r                  Release mode
#   -d                  Debug mode (default)
#   --clean             Clean build artifacts (muffin clean)
#   --check             Run muffin check after build
#   --test              Run muffin test after build
#   --features <list>   Comma-separated feature list (passed through)
#   --no-default        Disable default features (passed through)
#   -j <n>              Parallel jobs (passed through if supported)
#   -v                  Verbose
#   -h                  Help
#
# Examples:
#   ./std/scripts/build_std.sh -r
#   ./std/scripts/build_std.sh std-core std-runtime std-mem
#   ./std/scripts/build_std.sh --clean -r --check
#   ./std/scripts/build_std.sh -j 8 --features alloc,mem
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
std/scripts/build_std.sh - build Vitte stdlib

Usage:
  build_std.sh [options] [packages...]

Options:
  -C <dir>            Change directory before running
  -r                  Release mode
  -d                  Debug mode (default)
  --clean             Clean artifacts (muffin clean)
  --check             Run muffin check after build
  --test              Run muffin test after build
  --features <list>   Comma-separated features (forwarded)
  --no-default        Disable default features (forwarded)
  -j <n>              Parallel jobs (forwarded if supported)
  -v                  Verbose
  -h                  Help

If no packages are given, builds a default set.
EOF
}

# ----------------------------------------------------------------------------
# Defaults
# ----------------------------------------------------------------------------

CHDIR=""
MODE="debug"
DO_CLEAN=0
DO_CHECK=0
DO_TEST=0
FEATURES=""
NO_DEFAULT=0
JOBS=""
VERBOSE=0

# ----------------------------------------------------------------------------
# Parse args
# ----------------------------------------------------------------------------

PKGS=""

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
    --check)
      DO_CHECK=1
      shift
      ;;
    --test)
      DO_TEST=1
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
      # package name
      if [ -z "$PKGS" ]; then
        PKGS="$1"
      else
        PKGS="$PKGS $1"
      fi
      shift
      ;;
  esac
done

# Any remaining args after -- are treated as packages too
while [ $# -gt 0 ]; do
  if [ -z "$PKGS" ]; then
    PKGS="$1"
  else
    PKGS="$PKGS $1"
  fi
  shift
done

# ----------------------------------------------------------------------------
# Setup
# ----------------------------------------------------------------------------

need_cmd muffin

if [ -z "$CHDIR" ]; then
  sd=$(script_dir)
  if [ -n "$sd" ]; then
    if root=$(find_repo_root "$sd" 2>/dev/null); then
      CHDIR="$root"
    fi
  fi
  [ -n "$CHDIR" ] || CHDIR="$(pwd)"
fi

cd -- "$CHDIR"

# If in repo root, std lives in ./std
if [ -d "./std" ]; then
  STD_DIR="./std"
else
  # maybe already in std
  if [ -d "./bench" ] && [ -f "./mod.muf" ]; then
    STD_DIR="."
  else
    die "could not locate std directory from: $CHDIR"
  fi
fi

if [ "$VERBOSE" -eq 1 ]; then
  set -x
fi

# Default package set (ordered)
DEFAULT_PKGS="std-core std-runtime std-mem std-collections std-string std-io std-fs std-cli"

if [ -z "$PKGS" ]; then
  PKGS="$DEFAULT_PKGS"
fi

# Build muffin flags (best-effort)
MUFFIN_FLAGS=""
if [ "$MODE" = "release" ]; then
  MUFFIN_FLAGS="$MUFFIN_FLAGS --release"
fi

if [ -n "$JOBS" ]; then
  MUFFIN_FLAGS="$MUFFIN_FLAGS -j $JOBS"
fi

if [ -n "$FEATURES" ]; then
  MUFFIN_FLAGS="$MUFFIN_FLAGS --features $FEATURES"
fi

if [ "$NO_DEFAULT" -eq 1 ]; then
  MUFFIN_FLAGS="$MUFFIN_FLAGS --no-default-features"
fi

say "build: mode=$MODE clean=$DO_CLEAN check=$DO_CHECK test=$DO_TEST dir=$CHDIR"

# ----------------------------------------------------------------------------
# Clean (optional)
# ----------------------------------------------------------------------------

if [ "$DO_CLEAN" -eq 1 ]; then
  say "clean: muffin clean"
  # shellcheck disable=SC2086
  muffin clean $MUFFIN_FLAGS
fi

# ----------------------------------------------------------------------------
# Build all requested packages
# ----------------------------------------------------------------------------

for pkg in $PKGS; do
  say "build: $pkg"
  # shellcheck disable=SC2086
  muffin build $pkg $MUFFIN_FLAGS

done

# ----------------------------------------------------------------------------
# Post steps
# ----------------------------------------------------------------------------

if [ "$DO_CHECK" -eq 1 ]; then
  for pkg in $PKGS; do
    say "check: $pkg"
    # shellcheck disable=SC2086
    muffin check $pkg $MUFFIN_FLAGS
  done
fi

if [ "$DO_TEST" -eq 1 ]; then
  for pkg in $PKGS; do
    say "test: $pkg"
    # shellcheck disable=SC2086
    muffin test $pkg $MUFFIN_FLAGS
  done
fi

say "done"
