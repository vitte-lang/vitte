#!/usr/bin/env sh
# ============================================================
# env.sh — Vitte Toolchain
# Build environment setup
# ============================================================

set -eu

# ------------------------------------------------------------
# Helpers
# ------------------------------------------------------------

log() {
    printf "[env] %s\n" "$1"
}

die() {
    printf "[env][error] %s\n" "$1" >&2
    exit 1
}

# ------------------------------------------------------------
# Paths
# ------------------------------------------------------------

ROOT_DIR="$(cd "$(dirname "$0")/../../.." && pwd)"
BIN_DIR="$ROOT_DIR/bin"
BUILD_DIR="$ROOT_DIR/build"
CACHE_DIR="$ROOT_DIR/.cache"
TMP_DIR="$ROOT_DIR/.tmp"

# ------------------------------------------------------------
# Defaults (override via env before sourcing)
# ------------------------------------------------------------

# Toolchain
export CC="${CC:-cc}"
export CXX="${CXX:-c++}"

# Build
export BUILD_TYPE="${BUILD_TYPE:-Debug}"     # Debug | Release | RelWithDebInfo
export PROFILE="${PROFILE:-dev}"              # dev | ci | release

# Vitte toggles
export VITTE_ENABLE_ASSERTS="${VITTE_ENABLE_ASSERTS:-ON}"
export VITTE_ENABLE_DEBUG_LOGS="${VITTE_ENABLE_DEBUG_LOGS:-ON}"
export VITTE_ENABLE_TESTS="${VITTE_ENABLE_TESTS:-ON}"

# Optimisation / debug flags (used by CMake)
export CFLAGS="${CFLAGS:--Wall -Wextra}"
export CXXFLAGS="${CXXFLAGS:--Wall -Wextra -std=c++20}"

# Toolchain / cross
export CMAKE_GENERATOR="${CMAKE_GENERATOR:-}"
export CMAKE_TOOLCHAIN_FILE="${CMAKE_TOOLCHAIN_FILE:-}"

# Runtime
export VITTE_ROOT="$ROOT_DIR"
export VITTE_BIN="$BIN_DIR"

# ------------------------------------------------------------
# Sanity checks
# ------------------------------------------------------------

command -v "$CC"  >/dev/null 2>&1 || die "C compiler not found: $CC"
command -v "$CXX" >/dev/null 2>&1 || die "C++ compiler not found: $CXX"
command -v cmake  >/dev/null 2>&1 || die "cmake not found"

# ------------------------------------------------------------
# Directories
# ------------------------------------------------------------

mkdir -p "$BIN_DIR"
mkdir -p "$BUILD_DIR"
mkdir -p "$CACHE_DIR"
mkdir -p "$TMP_DIR"

# ------------------------------------------------------------
# PATH injection
# ------------------------------------------------------------

case ":$PATH:" in
    *":$BIN_DIR:"*)
        ;;
    *)
        export PATH="$BIN_DIR:$PATH"
        ;;
esac

# ------------------------------------------------------------
# Profile presets
# ------------------------------------------------------------

case "$PROFILE" in
    dev)
        export VITTE_ENABLE_ASSERTS=ON
        export VITTE_ENABLE_DEBUG_LOGS=ON
        export VITTE_ENABLE_TESTS=ON
        ;;
    ci)
        export VITTE_ENABLE_ASSERTS=ON
        export VITTE_ENABLE_DEBUG_LOGS=OFF
        export VITTE_ENABLE_TESTS=ON
        ;;
    release)
        export VITTE_ENABLE_ASSERTS=OFF
        export VITTE_ENABLE_DEBUG_LOGS=OFF
        export VITTE_ENABLE_TESTS=OFF
        ;;
    *)
        die "unknown profile: $PROFILE"
        ;;
esac

# ------------------------------------------------------------
# Summary
# ------------------------------------------------------------

log "environment initialized"
log "  root        = $ROOT_DIR"
log "  profile     = $PROFILE"
log "  build type  = $BUILD_TYPE"
log "  CC          = $CC"
log "  CXX         = $CXX"
log "  asserts     = $VITTE_ENABLE_ASSERTS"
log "  debug logs  = $VITTE_ENABLE_DEBUG_LOGS"
log "  tests       = $VITTE_ENABLE_TESTS"
log "  bin         = $BIN_DIR"

# ------------------------------------------------------------
# Usage note
# ------------------------------------------------------------
# Source this file:
#   . ./toolchain/scripts/build/env.sh
#
# Then run:
#   ./toolchain/scripts/build/configure.sh
#   ./toolchain/scripts/build/build-debug.sh
#   ./toolchain/scripts/build/build-release.sh