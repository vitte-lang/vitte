#!/usr/bin/env sh
# ============================================================
# stage1.sh — Vitte Toolchain
# Bootstrap stage1 (Vitte compiler built by vittec0)
# ============================================================

set -eu

# ------------------------------------------------------------
# Helpers
# ------------------------------------------------------------

log() {
    printf "[stage1] %s\n" "$1"
}

die() {
    printf "[stage1][error] %s\n" "$1" >&2
    exit 1
}

# ------------------------------------------------------------
# Paths
# ------------------------------------------------------------

ROOT_DIR="$(cd "$(dirname "$0")/../../.." && pwd)"
BIN_DIR="$ROOT_DIR/bin"

STAGE0_BIN="$BIN_DIR/vittec0"

STAGE1_DIR="$ROOT_DIR/toolchain/stage1"
BUILD_DIR="$STAGE1_DIR/build"
OUT_DIR="$STAGE1_DIR/out"

# ------------------------------------------------------------
# Environment
# ------------------------------------------------------------

BUILD_TYPE="${BUILD_TYPE:-Release}"

# ------------------------------------------------------------
# Checks
# ------------------------------------------------------------

log "root      = $ROOT_DIR"
log "stage1    = $STAGE1_DIR"

[ -x "$STAGE0_BIN" ] || die "vittec0 not found (run stage0 first)"
[ -d "$STAGE1_DIR/src" ] || die "stage1 sources missing"

# ------------------------------------------------------------
# Prepare directories
# ------------------------------------------------------------

log "preparing directories"

mkdir -p "$BUILD_DIR"
mkdir -p "$OUT_DIR"
mkdir -p "$BIN_DIR"

# ------------------------------------------------------------
# Build stage1 compiler
# ------------------------------------------------------------
#
# vittec0 compile tout le compilateur Vitte écrit en Vitte
#

log "building vittec1 using vittec0"

"$STAGE0_BIN" \
    build \
    --stage stage1 \
    --src "$STAGE1_DIR/src" \
    --out "$OUT_DIR" \
    --opt 0 \
    --debug

# ------------------------------------------------------------
# Verify output
# ------------------------------------------------------------

VITTEC1_BIN="$OUT_DIR/vittec1"

[ -x "$VITTEC1_BIN" ] || die "vittec1 not produced"

log "installing vittec1 → $BIN_DIR"

cp "$VITTEC1_BIN" "$BIN_DIR/vittec1"
chmod +x "$BIN_DIR/vittec1"

# ------------------------------------------------------------
# Smoke test
# ------------------------------------------------------------

log "verifying vittec1"

"$BIN_DIR/vittec1" --version || die "vittec1 verification failed"

log "stage1 completed successfully"