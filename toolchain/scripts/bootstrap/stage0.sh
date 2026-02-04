#!/usr/bin/env sh
# ============================================================
# stage0.sh — Vitte Toolchain
# Bootstrap stage0 (C/C++ compiler)
# ============================================================

set -eu

# ------------------------------------------------------------
# Helpers
# ------------------------------------------------------------

log() {
    printf "[stage0] %s\n" "$1"
}

die() {
    printf "[stage0][error] %s\n" "$1" >&2
    exit 1
}

# ------------------------------------------------------------
# Paths
# ------------------------------------------------------------

ROOT_DIR="$(cd "$(dirname "$0")/../../.." && pwd)"
STAGE0_DIR="$ROOT_DIR/toolchain/stage0"
BUILD_DIR="$STAGE0_DIR/build"
BIN_DIR="$ROOT_DIR/bin"

# ------------------------------------------------------------
# Environment
# ------------------------------------------------------------

CXX="${CXX:-c++}"
CXXFLAGS="${CXXFLAGS:--std=c++20 -O2 -Wall -Wextra -pedantic}"
BUILD_TYPE="${BUILD_TYPE:-Release}"

# ------------------------------------------------------------
# Checks
# ------------------------------------------------------------

log "root      = $ROOT_DIR"
log "stage0    = $STAGE0_DIR"
log "compiler  = $CXX"

command -v "$CXX" >/dev/null 2>&1 || die "C++ compiler not found"

[ -d "$STAGE0_DIR/src" ] || die "stage0 sources missing"

# ------------------------------------------------------------
# Prepare build directories
# ------------------------------------------------------------

log "preparing build directories"

mkdir -p "$BUILD_DIR"
mkdir -p "$BIN_DIR"

# ------------------------------------------------------------
# Configure (CMake)
# ------------------------------------------------------------

log "configuring stage0 (CMake)"

cd "$BUILD_DIR"

cmake \
    -S "$STAGE0_DIR" \
    -B "$BUILD_DIR" \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DCMAKE_CXX_COMPILER="$CXX" \
    -DCMAKE_CXX_FLAGS="$CXXFLAGS"

# ------------------------------------------------------------
# Build
# ------------------------------------------------------------

log "building stage0"

cmake --build "$BUILD_DIR" --parallel

# ------------------------------------------------------------
# Install artifact
# ------------------------------------------------------------

if [ ! -f "$BUILD_DIR/vittec0" ]; then
    die "vittec0 not produced"
fi

log "installing vittec0 → $BIN_DIR"

cp "$BUILD_DIR/vittec0" "$BIN_DIR/vittec0"
chmod +x "$BIN_DIR/vittec0"

# ------------------------------------------------------------
# Verification
# ------------------------------------------------------------

log "verifying vittec0"

"$BIN_DIR/vittec0" --version || die "vittec0 verification failed"

log "stage0 completed successfully"