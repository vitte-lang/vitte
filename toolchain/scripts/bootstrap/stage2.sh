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

checksum_file() {
    file_path="$1"
    if command -v sha256sum >/dev/null 2>&1; then
        sha256sum "$file_path" | awk '{print $1}'
    elif command -v shasum >/dev/null 2>&1; then
        shasum -a 256 "$file_path" | awk '{print $1}'
    else
        die "no SHA-256 tool available"
    fi
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
STAGE2_PROVENANCE="$OUT_DIR/provenance.json"
STAGE2_SOURCE_MANIFEST="$OUT_DIR/compiler-sources.sha256"

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

STAGE2_CANDIDATE_DIR="$OUT_DIR/.stage1-build"
STAGE2_CANDIDATE="$STAGE2_CANDIDATE_DIR/vittec"
VITTEC_INSTALL_TMP="$BIN_DIR/.vittec.tmp.$$"
VITTE_INSTALL_TMP="$BIN_DIR/.vitte.tmp.$$"
cleanup_stage2_candidate() {
    rm -rf "$STAGE2_CANDIDATE_DIR"
    rm -f "$VITTEC_INSTALL_TMP" "$VITTE_INSTALL_TMP"
}
trap cleanup_stage2_candidate EXIT HUP INT TERM
cleanup_stage2_candidate
mkdir -p "$STAGE2_CANDIDATE_DIR"

install_executable_atomically() {
    source_file="$1"
    destination="$2"
    temporary="$3"
    cp "$source_file" "$temporary"
    chmod +x "$temporary"
    mv "$temporary" "$destination"
}

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
    [ -f "${file_path}.bootstrap-bridge" ] && return 0
    LC_ALL=C grep -a -F "vitte-bootstrap-payload-bridge" "$file_path" >/dev/null 2>&1
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
        build_stage2_shell_payload "$SRC_VIT" "$STAGE2_CANDIDATE" || die "stage2 build-native failed"
        ;;
    native)
        log "backend mode=native (experimental)"
        if "$STAGE1_BIN" build "$SRC_VIT" -o "$STAGE2_CANDIDATE" && [ -x "$STAGE2_CANDIDATE" ]; then
            enforce_native_artifact_policy "$STAGE2_CANDIDATE" "native backend"
        elif [ "$STAGE2_EFFECTIVE_FALLBACK" = "1" ]; then
            log "native backend unavailable; explicit fallback -> shell"
            build_stage2_shell_payload "$SRC_VIT" "$STAGE2_CANDIDATE" || die "stage2 build-native fallback failed"
        else
            die "native backend unavailable and fallback disabled (set VITTE_BACKEND_FALLBACK=1)"
        fi
        ;;
    real-native)
        log "backend mode=real-native"
        if "$STAGE1_BIN" build "$SRC_VIT" -o "$STAGE2_CANDIDATE" && [ -x "$STAGE2_CANDIDATE" ]; then
            enforce_native_artifact_policy "$STAGE2_CANDIDATE" "real native backend"
        else
            die "real native backend unavailable and fallback is forbidden in real-native mode"
        fi
        ;;
    *)
        die "unsupported VITTE_BACKEND_MODE=$VITTE_BACKEND_MODE (expected shell|native|real-native)"
        ;;
esac

rm -f "$OUT_DIR/vittec" "$OUT_DIR/vittec.bootstrap-bridge"
mv "$STAGE2_CANDIDATE" "$OUT_DIR/vittec"
if [ -f "${STAGE2_CANDIDATE}.bootstrap-bridge" ]; then
    mv "${STAGE2_CANDIDATE}.bootstrap-bridge" "$OUT_DIR/vittec.bootstrap-bridge"
fi
cleanup_stage2_candidate

VITTEC_BIN="$OUT_DIR/vittec"
[ -x "$VITTEC_BIN" ] || die "final vittec not produced"
if [ "$STAGE2_EFFECTIVE_BACKEND_MODE" = "native" ] && [ "$STAGE2_EFFECTIVE_FALLBACK" != "1" ] && ! is_machine_executable "$VITTEC_BIN"; then
    die "native mode requested but final artifact is not a machine executable"
fi
if [ "$STAGE2_EFFECTIVE_BACKEND_MODE" = "native" ]; then
    enforce_native_artifact_policy "$VITTEC_BIN" "native mode"
fi

manifest_tmp="${STAGE2_SOURCE_MANIFEST}.tmp.$$"
find "$COMPILER_SOURCE_ROOT" -type f -name '*.vit' -print | LC_ALL=C sort | while IFS= read -r source_file; do
    relative_file="${source_file#"$ROOT_DIR/"}"
    printf '%s  %s\n' "$(checksum_file "$source_file")" "$relative_file"
done > "$manifest_tmp"
mv "$manifest_tmp" "$STAGE2_SOURCE_MANIFEST"

producer_version="$($STAGE1_BIN --version)"
producer_sha="$(checksum_file "$STAGE1_BIN")"
entry_sha="$(checksum_file "$COMPILER_ENTRY_POINT")"
source_manifest_sha="$(checksum_file "$STAGE2_SOURCE_MANIFEST")"
artifact_sha="$(checksum_file "$VITTEC_BIN")"
bridge_artifact=false
if is_bootstrap_bridge_artifact "$VITTEC_BIN"; then
    bridge_artifact=true
fi
provenance_tmp="${STAGE2_PROVENANCE}.tmp.$$"
cat > "$provenance_tmp" <<EOF
{
  "schema": "vitte.bootstrap.stage-provenance",
  "schema_version": "1.0.0",
  "stage": 2,
  "producer": "bin/vittec1",
  "producer_version": "$producer_version",
  "producer_sha256": "$producer_sha",
  "source_root": "src/vitte/compiler",
  "source_entry": "src/vitte/compiler/main.vit",
  "source_entry_sha256": "$entry_sha",
  "source_manifest": "target/bootstrap/stage2/compiler-sources.sha256",
  "source_manifest_sha256": "$source_manifest_sha",
  "command": "bin/vittec1 build src/vitte/compiler/main.vit -o target/bootstrap/stage2/.stage1-build/vittec",
  "backend_mode": "$STAGE2_EFFECTIVE_BACKEND_MODE",
  "bootstrap_bridge": $bridge_artifact,
  "artifact": "target/bootstrap/stage2/vittec",
  "artifact_sha256": "$artifact_sha"
}
EOF
mv "$provenance_tmp" "$STAGE2_PROVENANCE"

log "installing vittec → $BIN_DIR"
install_executable_atomically "$VITTEC_BIN" "$BIN_DIR/vittec" "$VITTEC_INSTALL_TMP"
install_executable_atomically "$VITTEC_BIN" "$BIN_DIR/vitte" "$VITTE_INSTALL_TMP"

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

trap - EXIT HUP INT TERM
log "stage2 completed successfully"
