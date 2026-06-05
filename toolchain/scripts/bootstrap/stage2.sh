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

is_shell_script() {
    file_path="$1"
    [ -f "$file_path" ] || return 1
    first_line="$(sed -n '1p' "$file_path" 2>/dev/null || true)"
    case "$first_line" in
        '#!'*sh*) return 0 ;;
    esac
    return 1
}

is_machine_executable() {
    file_path="$1"
    [ -f "$file_path" ] || return 1
    kind="$(LC_ALL=C file -b "$file_path" 2>/dev/null || true)"
    case "$kind" in
        *ELF*executable*|*ELF*shared\ object*|*Mach-O*executable*|*Mach-O*dynamically\ linked\ shared\ library*|*PE32*executable*|*PE32+*executable*)
            return 0
            ;;
    esac
    return 1
}

# ------------------------------------------------------------
# Paths
# ------------------------------------------------------------

ROOT_DIR="$(cd "$(dirname "$0")/../../.." && pwd)"
BIN_DIR="$ROOT_DIR/bin"

STAGE1_BIN="$BIN_DIR/vittec1"

STAGE2_DIR="$ROOT_DIR/toolchain/stage2"
COMPILER_SOURCE_ROOT="$ROOT_DIR/src/vitte/compiler"
COMPILER_ENTRY_POINT="$COMPILER_SOURCE_ROOT/main.vit"
BUILD_DIR="$ROOT_DIR/target/bootstrap/stage2-build"
OUT_DIR="$ROOT_DIR/target/bootstrap/stage2"

# ------------------------------------------------------------
# Environment
# ------------------------------------------------------------

BUILD_TYPE="${BUILD_TYPE:-Release}"
OPT_LEVEL="${OPT_LEVEL:-2}"
VITTE_BACKEND_MODE="${VITTE_BACKEND_MODE:-native}"
VITTE_BACKEND_FALLBACK="${VITTE_BACKEND_FALLBACK:-0}"
VITTE_ENFORCE_COMPILER_REACHABILITY="${VITTE_ENFORCE_COMPILER_REACHABILITY:-1}"
VITTE_STAGE2_BOOTSTRAP_COMPAT="${VITTE_STAGE2_BOOTSTRAP_COMPAT:-1}"
VITTE_BOOTSTRAP_ALLOW_FULL_COMPILER_BRIDGE="${VITTE_BOOTSTRAP_ALLOW_FULL_COMPILER_BRIDGE:-0}"
VITTE_STAGE2_ALLOW_BRIDGE_ARTIFACT="${VITTE_STAGE2_ALLOW_BRIDGE_ARTIFACT:-0}"
STAGE2_EFFECTIVE_BACKEND_MODE="$VITTE_BACKEND_MODE"
STAGE2_EFFECTIVE_FALLBACK="$VITTE_BACKEND_FALLBACK"
STAGE2_EFFECTIVE_ALLOW_BRIDGE_ARTIFACT="$VITTE_STAGE2_ALLOW_BRIDGE_ARTIFACT"

if [ "$VITTE_BACKEND_MODE" = "real-native" ]; then
    STAGE2_EFFECTIVE_BACKEND_MODE="native"
    STAGE2_EFFECTIVE_FALLBACK="0"
    STAGE2_EFFECTIVE_ALLOW_BRIDGE_ARTIFACT="0"
fi

# ------------------------------------------------------------
# Checks
# ------------------------------------------------------------

log "root      = $ROOT_DIR"
log "stage2    = $STAGE2_DIR"

[ -x "$STAGE1_BIN" ] || die "vittec1 not found (run stage1 first)"
[ -d "$STAGE2_DIR/src" ] || die "stage2 sources missing"
[ -d "$COMPILER_SOURCE_ROOT" ] || die "compiler source root missing: $COMPILER_SOURCE_ROOT"
[ -f "$COMPILER_ENTRY_POINT" ] || die "compiler entry point missing: $COMPILER_ENTRY_POINT"

if [ "$VITTE_ENFORCE_COMPILER_REACHABILITY" = "1" ]; then
    log "checking compiler module reachability"
    command -v python3 >/dev/null 2>&1 || die "python3 is required for compiler reachability audit"
    python3 "$ROOT_DIR/tools/compiler_reachability_audit.py" || die "compiler modules are not fully integrated from entrypoint"
fi

# ------------------------------------------------------------
# Prepare directories
# ------------------------------------------------------------

log "preparing directories"

mkdir -p "$BUILD_DIR"
mkdir -p "$OUT_DIR"
mkdir -p "$BIN_DIR"

build_stage2_shell_payload() {
    requested_src="$1"
    out_bin="$2"
    entry_err="$BUILD_DIR/bootstrap-entry.err"
    if "$STAGE1_BIN" dump-native-shell --src "$requested_src" > "$out_bin" 2>"$entry_err"; then
        chmod +x "$out_bin"
        sh -n "$out_bin" || {
            cat "$entry_err" >&2
            die "generated stage2 shell payload is not valid POSIX shell"
        }
        rm -f "$entry_err"
        return 0
    fi
    if [ "$VITTE_STAGE2_BOOTSTRAP_COMPAT" = "1" ]; then
        compat_src="$STAGE2_DIR/src/main.vit"
        log "compiler entry exceeds bootstrap subset; using temporary stage2 shell payload: $compat_src"
        "$STAGE1_BIN" dump-native-shell --src "$compat_src" > "$out_bin"
        chmod +x "$out_bin"
        sh -n "$out_bin"
        return $?
    fi
    cat "$entry_err" >&2
    return 1
}

is_bootstrap_bridge_artifact() {
    file_path="$1"
    [ -f "${file_path}.bootstrap-bridge" ]
}

bootstrap_bridge_source_for_artifact() {
    file_path="$1"
    sidecar="${file_path}.bootstrap-bridge"
    [ -f "$sidecar" ] || return 1
    awk -F= '/^src=/ { print $2; exit }' "$sidecar"
}

is_official_compiler_bridge_artifact() {
    file_path="$1"
    bridge_src="$(bootstrap_bridge_source_for_artifact "$file_path" || true)"
    [ -n "$bridge_src" ] || return 1
    case "$bridge_src" in
        "$COMPILER_ENTRY_POINT"|src/vitte/compiler/main.vit)
            return 0
            ;;
    esac
    return 1
}

enforce_native_artifact_policy() {
    file_path="$1"
    context="$2"
    if ! is_machine_executable "$file_path"; then
        die "$context did not produce a machine executable"
    fi
    if is_bootstrap_bridge_artifact "$file_path"; then
        if [ "$STAGE2_EFFECTIVE_ALLOW_BRIDGE_ARTIFACT" = "1" ]; then
            log "$context produced a bootstrap bridge artifact; accepting transitional native wrapper"
            return 0
        fi
        die "$context produced a bootstrap bridge artifact instead of a real backend executable"
    fi
}

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
        build_stage2_shell_payload "$SRC_VIT" "$OUT_DIR/vittec" || die "stage2 build-native failed"
        ;;
    native)
        log "backend mode=native (experimental)"
        if "$STAGE1_BIN" build "$SRC_VIT" -o "$OUT_DIR/vittec" && [ -x "$OUT_DIR/vittec" ]; then
            enforce_native_artifact_policy "$OUT_DIR/vittec" "native backend"
        elif [ "$STAGE2_EFFECTIVE_FALLBACK" = "1" ]; then
            log "native backend unavailable; explicit fallback -> shell"
            build_stage2_shell_payload "$SRC_VIT" "$OUT_DIR/vittec" || die "stage2 build-native fallback failed"
        else
            die "native backend unavailable and fallback disabled (set VITTE_BACKEND_FALLBACK=1)"
        fi
        ;;
    real-native)
        log "backend mode=real-native"
        if "$STAGE1_BIN" build "$SRC_VIT" -o "$OUT_DIR/vittec" && [ -x "$OUT_DIR/vittec" ]; then
            enforce_native_artifact_policy "$OUT_DIR/vittec" "real native backend"
        else
            die "real native backend unavailable and fallback is forbidden in real-native mode"
        fi
        ;;
    *)
        die "unsupported VITTE_BACKEND_MODE=$VITTE_BACKEND_MODE (expected shell|native|real-native)"
        ;;
esac

VITTEC_BIN="$OUT_DIR/vittec"
[ -x "$VITTEC_BIN" ] || die "final vittec not produced"
if [ "$STAGE2_EFFECTIVE_BACKEND_MODE" = "native" ] && [ "$STAGE2_EFFECTIVE_FALLBACK" != "1" ] && ! is_machine_executable "$VITTEC_BIN"; then
    die "native mode requested but final artifact is not a machine executable"
fi
if [ "$STAGE2_EFFECTIVE_BACKEND_MODE" = "native" ]; then
    enforce_native_artifact_policy "$VITTEC_BIN" "native mode"
fi
log "installing vittec → $BIN_DIR"
cp "$VITTEC_BIN" "$BIN_DIR/vittec"
chmod +x "$BIN_DIR/vittec"
cp "$VITTEC_BIN" "$BIN_DIR/vitte"
chmod +x "$BIN_DIR/vitte"

# ------------------------------------------------------------
# Self-check
# ------------------------------------------------------------

log "verifying final vittec"

"$BIN_DIR/vittec" --version || die "vittec verification failed"
ROOT_OUT="$("$BIN_DIR/vittec" selfhost-source || true)"
echo "$ROOT_OUT" | grep -q "^compiler_source_root=src/vitte/compiler$" || die "vittec selfhost source root mismatch"
echo "$ROOT_OUT" | grep -q "^compiler_entry_point=src/vitte/compiler/main.vit$" || die "vittec selfhost entry point mismatch"

# Optional: self-rebuild check (can be disabled in CI)
if [ "${VITTE_SELF_CHECK:-1}" -eq 1 ]; then
    log "running self-hosting check"

    VERSION_OUT="$("$BIN_DIR/vittec" --version || true)"
    echo "$VERSION_OUT" | grep -q "stage2-vitte" || die "stage2 version identity check failed"

    SELF_DIR="$BUILD_DIR/selfcheck"
    SELF_BIN="$SELF_DIR/vittec"
    mkdir -p "$SELF_DIR"

    if [ "$STAGE2_EFFECTIVE_BACKEND_MODE" = "native" ]; then
        "$BIN_DIR/vittec" build "$SRC_VIT" -o "$SELF_BIN" || die "stage2 native selfcheck build failed"
        enforce_native_artifact_policy "$SELF_BIN" "stage2 native selfcheck"
    else
        build_stage2_shell_payload "$SRC_VIT" "$SELF_BIN" || die "stage2 selfcheck build-native failed"
    fi
    [ -x "$SELF_BIN" ] || die "stage2 selfcheck compiler missing"
    if is_shell_script "$SELF_BIN"; then
        sh -n "$SELF_BIN" || die "stage2 selfcheck compiler is not POSIX shell"
    fi

    SELF_VERSION_OUT="$("$SELF_BIN" --version || true)"
    echo "$SELF_VERSION_OUT" | grep -q "stage2-vitte" || die "stage2 selfcheck version identity failed"
    SELF_ROOT_OUT="$("$SELF_BIN" selfhost-source || true)"
    echo "$SELF_ROOT_OUT" | grep -q "^compiler_source_root=src/vitte/compiler$" || die "stage2 selfcheck source root mismatch"
    echo "$SELF_ROOT_OUT" | grep -q "^compiler_entry_point=src/vitte/compiler/main.vit$" || die "stage2 selfcheck entry point mismatch"
fi

log "stage2 completed successfully"
