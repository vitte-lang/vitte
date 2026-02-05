#!/usr/bin/env sh
# ============================================================
# clean.sh — Vitte Toolchain
# Clean build artifacts
# ============================================================

set -eu

# ------------------------------------------------------------
# Helpers
# ------------------------------------------------------------

log() {
    printf "[clean] %s\n" "$1"
}

die() {
    printf "[clean][error] %s\n" "$1" >&2
    exit 1
}

# ------------------------------------------------------------
# Paths
# ------------------------------------------------------------

ROOT_DIR="$(cd "$(dirname "$0")/../../.." && pwd)"
BUILD_DIR="$ROOT_DIR/build"
CACHE_DIR="$ROOT_DIR/.cache"
TMP_DIR="$ROOT_DIR/.tmp"

BIN_DIR="$ROOT_DIR/bin"

# ------------------------------------------------------------
# Options
# ------------------------------------------------------------
# Usage:
#   ./clean.sh            → clean build directories only
#   ./clean.sh --all      → clean build + caches
#   ./clean.sh --dist     → clean build + caches + bin (DANGEROUS)
#

MODE="build"

case "${1:-}" in
    --all)
        MODE="all"
        ;;
    --dist)
        MODE="dist"
        ;;
    "")
        ;;
    *)
        die "unknown option: $1"
        ;;
esac

# ------------------------------------------------------------
# Clean build artifacts
# ------------------------------------------------------------

log "cleaning build artifacts"

if [ -d "$BUILD_DIR" ]; then
    rm -rf "$BUILD_DIR"
    log "removed $BUILD_DIR"
else
    log "no build directory"
fi

# ------------------------------------------------------------
# Clean caches (optional)
# ------------------------------------------------------------

if [ "$MODE" = "all" ] || [ "$MODE" = "dist" ]; then
    log "cleaning caches"

    if [ -d "$CACHE_DIR" ]; then
        rm -rf "$CACHE_DIR"
        log "removed $CACHE_DIR"
    fi

    if [ -d "$TMP_DIR" ]; then
        rm -rf "$TMP_DIR"
        log "removed $TMP_DIR"
    fi
fi

# ------------------------------------------------------------
# Clean installed binaries (DANGEROUS)
# ------------------------------------------------------------

if [ "$MODE" = "dist" ]; then
    log "cleaning installed binaries (dist clean)"

    if [ -d "$BIN_DIR" ]; then
        rm -rf "$BIN_DIR"
        log "removed $BIN_DIR"
    fi
fi

log "clean completed successfully (mode: $MODE)"