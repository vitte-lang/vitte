#!/bin/bash
# scripts/build/bootstrap.sh - Run the Vitte bootstrap pipeline.
#
# This helper is invoked by `make bootstrap`. It expects a working bytecode
# runner plus a compiled `vittec.vbc` compiler artifact and simply drives the
# bootstrap entrypoint (`bootstrap/mod.vitte`). The script is intentionally
# small so it can be run on fresh workspaces where the native toolchain is not
# yet rebuilt.

set -euo pipefail

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
OUT_DIR="${PROJECT_ROOT}/out"
LOG_DIR="${OUT_DIR}"
mkdir -p "${LOG_DIR}"

BOOTSTRAP_ENTRY="${PROJECT_ROOT}/bootstrap/mod.vitte"
COMPILER_BC="${OUT_DIR}/vittec.vbc"
RUNNER_BIN=""

# Allow overrides for CI / local tweaks.
PROFILE="${PROFILE:-release}"
detect_jobs() {
    if command -v sysctl >/dev/null 2>&1; then
        sysctl -n hw.ncpu 2>/dev/null && return
    fi
    if command -v nproc >/dev/null 2>&1; then
        nproc && return
    fi
    echo 1
}
JOBS="${JOBS:-$(detect_jobs)}"
TARGET_IMG="${TARGET_IMG:-${OUT_DIR}/bootstrap.img}"
ROOTS="${BOOTSTRAP_ROOTS:-${PROJECT_ROOT}/language/examples}"
EXTRA_ARGS=("$@")

log() {
    echo "[bootstrap] $*"
}

die() {
    echo "[bootstrap] ERROR: $*" >&2
    exit 1
}

discover_runner() {
    if [ -x "${OUT_DIR}/vbc-run" ]; then
        RUNNER_BIN="${OUT_DIR}/vbc-run"
    elif [ -x "${PROJECT_ROOT}/bin/vbc-run" ]; then
        RUNNER_BIN="${PROJECT_ROOT}/bin/vbc-run"
    else
        die "vbc-run introuvable. Lancez 'make build' pour générer out/vbc-run ou fournissez un runner natif."
    fi
}

validate_inputs() {
    [ -f "${BOOTSTRAP_ENTRY}" ] || die "Entrée bootstrap absente: ${BOOTSTRAP_ENTRY}"
    [ -f "${COMPILER_BC}" ] || die "Compiler bytecode introuvable: ${COMPILER_BC}. Lancez 'make build' d'abord."

    discover_runner
}

run_bootstrap() {
    local log_file="${LOG_DIR}/bootstrap.log"
    log "Runner : ${RUNNER_BIN}"
    log "Compiler: ${COMPILER_BC}"
    log "Entrée : ${BOOTSTRAP_ENTRY}"
    log "Profil : ${PROFILE} | Jobs: ${JOBS}"
    log "Target : ${TARGET_IMG}"
    log "Racines: ${ROOTS}"
    log "Log     : ${log_file}"

    set -x
    "${RUNNER_BIN}" "${COMPILER_BC}" "${BOOTSTRAP_ENTRY}" \
        --profile "${PROFILE}" \
        --jobs "${JOBS}" \
        --target "${TARGET_IMG}" \
        --root "${ROOTS}" \
        "${EXTRA_ARGS[@]}" \
        | tee "${log_file}"
    set +x
}

validate_inputs
run_bootstrap
