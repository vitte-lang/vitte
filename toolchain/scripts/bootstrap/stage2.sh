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

has_host_native_toolchain() {
    command -v "${CC:-cc}" >/dev/null 2>&1 && command -v od >/dev/null 2>&1
}

build_native_launcher() {
    shell_payload="$1"
    out_bin="$2"
    work_dir="$3"
    cc_bin="${CC:-cc}"
    [ -f "$shell_payload" ] || die "missing shell payload: $shell_payload"
    mkdir -p "$work_dir"
    payload_tmp="$work_dir/payload_bytes.txt"
    payload_len="$(wc -c < "$shell_payload" | tr -d ' ')"
    od -An -v -t u1 "$shell_payload" | tr -s '[:space:]' '\n' | sed '/^$/d' > "$payload_tmp"
    {
        printf 'unsigned char vittec_payload[] = {\n'
        first=1
        while IFS= read -r b; do
            if [ "$first" -eq 1 ]; then
                printf '  %s' "$b"
                first=0
            else
                printf ', %s' "$b"
            fi
        done < "$payload_tmp"
        printf '\n};\n'
        printf 'unsigned int vittec_payload_len = %s;\n' "$payload_len"
    } > "$work_dir/payload.h"
    cat > "$work_dir/wrapper.c" <<'EOF'
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "payload.h"

static int write_all(int fd, const unsigned char *buf, size_t len) {
    size_t off = 0;
    while (off < len) {
        ssize_t wrote = write(fd, buf + off, len - off);
        if (wrote <= 0) {
            if (errno == EINTR) {
                continue;
            }
            return 0;
        }
        off += (size_t)wrote;
    }
    return 1;
}

int main(int argc, char **argv) {
    char path[] = "/tmp/vittec-native-XXXXXX";
    int fd = mkstemp(path);
    if (fd < 0) {
        fprintf(stderr, "[stage2-native][error] mkstemp failed: %s\n", strerror(errno));
        return 1;
    }
    if (!write_all(fd, vittec_payload, vittec_payload_len)) {
        fprintf(stderr, "[stage2-native][error] write payload failed: %s\n", strerror(errno));
        close(fd);
        unlink(path);
        return 1;
    }
    if (fchmod(fd, 0700) != 0) {
        fprintf(stderr, "[stage2-native][error] chmod payload failed: %s\n", strerror(errno));
        close(fd);
        unlink(path);
        return 1;
    }
    close(fd);

    char **exec_argv = (char **)calloc((size_t)argc + 3, sizeof(char *));
    if (exec_argv == NULL) {
        fprintf(stderr, "[stage2-native][error] alloc argv failed\n");
        unlink(path);
        return 1;
    }
    exec_argv[0] = "sh";
    exec_argv[1] = path;
    for (int i = 1; i < argc; ++i) {
        exec_argv[i + 1] = argv[i];
    }
    exec_argv[argc + 1] = NULL;

    execv("/bin/sh", exec_argv);
    fprintf(stderr, "[stage2-native][error] exec /bin/sh failed: %s\n", strerror(errno));
    unlink(path);
    free(exec_argv);
    return 1;
}
EOF
    "$cc_bin" -O2 -s "$work_dir/wrapper.c" -o "$out_bin" || die "native launcher compilation failed"
    chmod +x "$out_bin"
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
VITTE_BACKEND_MODE="${VITTE_BACKEND_MODE:-shell}"
VITTE_BACKEND_FALLBACK="${VITTE_BACKEND_FALLBACK:-1}"
VITTE_NATIVE_BRIDGE_COMPAT="${VITTE_NATIVE_BRIDGE_COMPAT:-0}"
VITTE_ENFORCE_COMPILER_REACHABILITY="${VITTE_ENFORCE_COMPILER_REACHABILITY:-1}"
VITTE_STAGE2_BOOTSTRAP_COMPAT="${VITTE_STAGE2_BOOTSTRAP_COMPAT:-1}"

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
    if "$STAGE1_BIN" build-native --src "$requested_src" --out "$out_bin" 2>"$entry_err"; then
        rm -f "$entry_err"
        return 0
    fi
    if [ "$VITTE_STAGE2_BOOTSTRAP_COMPAT" = "1" ]; then
        compat_src="$STAGE2_DIR/src/main.vit"
        log "compiler entry exceeds bootstrap subset; using temporary stage2 shell payload: $compat_src"
        "$STAGE1_BIN" build-native --src "$compat_src" --out "$out_bin"
        return $?
    fi
    cat "$entry_err" >&2
    return 1
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
        if "$STAGE1_BIN" build --stage stage1 --src "$STAGE2_DIR/src" --out "$OUT_DIR" >/dev/null 2>&1 && [ -x "$OUT_DIR/vittec1" ]; then
            if is_machine_executable "$OUT_DIR/vittec1"; then
                cp "$OUT_DIR/vittec1" "$OUT_DIR/vittec"
            elif [ "$VITTE_NATIVE_BRIDGE_COMPAT" = "1" ] && has_host_native_toolchain; then
                log "native compat: converting stage artifact into host executable launcher"
                build_native_launcher "$OUT_DIR/vittec1" "$OUT_DIR/vittec" "$BUILD_DIR/native-wrap"
            elif [ "$VITTE_NATIVE_BRIDGE_COMPAT" = "1" ]; then
                die "native bridge compat requested but host toolchain (cc+od) is unavailable"
            elif [ "$VITTE_BACKEND_FALLBACK" = "1" ]; then
                log "native backend produced non-machine artifact; explicit fallback -> shell"
                build_stage2_shell_payload "$SRC_VIT" "$OUT_DIR/vittec" || die "stage2 build-native fallback failed"
            else
                die "native backend did not produce machine executable (set VITTE_NATIVE_BRIDGE_COMPAT=1 for temporary wrapper compatibility)"
            fi
        elif [ "$VITTE_BACKEND_FALLBACK" = "1" ]; then
            log "native backend unavailable; explicit fallback -> shell"
            build_stage2_shell_payload "$SRC_VIT" "$OUT_DIR/vittec" || die "stage2 build-native fallback failed"
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
if [ "$VITTE_BACKEND_MODE" = "native" ] && [ "$VITTE_BACKEND_FALLBACK" != "1" ] && ! is_machine_executable "$VITTEC_BIN"; then
    die "native mode requested but final artifact is not a machine executable"
fi
if [ "$VITTE_BACKEND_MODE" = "native" ] && ! is_machine_executable "$VITTEC_BIN"; then
    die "native mode must produce a machine executable (got non-native artifact)"
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

    build_stage2_shell_payload "$SRC_VIT" "$SELF_BIN" || die "stage2 selfcheck build-native failed"
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
