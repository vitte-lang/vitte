#!/usr/bin/env sh
# C:\Users\vince\Documents\GitHub\vitte\tools\scripts\debug_run.sh
#
# Debug run helper for Vitte (runs the Debug build of vittec with args).
#
# Behavior:
#   - ensures Debug build exists (CMake configure+build if missing or --rebuild)
#   - runs vittec with provided arguments
#   - optional: runs under a debugger (lldb/gdb) if requested
#
# Usage:
#   sh tools/scripts/debug_run.sh -- --help
#   sh tools/scripts/debug_run.sh --args "--help"
#   sh tools/scripts/debug_run.sh --rebuild --args "compile examples/hello.vitte"
#   sh tools/scripts/debug_run.sh --debugger lldb --args "--version"
#
# Exit codes:
#   0 ok
#   2 configure/build failed
#   3 run failed

set -eu

REBUILD=0
BUILD_DIR="build/debug"
BIN_NAME="vittec"
ARGS=""
DEBUGGER="none"   # none|lldb|gdb

usage() {
  cat <<EOF
Usage: sh tools/scripts/debug_run.sh [options] [-- <args...>]
  --rebuild            force reconfigure + rebuild
  --build-dir DIR      debug build directory (default: $BUILD_DIR)
  --bin NAME           binary name (default: $BIN_NAME)
  --args "ARGS"        arguments passed as a single string (default: empty)
  --debugger DBG       none|lldb|gdb (default: $DEBUGGER)
  -h, --help
EOF
}

die(){ printf "%s\n" "$*" 1>&2; exit 2; }
has(){ command -v "$1" >/dev/null 2>&1; }

# Parse options; everything after "--" is passthrough args
PASSTHRU=""
while [ $# -gt 0 ]; do
  case "$1" in
    --rebuild) REBUILD=1 ;;
    --build-dir) shift; [ $# -gt 0 ] || die "--build-dir requires a value"; BUILD_DIR="$1" ;;
    --bin) shift; [ $# -gt 0 ] || die "--bin requires a value"; BIN_NAME="$1" ;;
    --args) shift; [ $# -gt 0 ] || die "--args requires a value"; ARGS="$1" ;;
    --debugger) shift; [ $# -gt 0 ] || die "--debugger requires a value"; DEBUGGER="$1" ;;
    -h|--help) usage; exit 0 ;;
    --) shift; PASSTHRU="$*"; break ;;
    *) die "Unknown arg: $1 (use --help)" ;;
  esac
  shift
done

# If ARGS not set, use passthrough args
if [ -z "$ARGS" ] && [ -n "${PASSTHRU:-}" ]; then
  ARGS="$PASSTHRU"
fi

has cmake || die "cmake not found in PATH"

GEN=""
has ninja && GEN="-G Ninja"

mkdir -p "$BUILD_DIR"

CACHE="$BUILD_DIR/CMakeCache.txt"
if [ "$REBUILD" -eq 1 ] && [ -d "$BUILD_DIR" ]; then
  rm -rf "$BUILD_DIR"
  mkdir -p "$BUILD_DIR"
fi

if [ "$REBUILD" -eq 1 ] || [ ! -f "$CACHE" ]; then
  cmake -S . -B "$BUILD_DIR" $GEN \
    -DCMAKE_BUILD_TYPE=Debug \
    -DVITTE_BUILD_TESTS=OFF \
    -DVITTE_BUILD_BENCH=OFF \
    -DVITTE_ENABLE_SANITIZERS=OFF
fi

cmake --build "$BUILD_DIR" --config Debug

BIN_PATH=""
if [ -x "$BUILD_DIR/$BIN_NAME" ]; then
  BIN_PATH="$BUILD_DIR/$BIN_NAME"
elif [ -x "$BUILD_DIR/$BIN_NAME.exe" ]; then
  BIN_PATH="$BUILD_DIR/$BIN_NAME.exe"
elif [ -f "$BUILD_DIR/$BIN_NAME.exe" ]; then
  BIN_PATH="$BUILD_DIR/$BIN_NAME.exe"
else
  die "Binary not found in $BUILD_DIR ($BIN_NAME or $BIN_NAME.exe)"
fi

printf "[debug_run] bin=%s\n" "$BIN_PATH"
printf "[debug_run] args=%s\n" "${ARGS:-}"

case "$DEBUGGER" in
  none)
    # shellcheck disable=SC2086
    "$BIN_PATH" ${ARGS:-} || exit 3
    ;;
  lldb)
    has lldb || die "lldb not found in PATH"
    # shellcheck disable=SC2086
    lldb -- "$BIN_PATH" ${ARGS:-} || exit 3
    ;;
  gdb)
    has gdb || die "gdb not found in PATH"
    # shellcheck disable=SC2086
    gdb --args "$BIN_PATH" ${ARGS:-} || exit 3
    ;;
  *)
    die "Unknown debugger: $DEBUGGER (none|lldb|gdb)"
    ;;
esac

exit 0
