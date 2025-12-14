#!/bin/sh
#
# Build hook for vittec-stage1 (self-host bootstrap, POSIX sh)

set -eu

this_dir=$(cd "$(dirname "$0")" && pwd)
VITTE_WORKSPACE_ROOT=$(cd "${this_dir}/../.." && pwd)
TARGET_ROOT="${VITTE_WORKSPACE_ROOT}/target"

STAGE1_ROOT="${TARGET_ROOT}/bootstrap/stage1"
LOG_DIR="${STAGE1_ROOT}/logs"
LOG_FILE="${LOG_DIR}/stage1.build.log"
STATUS_FILE="${STAGE1_ROOT}/status.txt"
STAGE1_BIN="${STAGE1_ROOT}/vittec-stage1"
MAIN_SYMLINK="${TARGET_ROOT}/debug/vittec"

STAGE0_BIN="${VITTE_WORKSPACE_ROOT}/bootstrap/bin/vittec-stage0"
WORKSPACE_MANIFEST="${VITTE_WORKSPACE_ROOT}/muffin.muf"
BOOTSTRAP_MANIFEST="${VITTE_WORKSPACE_ROOT}/bootstrap/mod.muf"
PROJECT_MANIFEST="${VITTE_WORKSPACE_ROOT}/vitte.project.muf"
SHIMS_DIR="${VITTE_WORKSPACE_ROOT}/scripts/shims"
STUB_STAGE1_DRIVER="${VITTE_WORKSPACE_ROOT}/bootstrap/stage1/vittec-stage1.sh"
LOG_SNIPPET_LINES=50

if [ -d "${SHIMS_DIR}" ]; then
    PATH="${SHIMS_DIR}:${PATH}"
    export PATH
fi

if [ -z "${PYTHONIOENCODING:-}" ]; then
    export PYTHONIOENCODING="utf-8"
fi

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

dump_log_snippet() {
    if [ ! -f "${LOG_FILE}" ]; then
        log_warn "No stage1 log found at ${LOG_FILE}; nothing to dump"
        return
    fi
    {
        printf '[vitte][stage1-hook][ERROR] First %s lines of %s:\n' "${LOG_SNIPPET_LINES}" "${LOG_FILE}"
        head -n "${LOG_SNIPPET_LINES}" "${LOG_FILE}"
    } >&2
}

install_stub_stage1_driver() {
    if [ ! -f "${STUB_STAGE1_DRIVER}" ]; then
        log_warn "Stub vittec-stage1 driver missing at ${STUB_STAGE1_DRIVER}; downstream smoke tests may fail"
        return 1
    fi
    cp -f "${STUB_STAGE1_DRIVER}" "${STAGE1_BIN}"
    chmod +x "${STAGE1_BIN}"
    log_warn "Installed stub vittec-stage1 driver at ${STAGE1_BIN}"
    return 0
}

maybe_source_env_local() {
    env_file="${VITTE_WORKSPACE_ROOT}/scripts/env_local.sh"
    if [ -f "${env_file}" ]; then
        # shellcheck disable=SC1090
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

write_status_file() {
    date_value=$(date '+%Y-%m-%dT%H:%M:%S%z' 2>/dev/null || echo 'unknown')
    {
        echo "# Vitte bootstrap - stage1 build status"
        echo "workspace_root=${VITTE_WORKSPACE_ROOT}"
        echo "timestamp=${date_value}"
        echo "status=ok-vitte-stage1"
        echo "binary=${STAGE1_BIN}"
        echo "compiler_stage0=${STAGE0_BIN}"
        echo "workspace_manifest=${WORKSPACE_MANIFEST}"
        echo "bootstrap_manifest=${BOOTSTRAP_MANIFEST}"
        echo "project_manifest=${PROJECT_MANIFEST}"
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

build_stage1_with_stage0() {
    require_stage0
    log "Compiling vittec-stage1 with ${STAGE0_BIN}"
    {
        echo "== vitte bootstrap stage1 build =="
        echo "workspace_root=${VITTE_WORKSPACE_ROOT}"
        echo "stage0_bin=${STAGE0_BIN}"
        echo "project_manifest=${PROJECT_MANIFEST}"
        echo "workspace_manifest=${WORKSPACE_MANIFEST}"
        echo "bootstrap_manifest=${BOOTSTRAP_MANIFEST}"
        date_value=$(date '+%Y-%m-%dT%H:%M:%S%z' 2>/dev/null || echo 'unknown')
        echo "timestamp=${date_value}"
        echo
    } > "${LOG_FILE}"

    if ! "${STAGE0_BIN}" build \
        --project "${PROJECT_MANIFEST}" \
        --out-bin "${STAGE1_BIN}" \
        --log-file "${LOG_FILE}" >> "${LOG_FILE}" 2>&1; then
        log_warn "vittec-stage1 build failed, see ${LOG_FILE}"
        install_stub_stage1_driver || true
        die "stage1 build failed"
    fi
    chmod +x "${STAGE1_BIN}"
    log "vittec-stage1 binary created at ${STAGE1_BIN}"
    log "Stage1 build log stored at ${LOG_FILE}"
}

on_exit() {
    status=$1
    if [ "${status}" -ne 0 ]; then
        log_warn "Stage1 hook failed (exit ${status}); dumping log snippet"
        dump_log_snippet
    fi
}

main() {
    log "Building vittec-stage1 via stage0 compiler"
    maybe_source_env_local
    prepare_dirs
    build_stage1_with_stage0
    write_status_file
    link_stage1_as_main
    log "vittec-stage1 ready at ${STAGE1_BIN}"
}

trap 'status=$?; on_exit "$status"' EXIT

main "$@"
