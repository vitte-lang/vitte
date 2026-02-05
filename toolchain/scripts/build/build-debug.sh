#!/usr/bin/env sh
# ============================================================
# build-debug.sh — Vitte Toolchain
# Debug build (compiler + stdlib)
# ============================================================

set -eu

# ------------------------------------------------------------
# Helpers
# ------------------------------------------------------------

log() {
    printf "[build-debug] %s\n" "$1"
}

die() {
    printf "[build-debug][error] %s\n" "$1" >&2
    exit 1
}

# ------------------------------------------------------------
# Paths
# ------------------------------------------------------------

ROOT_DIR="$(cd "$(dirname "$0")/../../.." && pwd)"
BIN_DIR="$ROOT_DIR/bin"
BUILD_DIR="$ROOT_DIR/build/debug"
SRC_DIR="$ROOT_DIR/src"

# ------------------------------------------------------------
# Environment
# ------------------------------------------------------------

BUILD_TYPE="Debug"
CMAKE_GENERATOR="${CMAKE_GENERATOR:-}"
CMAKE_OPTS=""

if [ -n "$CMAKE_GENERATOR" ]; then
    CMAKE_OPTS="$CMAKE_OPTS -G \"$CMAKE_GENERATOR\""
fi

# Debug flags (can be overridden)
CFLAGS="${CFLAGS:--O0 -g -Wall -Wextra}"
CXXFLAGS="${CXXFLAGS:--O0 -g -Wall -Wextra -std=c++20}"

# ------------------------------------------------------------
# Checks
# ------------------------------------------------------------

log "root       = $ROOT_DIR"
log "build dir  = $BUILD_DIR"
log "build type = $BUILD_TYPE"

command -v cmake >/dev/null 2>&1 || die "cmake not found"
[ -d "$SRC_DIR" ] || die "src directory not found"

# ------------------------------------------------------------
# Prepare directories
# ------------------------------------------------------------

log "preparing directories"

mkdir -p "$BUILD_DIR"
mkdir -p "$BIN_DIR"

# ------------------------------------------------------------
# Build
# ------------------------------------------------------------

if [ -f "$ROOT_DIR/CMakeLists.txt" ]; then
    log "configuring (CMake)"
    cd "$BUILD_DIR"
    cmake \
        -S "$ROOT_DIR" \
        -B "$BUILD_DIR" \
        -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
        -DCMAKE_C_FLAGS="$CFLAGS" \
        -DCMAKE_CXX_FLAGS="$CXXFLAGS" \
        -DVITTE_ENABLE_ASSERTS=ON \
        -DVITTE_ENABLE_DEBUG_LOGS=ON

    log "building (parallel)"
    cmake --build "$BUILD_DIR" --parallel

    if [ -x "$BUILD_DIR/bin/vittec" ]; then
        log "installing vittec → $BIN_DIR"
        cp "$BUILD_DIR/bin/vittec" "$BIN_DIR/vittec-debug"
        chmod +x "$BIN_DIR/vittec-debug"
    else
        log "warning: vittec binary not found (expected in build/bin)"
    fi
else
    log "CMakeLists.txt not found; falling back to Makefile build"
    log "building (make)"
    (cd "$ROOT_DIR" && CFLAGS="$CFLAGS" CXXFLAGS="$CXXFLAGS" make build)
    if [ -x "$BIN_DIR/vitte" ]; then
        log "installing vitte → $BIN_DIR/vittec-debug"
        cp "$BIN_DIR/vitte" "$BIN_DIR/vittec-debug"
        chmod +x "$BIN_DIR/vittec-debug"
    else
        log "warning: vitte binary not found in bin/"
    fi
fi

# ------------------------------------------------------------
# Smoke test
# ------------------------------------------------------------

if [ -x "$BIN_DIR/vittec-debug" ]; then
    log "running smoke test"
    "$BIN_DIR/vittec-debug" -h >/dev/null 2>&1 || die "vittec-debug failed"
fi

log "debug build completed successfully"
