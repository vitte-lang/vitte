#!/usr/bin/env bash
#
# ============================================================================
# Vitte workspace – clean script
#
# Role:
#   - Provide a safe, documented cleaning routine for the vitte-core workspace.
#   - Remove generated artifacts (target/, logs, temporary files) without
#     touching source code, manifests, or configuration.
#
# Usage:
#   - From the repository root:
#       ./scripts/clean.sh
#
# Behavior:
#   - Cleans the `target/` directory (build artifacts, caches, logs).
#   - Leaves `.git` data, sources, manifests, and .vscode configs intact.
#   - Can be run multiple times without side effects.
# ============================================================================

set -euo pipefail

# ----------------------------------------------------------------------------
# Resolve workspace root
# ----------------------------------------------------------------------------

this_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
VITTE_WORKSPACE_ROOT="$(cd "${this_dir}/.." && pwd)"

export VITTE_WORKSPACE_ROOT

TARGET_DIR="${VITTE_WORKSPACE_ROOT}/target"

# ----------------------------------------------------------------------------
# Logging helpers
# ----------------------------------------------------------------------------

log_info() {
    printf '[vitte][clean][INFO] %s\n' "$*"
}

log_warn() {
    printf '[vitte][clean][WARN] %s\n' "$*" >&2
}

log_error() {
    printf '[vitte][clean][ERROR] %s\n' "$*" >&2
}

die() {
    log_error "$*"
    exit 1
}

# ----------------------------------------------------------------------------
# Safety checks
# ----------------------------------------------------------------------------

ensure_workspace_root() {
    if [[ ! -f "${VITTE_WORKSPACE_ROOT}/muffin.muf" ]]; then
        die "muffin.muf not found at workspace root (${VITTE_WORKSPACE_ROOT}); aborting clean."
    fi

    if [[ ! -d "${VITTE_WORKSPACE_ROOT}/scripts" ]]; then
        die "scripts/ directory not found at workspace root; aborting clean."
    fi

    log_info "Workspace root looks valid: ${VITTE_WORKSPACE_ROOT}"
}

# ----------------------------------------------------------------------------
# Cleaning routines
# ----------------------------------------------------------------------------

clean_target_dir() {
    if [[ -d "${TARGET_DIR}" ]]; then
        log_info "Cleaning target directory: ${TARGET_DIR}"

        # We remove contents but keep the directory itself present,
        # so that scripts and tools relying on `target/` continue to work.
        find "${TARGET_DIR}" -mindepth 1 -maxdepth 1 -print0 2>/dev/null \
          | xargs -0 rm -rf --

        log_info "target/ cleaned. Leaving the directory itself in place."
    else
        log_info "No target/ directory found at ${TARGET_DIR}; nothing to clean."
        mkdir -p "${TARGET_DIR}"
        log_info "Created empty target/ directory."
    fi
}

clean_misc_temp() {
    log_info "Cleaning miscellaneous temporary files (if any)…"

    # Extend this block as needed when more temporary patterns appear.
    local patterns=(
        "*.tmp"
        "*.swp"
        "*~"
        ".DS_Store"
    )

    # We only scan a limited set of directories to avoid surprises.
    local roots=(
        "${VITTE_WORKSPACE_ROOT}/src"
        "${VITTE_WORKSPACE_ROOT}/tests"
        "${VITTE_WORKSPACE_ROOT}/bootstrap"
    )

    for root in "${roots[@]}"; do
        if [[ -d "${root}" ]]; then
            for pattern in "${patterns[@]}"; do
                # shellcheck disable=SC2044
                for f in $(find "${root}" -name "${pattern}" -type f 2>/dev/null || true); do
                    log_info "Removing temp file: ${f}"
                    rm -f -- "${f}"
                done
            done
        fi
    done

    log_info "Misc temporary cleanup done."
}

# ----------------------------------------------------------------------------
# Summary
# ----------------------------------------------------------------------------

print_summary() {
    log_info "Clean completed."
    log_info "  - Workspace root: ${VITTE_WORKSPACE_ROOT}"
    log_info "  - target/ directory cleaned: ${TARGET_DIR}"
}

# ----------------------------------------------------------------------------
# Main
# ----------------------------------------------------------------------------

main() {
    log_info "Starting Vitte workspace clean…"
    ensure_workspace_root
    clean_target_dir
    clean_misc_temp
    print_summary
}

main "$@"
