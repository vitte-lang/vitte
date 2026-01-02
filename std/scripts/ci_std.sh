#!/usr/bin/env sh
# /Users/vincent/Documents/Github/vitte/std/scripts/ci_std.sh
# -----------------------------------------------------------------------------
# std/scripts/ci_std.sh
# -----------------------------------------------------------------------------
# CI driver for Vitte stdlib.
#
# MAX goals:
# - Single CI entrypoint to build/check/test core std packages.
# - Works from repo root or from std/.
# - POSIX sh (no bashisms), strict mode.
# - Deterministic output and failure behavior.
# - Supports job splitting via CI_STD_SHARD/CI_STD_SHARDS.
# - Supports optional benches in CI.
#
# Environment variables:
#   CI_STD_MODE           "debug" (default) or "release"
#   CI_STD_JOBS           parallel jobs (best-effort)
#   CI_STD_FEATURES       comma-separated features to pass
#   CI_STD_NO_DEFAULT     1 to disable default features
#   CI_STD_CLEAN          1 to run muffin clean first
#   CI_STD_BENCH          1 to run benches (optional)
#   CI_STD_SHARDS         total shard count (default: 1)
#   CI_STD_SHARD          shard index (0-based, default: 0)
#
# Exit codes:
#   non-zero on first failing step.
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

# Simple hash to shard mapping: sum of bytes % shards.
# POSIX: use od for bytes.
shard_pick() {
  name="$1"
  shards="$2"

  # Compute sum of byte values
  sum=0
  # shellcheck disable=SC2039
  bytes=$(printf '%s' "$name" | od -An -tu1 | tr -s ' ')
  for b in $bytes; do
    sum=$((sum + b))
  done
  printf '%s' $((sum % shards))
}

usage() {
  cat <<'EOF'
std/scripts/ci_std.sh - CI driver for Vitte stdlib

Env:
  CI_STD_MODE        debug|release (default: debug)
  CI_STD_JOBS        parallel jobs
  CI_STD_FEATURES    comma-separated features
  CI_STD_NO_DEFAULT  1 disable default features
  CI_STD_CLEAN       1 run muffin clean
  CI_STD_BENCH       1 run benches
  CI_STD_SHARDS      total shards (default: 1)
  CI_STD_SHARD       shard index (0-based, default: 0)

Typical:
  CI_STD_MODE=release CI_STD_JOBS=8 ./std/scripts/ci_std.sh
EOF
}

# ----------------------------------------------------------------------------
# Config
# ----------------------------------------------------------------------------

MODE=${CI_STD_MODE:-debug}
JOBS=${CI_STD_JOBS:-}
FEATURES=${CI_STD_FEATURES:-}
NO_DEFAULT=${CI_STD_NO_DEFAULT:-0}
DO_CLEAN=${CI_STD_CLEAN:-0}
DO_BENCH=${CI_STD_BENCH:-0}
SHARDS=${CI_STD_SHARDS:-1}
SHARD=${CI_STD_SHARD:-0}

case "$MODE" in
  debug|release) ;;
  *) die "CI_STD_MODE must be debug or release (got: $MODE)" ;;
esac

case "$SHARDS" in
  ''|*[!0-9]*) die "CI_STD_SHARDS must be an integer" ;;
esac
case "$SHARD" in
  ''|*[!0-9]*) die "CI_STD_SHARD must be an integer" ;;
esac

[ "$SHARDS" -ge 1 ] || die "CI_STD_SHARDS must be >= 1"
[ "$SHARD" -lt "$SHARDS" ] || die "CI_STD_SHARD must be < CI_STD_SHARDS"

need_cmd muffin

# Auto-detect repo root
sd=$(script_dir)
CHDIR=""
if [ -n "$sd" ]; then
  if root=$(find_repo_root "$sd" 2>/dev/null); then
    CHDIR="$root"
  fi
fi
[ -n "$CHDIR" ] || CHDIR="$(pwd)"

cd -- "$CHDIR"

if [ -d "./std" ]; then
  STD_DIR="./std"
else
  if [ -d "./bench" ] && [ -f "./mod.muf" ]; then
    STD_DIR="."
  else
    die "could not locate std directory from: $CHDIR"
  fi
fi

# Ordered package list (keep core first)
ALL_PKGS="std-core std-runtime std-mem std-collections std-string std-io std-fs std-cli std-path std-net std-regex std-time"

# Muffin flags (best-effort)
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

if [ "$NO_DEFAULT" = "1" ]; then
  MUFFIN_FLAGS="$MUFFIN_FLAGS --no-default-features"
fi

say "ci: mode=$MODE jobs=${JOBS:-auto} shards=$SHARDS shard=$SHARD clean=$DO_CLEAN bench=$DO_BENCH dir=$CHDIR"

# ----------------------------------------------------------------------------
# Clean (optional)
# ----------------------------------------------------------------------------

if [ "$DO_CLEAN" = "1" ]; then
  say "clean: muffin clean"
  # shellcheck disable=SC2086
  muffin clean $MUFFIN_FLAGS
fi

# ----------------------------------------------------------------------------
# Build/check/test
# ----------------------------------------------------------------------------

run_pkg() {
  pkg="$1"

  pick=$(shard_pick "$pkg" "$SHARDS")
  if [ "$pick" -ne "$SHARD" ]; then
    say "skip[$SHARD/$SHARDS]: $pkg"
    return 0
  fi

  say "build: $pkg"
  # shellcheck disable=SC2086
  muffin build $pkg $MUFFIN_FLAGS

  say "check: $pkg"
  # shellcheck disable=SC2086
  muffin check $pkg $MUFFIN_FLAGS

  say "test: $pkg"
  # shellcheck disable=SC2086
  muffin test $pkg $MUFFIN_FLAGS
}

for pkg in $ALL_PKGS; do
  run_pkg "$pkg"
done

# ----------------------------------------------------------------------------
# Optional benches
# ----------------------------------------------------------------------------

if [ "$DO_BENCH" = "1" ]; then
  # Bench runs only on shard 0 by default
  if [ "$SHARD" -eq 0 ]; then
    say "bench: std-bench"
    # shellcheck disable=SC2086
    muffin run std-bench std-bench $MUFFIN_FLAGS -- --text
  else
    say "bench: skipped on shard $SHARD"
  fi
fi

say "ci done"
