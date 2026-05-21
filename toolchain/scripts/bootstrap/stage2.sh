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
COMPILER_SOURCE_ROOT="$ROOT_DIR/src/vitte/compiler"
COMPILER_ENTRY_POINT="$COMPILER_SOURCE_ROOT/driver/compiler.vit"
BUILD_DIR="$ROOT_DIR/target/bootstrap/stage2-build"
OUT_DIR="$ROOT_DIR/target/bootstrap/stage2"

# ------------------------------------------------------------
# Environment
# ------------------------------------------------------------

BUILD_TYPE="${BUILD_TYPE:-Release}"
OPT_LEVEL="${OPT_LEVEL:-2}"
VITTE_BACKEND_MODE="${VITTE_BACKEND_MODE:-shell}"
VITTE_BACKEND_FALLBACK="${VITTE_BACKEND_FALLBACK:-1}"

# ------------------------------------------------------------
# Checks
# ------------------------------------------------------------

log "root      = $ROOT_DIR"
log "stage2    = $STAGE2_DIR"

[ -x "$STAGE1_BIN" ] || die "vittec1 not found (run stage1 first)"
[ -d "$STAGE2_DIR/src" ] || die "stage2 sources missing"
[ -d "$COMPILER_SOURCE_ROOT" ] || die "compiler source root missing: $COMPILER_SOURCE_ROOT"
[ -f "$COMPILER_ENTRY_POINT" ] || die "compiler entry point missing: $COMPILER_ENTRY_POINT"

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
SRC_VIT="$COMPILER_ENTRY_POINT"

[ -f "$SRC_VIT" ] || die "stage2 entry source missing (expected $SRC_VIT)"
log "compiler source root = $COMPILER_SOURCE_ROOT"
log "compiler entry      = $COMPILER_ENTRY_POINT"
case "$VITTE_BACKEND_MODE" in
    shell)
        log "backend mode=shell"
        "$STAGE1_BIN" build-native --src "$SRC_VIT" --out "$OUT_DIR/vittec" || die "stage2 build-native failed"
        ;;
    native)
        log "backend mode=native (experimental)"
        if "$STAGE1_BIN" build --stage stage1 --src "$STAGE2_DIR/src" --out "$OUT_DIR" >/dev/null 2>&1 && [ -x "$OUT_DIR/vittec1" ]; then
            cp "$OUT_DIR/vittec1" "$OUT_DIR/vittec"
        elif [ "$VITTE_BACKEND_FALLBACK" = "1" ]; then
            log "native backend unavailable; explicit fallback -> shell"
            "$STAGE1_BIN" build-native --src "$SRC_VIT" --out "$OUT_DIR/vittec" || die "stage2 build-native fallback failed"
        else
            die "native backend unavailable and fallback disabled (set VITTE_BACKEND_FALLBACK=1)"
        fi
        ;;
    *)
        die "unsupported VITTE_BACKEND_MODE=$VITTE_BACKEND_MODE (expected shell|native)"
        ;;
esac

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
ROOT_OUT="$("$BIN_DIR/vittec" selfhost-source || true)"
echo "$ROOT_OUT" | grep -q "^compiler_source_root=src/vitte/compiler$" || die "vittec selfhost source root mismatch"
echo "$ROOT_OUT" | grep -q "^compiler_entry_point=src/vitte/compiler/driver/compiler.vit$" || die "vittec selfhost entry point mismatch"

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
    SELF_ROOT_OUT="$("$SELF_BIN" selfhost-source || true)"
    echo "$SELF_ROOT_OUT" | grep -q "^compiler_source_root=src/vitte/compiler$" || die "stage2 selfcheck source root mismatch"
    echo "$SELF_ROOT_OUT" | grep -q "^compiler_entry_point=src/vitte/compiler/driver/compiler.vit$" || die "stage2 selfcheck entry point mismatch"
fi

log "stage2 completed successfully"
