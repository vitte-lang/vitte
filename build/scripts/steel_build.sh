#!/usr/bin/env sh
set -eu
./steel plan
# TODO: execute build

#!/usr/bin/env sh
#
# steel_build.sh
#
# Workspace build entrypoint for the Vitte toolchain (Steel).
#
# Goals:
#   - Provide a stable, portable CLI wrapper for building a Muffin workspace.
#   - Prefer the repo-local binaries (target/bin/steel, target/bin/steelc).
#   - Allow hermetic toolchain selection (zig cc) or system compiler.
#   - Keep POSIX sh compatibility (BSD/Solaris/Linux/macOS).
#
# This script is intentionally a thin wrapper around the `steel` CLI.
# The heavy lifting (planning, dependency graph, compilation, caching) lives in:
#   src/cli/steel_cmd_build.c  (+ compiler/runtime components)
#
# Usage examples:
#   ./build/scripts/steel_build.sh
#   ./build/scripts/steel_build.sh --release
#   ./build/scripts/steel_build.sh --target darwin_arm64
#   ./build/scripts/steel_build.sh --clean
#   ./build/scripts/steel_build.sh --plan
#   ./build/scripts/steel_build.sh --cc clang
#   ./build/scripts/steel_build.sh --zig ./toolchain/zig/bin/zig
#   ./build/scripts/steel_build.sh --pkg std
#

set -eu

# ------------------------------- helpers ---------------------------------

say() { printf "%s\n" "$*"; }
warn() { printf "steel_build: %s\n" "$*" >&2; }
die() { printf "steel_build: %s\n" "$*" >&2; exit 2; }

abspath() {
  _p="$1"
  if [ -z "$_p" ]; then return 1; fi
  if [ -d "$_p" ]; then
    (cd "$_p" 2>/dev/null && pwd)
  else
    _d=$(dirname "$_p")
    _b=$(basename "$_p")
    (cd "$_d" 2>/dev/null && printf "%s/%s\n" "$(pwd)" "$_b")
  fi
}

is_cmd() { command -v "$1" >/dev/null 2>&1; }

mkdir_p() {
  _d="$1"
  [ -n "$_d" ] || return 0
  [ -d "$_d" ] || mkdir -p "$_d"
}

rm_rf() {
  _p="$1"
  [ -n "$_p" ] || return 0
  [ ! -e "$_p" ] || rm -rf "$_p"
}

# best-effort check if `steel` supports a subcommand (without parsing locale)
steel_supports() {
  _steel="$1"
  _sub="$2"
  # Try: `steel <sub> --help`
  if "$_steel" "$_sub" --help >/dev/null 2>&1; then
    return 0
  fi
  # Try: `steel help <sub>`
  if "$_steel" help "$_sub" >/dev/null 2>&1; then
    return 0
  fi
  return 1
}

usage() {
  cat <<EOF
Usage: steel_build.sh [options]

Options:
  --release              build in release mode
  --debug                build in debug mode (default)
  --target <name>        target profile or triple (e.g. darwin_arm64)
  --pkg <name>           build a specific package (default: workspace default)
  --clean                remove build outputs under target/ then build
  --plan                 only compute and write the build plan (no compile)
  --no-plan              skip plan step (if steel build already plans)
  -j, --jobs <n>         parallel jobs (passed through)
  --cc <cc>              override C compiler for backend compilation
  --zig <zig>            use zig as toolchain (passes through as: <zig> cc)
  --config <path>        explicit workspace muffin.muf path
  --verbose              verbose logging
  --quiet                minimal output
  -h, --help             show help

Environment:
  STEEL                  override steel binary path
  STEELC                 override steelc binary path
  STEEL_TARGET           default --target
  STEEL_MODE             default mode (debug|release)
  STEEL_JOBS             default jobs
EOF
}

# ------------------------------- config ----------------------------------

ROOT_DIR=$(abspath "$(dirname "$0")/../..") || exit 1

MODE=${STEEL_MODE:-debug}
TARGET_NAME=${STEEL_TARGET:-}
PKG=""
CLEAN=0
PLAN_ONLY=0
DO_PLAN=1
JOBS=${STEEL_JOBS:-}
CC_OVERRIDE=""
ZIG_OVERRIDE=""
CONFIG_PATH=""
VERBOSE=0
QUIET=0

# Outputs
TARGET_DIR="$ROOT_DIR/target"
PLAN_DIR="$TARGET_DIR/plan"

# ----------------------------- parse args --------------------------------

while [ $# -gt 0 ]; do
  case "$1" in
    --release) MODE=release; shift ;;
    --debug) MODE=debug; shift ;;
    --target) shift; [ $# -gt 0 ] || { usage >&2; exit 1; }; TARGET_NAME="$1"; shift ;;
    --pkg) shift; [ $# -gt 0 ] || { usage >&2; exit 1; }; PKG="$1"; shift ;;
    --clean) CLEAN=1; shift ;;
    --plan) PLAN_ONLY=1; shift ;;
    --no-plan) DO_PLAN=0; shift ;;
    -j|--jobs) shift; [ $# -gt 0 ] || { usage >&2; exit 1; }; JOBS="$1"; shift ;;
    --cc) shift; [ $# -gt 0 ] || { usage >&2; exit 1; }; CC_OVERRIDE="$1"; shift ;;
    --zig) shift; [ $# -gt 0 ] || { usage >&2; exit 1; }; ZIG_OVERRIDE="$1"; shift ;;
    --config) shift; [ $# -gt 0 ] || { usage >&2; exit 1; }; CONFIG_PATH="$1"; shift ;;
    --verbose) VERBOSE=1; shift ;;
    --quiet) QUIET=1; shift ;;
    -h|--help) usage; exit 0 ;;
    *) usage >&2; exit 1 ;;
  esac
done

# ----------------------------- preflight ---------------------------------

if [ -z "$CONFIG_PATH" ]; then
  CONFIG_PATH="$ROOT_DIR/muffin.muf"
fi

if [ ! -f "$CONFIG_PATH" ]; then
  die "missing workspace manifest: $CONFIG_PATH"
fi

if [ "$CLEAN" -eq 1 ]; then
  if [ "$QUIET" -ne 1 ]; then
    say "steel_build: cleaning $TARGET_DIR"
  fi
  rm_rf "$TARGET_DIR"
fi

mkdir_p "$PLAN_DIR"

# Resolve `steel` and `steelc` binaries.
# Prefer repo-local outputs.
STEEL=${STEEL:-}
STEELC=${STEELC:-}

if [ -z "$STEEL" ]; then
  if [ -x "$ROOT_DIR/target/bin/steel" ]; then
    STEEL="$ROOT_DIR/target/bin/steel"
  elif is_cmd steel; then
    STEEL=steel
  else
    STEEL=""
  fi
fi

if [ -z "$STEELC" ]; then
  if [ -x "$ROOT_DIR/target/bin/steelc" ]; then
    STEELC="$ROOT_DIR/target/bin/steelc"
  elif is_cmd steelc; then
    STEELC=steelc
  else
    STEELC=""
  fi
fi

if [ -z "$STEEL" ] || [ -z "$STEELC" ]; then
  die "missing tools (steel/steelc). Build them first: ./build/scripts/steel_bootstrap.sh"
fi

# ----------------------------- build args --------------------------------

STEEL_ARGS=""

# Mode
STEEL_ARGS="$STEEL_ARGS --mode $MODE"

# Target profile/triple
if [ -n "$TARGET_NAME" ]; then
  STEEL_ARGS="$STEEL_ARGS --target $TARGET_NAME"
fi

# Package
if [ -n "$PKG" ]; then
  STEEL_ARGS="$STEEL_ARGS --pkg $PKG"
fi

# Jobs
if [ -n "$JOBS" ]; then
  STEEL_ARGS="$STEEL_ARGS --jobs $JOBS"
fi

# Toolchain overrides
if [ -n "$CC_OVERRIDE" ]; then
  STEEL_ARGS="$STEEL_ARGS --cc $CC_OVERRIDE"
fi
if [ -n "$ZIG_OVERRIDE" ]; then
  STEEL_ARGS="$STEEL_ARGS --zig $ZIG_OVERRIDE"
fi

# Verbosity
if [ "$VERBOSE" -eq 1 ]; then
  STEEL_ARGS="$STEEL_ARGS --verbose"
fi
if [ "$QUIET" -eq 1 ]; then
  STEEL_ARGS="$STEEL_ARGS --quiet"
fi

# Manifest path
STEEL_ARGS="$STEEL_ARGS --config $CONFIG_PATH"

# ------------------------------- plan ------------------------------------

PLAN_TXT="$PLAN_DIR/plan.txt"
PLAN_JSON="$PLAN_DIR/plan.json"

if [ "$DO_PLAN" -eq 1 ]; then
  if steel_supports "$STEEL" plan; then
    if [ "$QUIET" -ne 1 ]; then
      say "steel_build: planning -> $PLAN_TXT"
    fi

    # Best-effort: if steel supports --format/--out, use it; otherwise capture stdout.
    if "$STEEL" plan --help 2>/dev/null | grep -q "--format"; then
      if "$STEEL" plan --help 2>/dev/null | grep -q "json"; then
        # shellcheck disable=SC2086
        if "$STEEL" plan $STEEL_ARGS --format json >"$PLAN_JSON" 2>"$PLAN_DIR/plan.stderr"; then
          :
        else
          # fallback to text
          # shellcheck disable=SC2086
          "$STEEL" plan $STEEL_ARGS >"$PLAN_TXT" 2>"$PLAN_DIR/plan.stderr" || die "plan failed"
        fi
      else
        # shellcheck disable=SC2086
        "$STEEL" plan $STEEL_ARGS >"$PLAN_TXT" 2>"$PLAN_DIR/plan.stderr" || die "plan failed"
      fi
    else
      # shellcheck disable=SC2086
      "$STEEL" plan $STEEL_ARGS >"$PLAN_TXT" 2>"$PLAN_DIR/plan.stderr" || die "plan failed"
    fi

  else
    warn "steel does not expose 'plan' subcommand; skipping plan step"
  fi
fi

if [ "$PLAN_ONLY" -eq 1 ]; then
  if [ "$QUIET" -ne 1 ]; then
    say "steel_build: plan-only; done"
  fi
  exit 0
fi

# ------------------------------- build -----------------------------------

if steel_supports "$STEEL" build; then
  if [ "$QUIET" -ne 1 ]; then
    say "steel_build: build"
  fi

  # shellcheck disable=SC2086
  "$STEEL" build $STEEL_ARGS || die "build failed"

else
  # Fallback: call steelc directly only if a minimal compatible CLI exists.
  # This keeps the script useful while the steel frontend evolves.
  if "$STEELC" --help 2>/dev/null | grep -q "--config"; then
    if [ "$QUIET" -ne 1 ]; then
      warn "steel 'build' not available; falling back to steelc (single-unit build)"
    fi

    # Minimal best-effort compilation mode:
    #   - Let steelc interpret muffin.muf and drive compilation.
    #   - The exact flags are expected to stabilize in steelc.
    # shellcheck disable=SC2086
    "$STEELC" --config "$CONFIG_PATH" --mode "$MODE" ${TARGET_NAME:+--target "$TARGET_NAME"} ${PKG:+--pkg "$PKG"} ${JOBS:+--jobs "$JOBS"} \
      ${CC_OVERRIDE:+--cc "$CC_OVERRIDE"} ${ZIG_OVERRIDE:+--zig "$ZIG_OVERRIDE"} \
      ${VERBOSE:+--verbose} ${QUIET:+--quiet} || die "steelc build failed"

  else
    die "steel has no 'build' and steelc has no compatible CLI yet"
  fi
fi

if [ "$QUIET" -ne 1 ]; then
  say "steel_build: ok"
fi

exit 0