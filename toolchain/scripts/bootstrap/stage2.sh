#!/usr/bin/env sh
# ============================================================
# stage2.sh — Vitte Toolchain
# Bootstrap stage2 (fully self-hosted compiler)
# ============================================================

set -eu

# ------------------------------------------------------------
# Helpers
# ------------------------------------------------------------

log() {
    printf "[stage2] %s\n" "$1"
}

die() {
    printf "[stage2][error] %s\n" "$1" >&2
    exit 1
}

# ------------------------------------------------------------
# Paths
# ------------------------------------------------------------

ROOT_DIR="$(cd "$(dirname "$0")/../../.." && pwd)"
BIN_DIR="$ROOT_DIR/bin"

STAGE1_BIN="$BIN_DIR/vittec1"

STAGE2_DIR="$ROOT_DIR/toolchain/stage2"
BUILD_DIR="$STAGE2_DIR/build"
OUT_DIR="$STAGE2_DIR/out"

# ------------------------------------------------------------
# Environment
# ------------------------------------------------------------

BUILD_TYPE="${BUILD_TYPE:-Release}"
OPT_LEVEL="${OPT_LEVEL:-2}"

# ------------------------------------------------------------
# Checks
# ------------------------------------------------------------

log "root      = $ROOT_DIR"
log "stage2    = $STAGE2_DIR"

[ -x "$STAGE1_BIN" ] || die "vittec1 not found (run stage1 first)"
[ -d "$STAGE2_DIR/src" ] || die "stage2 sources missing"

# ------------------------------------------------------------
# Prepare directories
# ------------------------------------------------------------

log "preparing directories"

mkdir -p "$BUILD_DIR"
mkdir -p "$OUT_DIR"
mkdir -p "$BIN_DIR"

# ------------------------------------------------------------
# Build stage2 compiler
# ------------------------------------------------------------
#
# vittec1 compile le compilateur Vitte écrit en Vitte
# avec stdlib + linker complets
#

log "building final vittec using vittec1"

"$STAGE1_BIN" \
    build \
    --stage stage2 \
    --src "$STAGE2_DIR/src" \
    --out "$OUT_DIR" \
    --opt "$OPT_LEVEL" \
    --release

# ------------------------------------------------------------
# Verify output
# ------------------------------------------------------------

VITTEC_BIN="$OUT_DIR/vittec"

[ -x "$VITTEC_BIN" ] || die "final vittec not produced"

log "installing vittec → $BIN_DIR"

cp "$VITTEC_BIN" "$BIN_DIR/vittec"
chmod +x "$BIN_DIR/vittec"

# ------------------------------------------------------------
# Self-check
# ------------------------------------------------------------

log "verifying final vittec"

"$BIN_DIR/vittec" --version || die "vittec verification failed"

# Optional: self-rebuild check (can be disabled in CI)
if [ "${VITTE_SELF_CHECK:-1}" -eq 1 ]; then
    log "running self-hosting check (rebuild compiler)"

    "$BIN_DIR/vittec" \
        build \
        --stage stage2 \
        --src "$STAGE2_DIR/src" \
        --out "$BUILD_DIR/selfcheck" \
        --opt "$OPT_LEVEL" \
        --release
fi

log "stage2 completed successfully"