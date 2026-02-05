#!/usr/bin/env sh
# ============================================================
# configure.sh — Vitte Toolchain
# Central CMake configuration (no build)
# ============================================================

set -eu

# ------------------------------------------------------------
# Helpers
# ------------------------------------------------------------

log() {
    printf "[configure] %s\n" "$1"
}

die() {
    printf "[configure][error] %s\n" "$1" >&2
    exit 1
}

# ------------------------------------------------------------
# Paths
# ------------------------------------------------------------

ROOT_DIR="$(cd "$(dirname "$0")/../../.." && pwd)"
BUILD_ROOT="$ROOT_DIR/build"
SRC_DIR="$ROOT_DIR/src"

# ------------------------------------------------------------
# Defaults
# ------------------------------------------------------------

BUILD_TYPE="${BUILD_TYPE:-Debug}"        # Debug | Release | RelWithDebInfo
PROFILE="${PROFILE:-dev}"                # dev | ci | release
GENERATOR="${CMAKE_GENERATOR:-}"
TOOLCHAIN_FILE="${TOOLCHAIN_FILE:-}"

# Common toggles (override via env)
VITTE_ENABLE_ASSERTS="${VITTE_ENABLE_ASSERTS:-ON}"
VITTE_ENABLE_DEBUG_LOGS="${VITTE_ENABLE_DEBUG_LOGS:-ON}"
VITTE_ENABLE_TESTS="${VITTE_ENABLE_TESTS:-ON}"

# ------------------------------------------------------------
# Parse args
# ------------------------------------------------------------
# Usage:
#   ./configure.sh
#   ./configure.sh debug|release|relwithdebinfo
#   PROFILE=ci ./configure.sh release
#

case "${1:-}" in
    debug)
        BUILD_TYPE="Debug"
        ;;
    release)
        BUILD_TYPE="Release"
        ;;
    relwithdebinfo)
        BUILD_TYPE="RelWithDebInfo"
        ;;
    "")
        ;;
    *)
        die "unknown build type: $1"
        ;;
esac

# ------------------------------------------------------------
# Checks
# ------------------------------------------------------------

log "root       = $ROOT_DIR"
log "src        = $SRC_DIR"
log "build type = $BUILD_TYPE"
log "profile    = $PROFILE"

command -v cmake >/dev/null 2>&1 || die "cmake not found"
[ -d "$SRC_DIR" ] || die "src directory not found"

# ------------------------------------------------------------
# Build directory selection
# ------------------------------------------------------------

case "$BUILD_TYPE" in
    Debug)
        BUILD_DIR="$BUILD_ROOT/debug"
        ;;
    Release)
        BUILD_DIR="$BUILD_ROOT/release"
        ;;
    RelWithDebInfo)
        BUILD_DIR="$BUILD_ROOT/relwithdebinfo"
        ;;
esac

mkdir -p "$BUILD_DIR"

# ------------------------------------------------------------
# Profile presets
# ------------------------------------------------------------

case "$PROFILE" in
    dev)
        VITTE_ENABLE_ASSERTS=ON
        VITTE_ENABLE_DEBUG_LOGS=ON
        VITTE_ENABLE_TESTS=ON
        ;;
    ci)
        VITTE_ENABLE_ASSERTS=ON
        VITTE_ENABLE_DEBUG_LOGS=OFF
        VITTE_ENABLE_TESTS=ON
        ;;
    release)
        VITTE_ENABLE_ASSERTS=OFF
        VITTE_ENABLE_DEBUG_LOGS=OFF
        VITTE_ENABLE_TESTS=OFF
        ;;
    *)
        die "unknown profile: $PROFILE"
        ;;
esac

# ------------------------------------------------------------
# Generator / toolchain
# ------------------------------------------------------------

CMAKE_OPTS=""

if [ -n "$GENERATOR" ]; then
    log "generator  = $GENERATOR"
    CMAKE_OPTS="$CMAKE_OPTS -G \"$GENERATOR\""
fi

if [ -n "$TOOLCHAIN_FILE" ]; then
    log "toolchain  = $TOOLCHAIN_FILE"
    CMAKE_OPTS="$CMAKE_OPTS -DCMAKE_TOOLCHAIN_FILE=$TOOLCHAIN_FILE"
fi

# ------------------------------------------------------------
# Configure
# ------------------------------------------------------------

log "configuring build directory: $BUILD_DIR"

cd "$BUILD_DIR"

cmake \
    -S "$ROOT_DIR" \
    -B "$BUILD_DIR" \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DVITTE_ENABLE_ASSERTS="$VITTE_ENABLE_ASSERTS" \
    -DVITTE_ENABLE_DEBUG_LOGS="$VITTE_ENABLE_DEBUG_LOGS" \
    -DVITTE_ENABLE_TESTS="$VITTE_ENABLE_TESTS"

# ------------------------------------------------------------
# Summary
# ------------------------------------------------------------

log "configuration completed"
log "  build dir  : $BUILD_DIR"
log "  build type : $BUILD_TYPE"
log "  profile    : $PROFILE"
log "  asserts    : $VITTE_ENABLE_ASSERTS"
log "  debug logs : $VITTE_ENABLE_DEBUG_LOGS"
log "  tests      : $VITTE_ENABLE_TESTS"