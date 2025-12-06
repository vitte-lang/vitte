#!/usr/bin/env bash
#
# ============================================================================
# Vitte bootstrap – stage1 + stage2 (self-host compiler)
#
# Role:
#   - Orchestrate the "middle" phases of the Vitte bootstrap:
#       * stage1: build a first Vitte compiler (vittec-stage1),
#       * stage2: rebuild the compiler with itself (self-host, vittec-stage2),
#       * optionally expose a "main" compiler binary (vittec) in target/.
#   - Rely on:
#       * the host/stage0 setup (scripts/bootstrap_stage0.sh),
#       * local environment hints (scripts/env_local.sh),
#       * optional hook scripts for actual compilation steps.
#
# Notes:
#   - This script is intentionally conservative and toolchain-agnostic:
#       * it does NOT hardcode any specific build system,
#       * it exposes clear hook points for you to wire real Vitte builds,
#       * it always produces logs and small status files.
#
# Usage (from repo root):
#   ./scripts/self_host_stage1.sh
# ============================================================================

set -euo pipefail

# ----------------------------------------------------------------------------
# Resolve workspace roots
# ----------------------------------------------------------------------------

this_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
VITTE_WORKSPACE_ROOT="$(cd "${this_dir}/.." && pwd)"
VITTE_BOOTSTRAP_ROOT="${VITTE_WORKSPACE_ROOT}/bootstrap"

export VITTE_WORKSPACE_ROOT
export VITTE_BOOTSTRAP_ROOT

# ----------------------------------------------------------------------------
# Logging helpers
# ----------------------------------------------------------------------------

log_info() {
    printf '[vitte][stage1+2][INFO] %s\n' "$*"
}

log_warn() {
    printf '[vitte][stage1+2][WARN] %s\n' "$*" >&2
}

log_error() {
    printf '[vitte][stage1+2][ERROR] %s\n' "$*" >&2
}

die() {
    log_error "$*"
    exit 1
}

# ----------------------------------------------------------------------------
# Paths and directories
# ----------------------------------------------------------------------------

TARGET_ROOT="${VITTE_WORKSPACE_ROOT}/target"

STAGE0_ROOT="${TARGET_ROOT}/bootstrap/host"
STAGE1_ROOT="${TARGET_ROOT}/bootstrap/stage1"
STAGE2_ROOT="${TARGET_ROOT}/bootstrap/stage2"

STAGE1_LOG_DIR="${STAGE1_ROOT}/logs"
STAGE2_LOG_DIR="${STAGE2_ROOT}/logs"

mkdir -p "${STAGE1_LOG_DIR}" "${STAGE2_LOG_DIR}"

STAGE1_MAIN_LOG="${STAGE1_LOG_DIR}/stage1.log"
STAGE2_MAIN_LOG="${STAGE2_LOG_DIR}/stage2.log"

# Convenient references for potential binaries/artifacts (purely conventional).
VITTEC_STAGE1_BIN="${TARGET_ROOT}/bootstrap/stage1/vittec-stage1"
VITTEC_STAGE2_BIN="${TARGET_ROOT}/bootstrap/stage2/vittec-stage2"
VITTEC_MAIN_BIN_DEBUG="${TARGET_ROOT}/debug/vittec"
VITTEC_MAIN_BIN_RELEASE="${TARGET_ROOT}/release/vittec"

# ----------------------------------------------------------------------------
# Optional env_local.sh sourcing
# ----------------------------------------------------------------------------

maybe_source_env_local() {
    local env_file="${VITTE_WORKSPACE_ROOT}/scripts/env_local.sh"
    if [[ -f "${env_file}" ]]; then
        log_info "Sourcing env_local.sh from ${env_file}"
        # shellcheck disable=SC1090
        . "${env_file}"
    else
        log_info "No env_local.sh found (optional)."
    fi
}

# ----------------------------------------------------------------------------
# Workspace / bootstrap safety checks
# ----------------------------------------------------------------------------

ensure_workspace_root() {
    if [[ ! -f "${VITTE_WORKSPACE_ROOT}/muffin.muf" ]]; then
        die "muffin.muf not found at workspace root (${VITTE_WORKSPACE_ROOT})."
    fi
    if [[ ! -d "${VITTE_WORKSPACE_ROOT}/bootstrap" ]]; then
        die "bootstrap/ directory not found at workspace root (${VITTE_WORKSPACE_ROOT})."
    fi
    if [[ ! -d "${VITTE_WORKSPACE_ROOT}/src" ]]; then
        die "src/ directory not found at workspace root (${VITTE_WORKSPACE_ROOT})."
    fi
    log_info "Workspace root looks valid: ${VITTE_WORKSPACE_ROOT}"
}

maybe_run_stage0() {
    # If stage0 has not been run, we can invoke it automatically once,
    # unless explicitly disabled.
    local stage0_script="${VITTE_WORKSPACE_ROOT}/scripts/bootstrap_stage0.sh"
    local stage0_log="${STAGE0_ROOT}/logs/stage0.log"

    if [[ "${VITTE_SKIP_STAGE0:-0}" == "1" ]]; then
        log_info "VITTE_SKIP_STAGE0=1 set, skipping automatic stage0 invocation."
        return 0
    fi

    if [[ -f "${stage0_log}" ]]; then
        log_info "Stage0 seems to have been executed already (log: ${stage0_log})."
        return 0
    fi

    if [[ -x "${stage0_script}" ]]; then
        log_info "Stage0 does not appear to have run yet; invoking bootstrap_stage0.sh..."
        ( cd "${VITTE_WORKSPACE_ROOT}" && "${stage0_script}" )
        log_info "Stage0 finished; continuing with stage1+2."
    else
        log_warn "bootstrap_stage0.sh not found or not executable; continuing without stage0."
    fi
}

# ----------------------------------------------------------------------------
# Hooks for actual compilation steps
#
# These hooks are intentionally flexible. You can implement them as separate
# scripts (e.g. scripts/hooks/build_vittec_stage1.sh) or replace the bodies
# below with real commands once your compiler/runtime are in place.
# ----------------------------------------------------------------------------

run_hook_if_exists() {
    # Helper: run a given script if it exists and is executable.
    # Usage: run_hook_if_exists "scripts/hooks/build_vittec_stage1.sh" "description text"
    local hook="$1"
    local desc="$2"

    if [[ -x "${hook}" ]]; then
        log_info "Running hook: ${desc} -> ${hook}"
        ( cd "${VITTE_WORKSPACE_ROOT}" && "${hook}" )
        log_info "Hook finished: ${desc}"
        return 0
    fi

    log_info "No hook found for: ${desc} (${hook}); skipping."
    return 1
}

build_stage1_compiler() {
    log_info "=== Stage1: building initial Vitte compiler (vittec-stage1) ==="

    mkdir -p "${STAGE1_ROOT}"

    local status_file="${STAGE1_ROOT}/status.txt"
    local hook1="${VITTE_WORKSPACE_ROOT}/scripts/hooks/build_vittec_stage1.sh"
    local hook2="${VITTE_WORKSPACE_ROOT}/scripts/hooks/build_stage1_compiler.sh"

    # Try hooks in order. You can rename/remove these later.
    if run_hook_if_exists "${hook1}" "build vittec-stage1 (primary hook)"; then
        :
    elif run_hook_if_exists "${hook2}" "build vittec-stage1 (fallback hook)"; then
        :
    else
        log_warn "No stage1 compiler build hook present. Creating placeholder status file."

        {
            echo "# Vitte bootstrap – stage1 status (placeholder)"
            echo "workspace_root=${VITTE_WORKSPACE_ROOT}"
            echo "bootstrap_root=${VITTE_BOOTSTRAP_ROOT}"
            echo "timestamp=$(date '+%Y-%m-%dT%H:%M:%S%z' 2>/dev/null || echo 'unknown')"
            echo "status=stage1-placeholders-only"
            echo "# Implement scripts/hooks/build_vittec_stage1.sh to perform a real build."
        } > "${status_file}"
    fi

    log_info "Stage1 step completed (logical level)."
}

build_stage2_self_host() {
    log_info "=== Stage2: rebuilding compiler with itself (self-host, vittec-stage2) ==="

    mkdir -p "${STAGE2_ROOT}"

    local status_file="${STAGE2_ROOT}/status.txt"
    local hook1="${VITTE_WORKSPACE_ROOT}/scripts/hooks/build_vittec_stage2.sh"
    local hook2="${VITTE_WORKSPACE_ROOT}/scripts/hooks/build_stage2_compiler.sh"

    if run_hook_if_exists "${hook1}" "build vittec-stage2 (primary hook)"; then
        :
    elif run_hook_if_exists "${hook2}" "build vittec-stage2 (fallback hook)"; then
        :
    else
        log_warn "No stage2 compiler build hook present. Creating placeholder status file."

        {
            echo "# Vitte bootstrap – stage2 status (placeholder)"
            echo "workspace_root=${VITTE_WORKSPACE_ROOT}"
            echo "bootstrap_root=${VITTE_BOOTSTRAP_ROOT}"
            echo "timestamp=$(date '+%Y-%m-%dT%H:%M:%S%z' 2>/dev/null || echo 'unknown')"
            echo "status=stage2-placeholders-only"
            echo "# Implement scripts/hooks/build_vittec_stage2.sh to perform a real self-host build."
        } > "${status_file}"
    fi

    log_info "Stage2 step completed (logical level)."
}

expose_main_vittec_symlink() {
    log_info "=== Exposing main vittec binary (optional) ==="

    # We prefer a stage2-built compiler if available.
    local candidate=""

    if [[ -x "${VITTEC_STAGE2_BIN}" ]]; then
        candidate="${VITTEC_STAGE2_BIN}"
    elif [[ -x "${VITTEC_STAGE1_BIN}" ]]; then
        candidate="${VITTEC_STAGE1_BIN}"
    fi

    if [[ -z "${candidate}" ]]; then
        log_warn "No stage1/stage2 compiler binary found to expose as 'vittec'."
        log_warn "You can adjust this logic once your build hooks produce real binaries."
        return 0
    fi

    mkdir -p "$(dirname "${VITTEC_MAIN_BIN_DEBUG}")"

    if [[ -L "${VITTEC_MAIN_BIN_DEBUG}" || -e "${VITTEC_MAIN_BIN_DEBUG}" ]]; then
        log_info "Removing existing vittec debug entry: ${VITTEC_MAIN_BIN_DEBUG}"
        rm -f "${VITTEC_MAIN_BIN_DEBUG}"
    fi

    ln -s "${candidate}" "${VITTEC_MAIN_BIN_DEBUG}"
    log_info "Linked main debug compiler to: ${VITTEC_MAIN_BIN_DEBUG} -> ${candidate}"
}

# ----------------------------------------------------------------------------
# Summary
# ----------------------------------------------------------------------------

print_summary() {
    log_info "Stage1+2 bootstrap sequence completed (logical level)."
    log_info "  - Workspace root : ${VITTE_WORKSPACE_ROOT}"
    log_info "  - Bootstrap root : ${VITTE_BOOTSTRAP_ROOT}"
    log_info "  - Stage0 root    : ${STAGE0_ROOT}"
    log_info "  - Stage1 root    : ${STAGE1_ROOT}"
    log_info "  - Stage2 root    : ${STAGE2_ROOT}"
    log_info "  - Candidate vittec (debug): ${VITTEC_MAIN_BIN_DEBUG}"
}

# ----------------------------------------------------------------------------
# Main
# ----------------------------------------------------------------------------

main() {
    log_info "Starting Vitte bootstrap stage1+2 (self-host compiler)…"
    log_info "Workspace root: ${VITTE_WORKSPACE_ROOT}"
    log_info "Bootstrap root: ${VITTE_BOOTSTRAP_ROOT}"

    maybe_source_env_local
    ensure_workspace_root
    maybe_run_stage0

    # Stage1 and Stage2 are logged separately for easier inspection.
    {
        build_stage1_compiler
    } | tee "${STAGE1_MAIN_LOG}"

    {
        build_stage2_self_host
        expose_main_vittec_symlink
    } | tee "${STAGE2_MAIN_LOG}"

    print_summary
}

main "$@"
