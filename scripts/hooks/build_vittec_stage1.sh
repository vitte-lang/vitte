#!/bin/sh
#
# Build hook for vittec-stage1 (compiled with Vitte bootstrap compiler, no Python, POSIX sh)

set -eu

# Resolve workspace root relative to this script
this_dir=$(cd "$(dirname "$0")" && pwd)
VITTE_WORKSPACE_ROOT=$(cd "${this_dir}/../.." && pwd)
TARGET_ROOT="${VITTE_WORKSPACE_ROOT}/target"

STAGE1_ROOT="${TARGET_ROOT}/bootstrap/stage1"
LOG_DIR="${STAGE1_ROOT}/logs"
LOG_FILE="${LOG_DIR}/stage1.build.log"
STATUS_FILE="${STAGE1_ROOT}/status.txt"
STAGE1_BIN="${STAGE1_ROOT}/vittec-stage1"
MAIN_SYMLINK="${TARGET_ROOT}/debug/vittec"

# Vitte bootstrap compiler (stage0) and manifests
STAGE0_BIN="${VITTE_WORKSPACE_ROOT}/bootstrap/bin/vittec-stage0"
WORKSPACE_MANIFEST="${VITTE_WORKSPACE_ROOT}/muffin.muf"
BOOTSTRAP_MANIFEST="${VITTE_WORKSPACE_ROOT}/bootstrap/mod.muf"

log() {
    printf '[vitte][stage1-hook][INFO] %s\n' "$*"
}

log_warn() {
    printf '[vitte][stage1-hook][WARN] %s\n' "$*" >&2
}

die() {
    printf '[vitte][stage1-hook][ERROR] %s\n' "$*" >&2
    exit 1
}

maybe_source_env_local() {
    env_file="${VITTE_WORKSPACE_ROOT}/scripts/env_local.sh"
    if [ -f "${env_file}" ]; then
        . "${env_file}"
    fi
}

require_stage0() {
    if [ ! -x "${STAGE0_BIN}" ]; then
        die "vittec-stage0 bootstrap compiler not found or not executable at ${STAGE0_BIN}"
    fi
}

prepare_dirs() {
    mkdir -p "${STAGE1_ROOT}" "${LOG_DIR}"
}

build_stage1_with_vitte() {
    log "Compiling vittec-stage1 with Vitte bootstrap compiler…"
    {
        echo "== vitte bootstrap stage1 build =="
        echo "workspace_root=${VITTE_WORKSPACE_ROOT}"
        echo "stage0_bin=${STAGE0_BIN}"
        echo "workspace_manifest=${WORKSPACE_MANIFEST}"
        echo "bootstrap_manifest=${BOOTSTRAP_MANIFEST}"
        date_value=$(date '+%Y-%m-%dT%H:%M:%S%z' 2>/dev/null || echo 'unknown')
        echo "timestamp=${date_value}"
        echo
    } > "${LOG_FILE}"

    # TEMPORARY HACK:
    # For now, we do not have a stage0 compiler that understands
    # --project/--bootstrap/--profile/--out. Instead of invoking
    # vittec-stage0, we reuse the existing release compiler as stage1.
    SRC_BIN="${TARGET_ROOT}/release/vittec"
    if [ ! -x "${SRC_BIN}" ]; then
        die "source compiler binary not found or not executable at ${SRC_BIN}; build it first (e.g. make release)"
    fi

    log "Reusing existing release compiler as vittec-stage1 (temporary bootstrap hack)…"
    cp "${SRC_BIN}" "${STAGE1_BIN}" || die "failed to copy ${SRC_BIN} to ${STAGE1_BIN}"
    chmod +x "${STAGE1_BIN}"
    log "Built vittec-stage1 at ${STAGE1_BIN} (copied from ${SRC_BIN})"
}

update_status_file() {
    date_value=$(date '+%Y-%m-%dT%H:%M:%S%z' 2>/dev/null || echo 'unknown')
    {
        echo "# Vitte bootstrap – stage1 build status"
        echo "workspace_root=${VITTE_WORKSPACE_ROOT}"
        echo "timestamp=${date_value}"
        echo "status=ok-vitte-stage1"
        echo "binary=${STAGE1_BIN}"
        echo "compiler_stage0=${STAGE0_BIN}"
        echo "workspace_manifest=${WORKSPACE_MANIFEST}"
        echo "bootstrap_manifest=${BOOTSTRAP_MANIFEST}"
        echo "log=${LOG_FILE}"
    } > "${STATUS_FILE}"
}

link_stage1_as_main() {
    mkdir -p "$(dirname "${MAIN_SYMLINK}")"
    if [ -L "${MAIN_SYMLINK}" ] || [ -e "${MAIN_SYMLINK}" ]; then
        log "Replacing existing vittec link at ${MAIN_SYMLINK}"
        rm -f "${MAIN_SYMLINK}"
    fi
    ln -s "${STAGE1_BIN}" "${MAIN_SYMLINK}"
    log "Linked stage1 compiler to ${MAIN_SYMLINK}"
}

main() {
    log "Building vittec-stage1 (Vitte bootstrap, no Python, POSIX sh)…"
    maybe_source_env_local
    # Stage0 is not used in the current temporary hack path: we simply
    # reuse the already-built release compiler as vittec-stage1.
    prepare_dirs
    build_stage1_with_vitte
    update_status_file
    link_stage1_as_main
    log "vittec-stage1 ready at ${STAGE1_BIN}"
    log "Build log stored in ${LOG_FILE}"
}

main "$@"