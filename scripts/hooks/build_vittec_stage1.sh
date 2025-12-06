#!/usr/bin/env bash
#
# Build hook for vittec-stage1 (Python implementation with type rules)

set -euo pipefail

this_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
VITTE_WORKSPACE_ROOT="$(cd "${this_dir}/../.." && pwd)"
TARGET_ROOT="${VITTE_WORKSPACE_ROOT}/target"

STAGE1_ROOT="${TARGET_ROOT}/bootstrap/stage1"
LOG_DIR="${STAGE1_ROOT}/logs"
LOG_FILE="${LOG_DIR}/stage1.build.log"
STATUS_FILE="${STAGE1_ROOT}/status.txt"
STAGE1_BIN="${STAGE1_ROOT}/vittec-stage1"
MAIN_SYMLINK="${TARGET_ROOT}/debug/vittec"

STAGE1_SRC="${VITTE_WORKSPACE_ROOT}/bootstrap/stage1/vittec_stage1.py"

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
    local env_file="${VITTE_WORKSPACE_ROOT}/scripts/env_local.sh"
    if [[ -f "${env_file}" ]]; then
        # shellcheck disable=SC1090
        . "${env_file}"
    fi
}

require_python() {
    if ! command -v python3 >/dev/null 2>&1; then
        die "python3 is required to build vittec-stage1."
    fi
}

prepare_dirs() {
    mkdir -p "${STAGE1_ROOT}" "${LOG_DIR}"
}

verify_source() {
    [[ -f "${STAGE1_SRC}" ]] || die "Stage1 source not found at ${STAGE1_SRC}"
    python3 -m py_compile "${STAGE1_SRC}" >>"${LOG_FILE}" 2>&1 || die "py_compile failed for ${STAGE1_SRC}"
}

write_stage1_binary() {
    cp "${STAGE1_SRC}" "${STAGE1_BIN}"
    chmod +x "${STAGE1_BIN}"
    log "Copied Python stage1 compiler to ${STAGE1_BIN}"
}

update_status_file() {
    {
        echo "# Vitte bootstrap – stage1 build status"
        echo "workspace_root=${VITTE_WORKSPACE_ROOT}"
        echo "timestamp=$(date '+%Y-%m-%dT%H:%M:%S%z' 2>/dev/null || echo 'unknown')"
        echo "status=ok-python-stage1"
        echo "binary=${STAGE1_BIN}"
        echo "source=${STAGE1_SRC}"
        echo "log=${LOG_FILE}"
    } > "${STATUS_FILE}"
}

link_stage1_as_main() {
    mkdir -p "$(dirname "${MAIN_SYMLINK}")"
    if [[ -L "${MAIN_SYMLINK}" || -e "${MAIN_SYMLINK}" ]]; then
        log "Replacing existing vittec link at ${MAIN_SYMLINK}"
        rm -f "${MAIN_SYMLINK}"
    fi
    ln -s "${STAGE1_BIN}" "${MAIN_SYMLINK}"
    log "Linked stage1 compiler to ${MAIN_SYMLINK}"
}

main() {
    log "Building vittec-stage1 (Python frontend/type checker)…"
    maybe_source_env_local
    require_python
    prepare_dirs
    verify_source
    write_stage1_binary
    update_status_file
    link_stage1_as_main
    log "vittec-stage1 ready at ${STAGE1_BIN}"
    log "Build log stored in ${LOG_FILE}"
}

main "$@"
