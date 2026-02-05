#!/usr/bin/env sh
# ============================================================
# build-release.sh — Vitte Toolchain
# Release build (compiler + stdlib)
# ============================================================

set -eu

# ------------------------------------------------------------
# Helpers
# ------------------------------------------------------------

log() {
    printf "[build-release] %s\n" "$1"
}

die() {
    printf "[build-release][error] %s\n" "$1" >&2
    exit 1
}

# ------------------------------------------------------------
# Paths
# ------------------------------------------------------------

ROOT_DIR="$(cd "$(dirname "$0")/../../.." && pwd)"
BIN_DIR="$ROOT_DIR/bin"
BUILD_DIR="$ROOT_DIR/build/release"
SRC_DIR="$ROOT_DIR/src"

# ------------------------------------------------------------
# Environment
# ------------------------------------------------------------

BUILD_TYPE="Release"
CMAKE_GENERATOR="${CMAKE_GENERATOR:-}"

# Optimisation agressive mais sûre
CFLAGS="${CFLAGS:--O3 -DNDEBUG}"
CXXFLAGS="${CXXFLAGS:--O3 -DNDEBUG -std=c++20}"

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
# Configure (CMake)
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
        -DVITTE_ENABLE_ASSERTS=OFF \
        -DVITTE_ENABLE_DEBUG_LOGS=OFF
fi

# ------------------------------------------------------------
# Build
# ------------------------------------------------------------

if [ -f "$ROOT_DIR/CMakeLists.txt" ]; then
    log "building (parallel)"
    cmake --build "$BUILD_DIR" --parallel
else
    log "CMakeLists.txt not found; falling back to Makefile build"
    log "building (make)"
    (cd "$ROOT_DIR" && CFLAGS="$CFLAGS" CXXFLAGS="$CXXFLAGS" make build)
fi

# ------------------------------------------------------------
# Artifacts
# ------------------------------------------------------------

if [ -x "$BUILD_DIR/bin/vittec" ]; then
    log "installing vittec → $BIN_DIR"
    cp "$BUILD_DIR/bin/vittec" "$BIN_DIR/vittec"
    chmod +x "$BIN_DIR/vittec"
elif [ -x "$BIN_DIR/vitte" ]; then
    log "installing vitte → $BIN_DIR/vittec"
    cp "$BIN_DIR/vitte" "$BIN_DIR/vittec"
    chmod +x "$BIN_DIR/vittec"
else
    die "vittec binary not found"
fi

# ------------------------------------------------------------
# Smoke test
# ------------------------------------------------------------

log "running smoke test"

"$BIN_DIR/vittec" -h >/dev/null 2>&1 || die "vittec failed to run"

# ------------------------------------------------------------
# Size / strip info (optional)
# ------------------------------------------------------------

if command -v strip >/dev/null 2>&1; then
    log "stripping binary"
    strip "$BIN_DIR/vittec" || log "strip failed (ignored)"
fi

if command -v ls >/dev/null 2>&1; then
    log "binary size:"
    ls -lh "$BIN_DIR/vittec"
fi

log "release build completed successfully"
