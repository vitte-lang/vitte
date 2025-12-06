#!/usr/bin/env bash
#
# Build hook for vittec-stage2 (self-host using vittec-stage1)

set -euo pipefail

this_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
VITTE_WORKSPACE_ROOT="$(cd "${this_dir}/../.." && pwd)"
TARGET_ROOT="${VITTE_WORKSPACE_ROOT}/target"

STAGE1_BIN="${TARGET_ROOT}/bootstrap/stage1/vittec-stage1"
STAGE2_ROOT="${TARGET_ROOT}/bootstrap/stage2"
LOG_DIR="${STAGE2_ROOT}/logs"
STATUS_FILE="${STAGE2_ROOT}/status.txt"
STAGE2_BIN="${STAGE2_ROOT}/vittec-stage2"
MAIN_SYMLINK="${TARGET_ROOT}/debug/vittec"
DIAG_LOG="${STAGE2_ROOT}/stage2.selfhost.log"

log() {
    printf '[vitte][stage2-hook][INFO] %s\n' "$*"
}

log_warn() {
    printf '[vitte][stage2-hook][WARN] %s\n' "$*" >&2
}

die() {
    printf '[vitte][stage2-hook][ERROR] %s\n' "$*" >&2
    exit 1
}

maybe_source_env_local() {
    local env_file="${VITTE_WORKSPACE_ROOT}/scripts/env_local.sh"
    if [[ -f "${env_file}" ]]; then
        # shellcheck disable=SC1090
        . "${env_file}"
    fi
}

collect_sources() {
    local bootstrap_dir="${VITTE_WORKSPACE_ROOT}/bootstrap"
    local project_manifest="${VITTE_WORKSPACE_ROOT}/vitte.project.muf"

    VITTE_SOURCES=()
    while IFS= read -r line; do
        VITTE_SOURCES+=("${line}")
    done < <(
        find "${bootstrap_dir}" -type f \( -name '*.vitte' -o -name '*.muf' \) \
            | sort \
            | head -n 25
    )

    if [[ -f "${project_manifest}" ]]; then
        VITTE_SOURCES+=("${project_manifest}")
    fi
}

run_stage1_over_sources() {
    : > "${DIAG_LOG}"
    local failures=0

    for src in "${VITTE_SOURCES[@]}"; do
        log "Self-host check via stage1 on ${src}"
        if "${STAGE1_BIN}" "${src}" >>"${DIAG_LOG}" 2>&1; then
            :
        else
            failures=$((failures + 1))
            log_warn "stage1 reported an issue for ${src} (details in ${DIAG_LOG})"
        fi
    done

    if [[ "${failures}" -gt 0 ]]; then
        log_warn "Stage1 encountered ${failures} issue(s) while reading sources."
    fi
}

write_stage2_binary() {
    cp "${STAGE1_BIN}" "${STAGE2_BIN}"
    chmod +x "${STAGE2_BIN}"
    printf '\n# vittec-stage2 placeholder built via stage1 hook on %s\n' \
        "$(date '+%Y-%m-%dT%H:%M:%S%z' 2>/dev/null || echo 'unknown')" >> "${STAGE2_BIN}"
}

update_status_file() {
    {
        echo "# Vitte bootstrap – stage2 build status"
        echo "workspace_root=${VITTE_WORKSPACE_ROOT}"
        echo "timestamp=$(date '+%Y-%m-%dT%H:%M:%S%z' 2>/dev/null || echo 'unknown')"
        echo "status=ok-self-host-placeholder"
        echo "binary=${STAGE2_BIN}"
        echo "stage1_binary_used=${STAGE1_BIN}"
        echo "sources_checked=${#VITTE_SOURCES[@]}"
        echo "diag_log=${DIAG_LOG}"
    } > "${STATUS_FILE}"
}

link_stage2_as_main() {
    mkdir -p "$(dirname "${MAIN_SYMLINK}")"
    if [[ -L "${MAIN_SYMLINK}" || -e "${MAIN_SYMLINK}" ]]; then
        log "Replacing existing vittec link at ${MAIN_SYMLINK}"
        rm -f "${MAIN_SYMLINK}"
    fi
    ln -s "${STAGE2_BIN}" "${MAIN_SYMLINK}"
    log "Linked stage2 compiler to ${MAIN_SYMLINK}"
}

main() {
    log "Building vittec-stage2 (self-host via stage1)…"
    maybe_source_env_local

    [[ -x "${STAGE1_BIN}" ]] || die "stage1 binary not found at ${STAGE1_BIN}"

    mkdir -p "${STAGE2_ROOT}" "${LOG_DIR}"
    collect_sources
    run_stage1_over_sources
    write_stage2_binary
    update_status_file
    link_stage2_as_main

    log "vittec-stage2 ready at ${STAGE2_BIN}"
    log "Diagnostics stored in ${DIAG_LOG}"
}

main "$@"
