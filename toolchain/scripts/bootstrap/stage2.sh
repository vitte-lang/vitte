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
BUILD_DIR="$ROOT_DIR/target/bootstrap/stage2-build"
OUT_DIR="$ROOT_DIR/target/bootstrap/stage2"

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

log "building final vittec using vittec1"
SRC_VIT="$STAGE2_DIR/src/main.vit"

[ -f "$SRC_VIT" ] || die "stage2 entry source missing (expected $SRC_VIT)"
log "building native stage2 from Vitte source via vittec1"
"$STAGE1_BIN" build-native --src "$SRC_VIT" --out "$OUT_DIR/vittec" || die "stage2 build-native failed"

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
    log "running self-hosting check"

    VERSION_OUT="$("$BIN_DIR/vittec" --version || true)"
    echo "$VERSION_OUT" | grep -q "stage2-vitte" || die "stage2 version identity check failed"

    SELF_DIR="$BUILD_DIR/selfcheck"
    SELF_BIN="$SELF_DIR/vittec"
    mkdir -p "$SELF_DIR"

    "$STAGE1_BIN" build-native --src "$SRC_VIT" --out "$SELF_BIN" || die "stage2 selfcheck build-native failed"
    [ -x "$SELF_BIN" ] || die "stage2 selfcheck compiler missing"
    sh -n "$SELF_BIN" || die "stage2 selfcheck compiler is not POSIX shell"

    SELF_VERSION_OUT="$("$SELF_BIN" --version || true)"
    echo "$SELF_VERSION_OUT" | grep -q "stage2-vitte" || die "stage2 selfcheck version identity failed"
fi

log "stage2 completed successfully"
