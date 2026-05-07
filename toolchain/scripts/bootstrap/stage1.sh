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
STAGE1_BIN="$BIN_DIR/vittec1"

STAGE1_DIR="$ROOT_DIR/toolchain/stage1"
OUT_DIR="$ROOT_DIR/target/bootstrap/stage1"

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
# Stage0 compatibility gate
# ------------------------------------------------------------

log "running stage0 compatibility checks"

check_files="$(
    {
        find "$ROOT_DIR/src/vitte/compiler/driver" -type f -name '*.vit'
        find "$ROOT_DIR/src/vitte/compiler/ir" -type f -name '*.vit'
        find "$ROOT_DIR/src/vitte/compiler/frontend" -type f -name '*.vit'
    } | sort
)"
[ -n "$check_files" ] || die "no compiler .vit files found for stage0 compatibility gate"

for src in $check_files; do
    "$STAGE0_BIN" check "$src" || die "stage0 compatibility failed: $src"
done

log "building vittec1 from Vitte source via vittec0"
mkdir -p "$OUT_DIR"
"$STAGE0_BIN" build-native --src "$STAGE1_DIR/src/main.vit" --out "$OUT_DIR/vittec1" || die "stage1 build-native failed"
[ -x "$OUT_DIR/vittec1" ] || die "vittec1 not produced"
cp "$OUT_DIR/vittec1" "$STAGE1_BIN"
chmod +x "$STAGE1_BIN"

# ------------------------------------------------------------
# Smoke test
# ------------------------------------------------------------

log "verifying vittec1"

"$BIN_DIR/vittec1" --version || die "vittec1 verification failed"

log "stage1 completed successfully"
