#!/usr/bin/env bash
#
# ============================================================================
# Vitte workspace – local environment
#
# Role:
#   - Configure local environment variables for the vitte-core workspace.
#   - Make it easy for scripts, VS Code, and terminals to know where the
#     workspace and bootstrap roots are, and optionally adjust PATH.
#
# Usage:
#   - From a shell:
#       source ./scripts/env_local.sh
#   - From other scripts:
#       . "$(dirname "$0")/env_local.sh"
#
# Notes:
#   - This file is meant to be safe to `source` multiple times.
#   - It should NOT perform any build or I/O-heavy action.
#   - Keep this file focused on environment variables only.
# ============================================================================

# Guard: only source, do not execute directly in subshell without effect.
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    echo "[vitte][env] This script is intended to be sourced, not executed directly." >&2
    echo "Usage: source ./scripts/env_local.sh" >&2
    exit 1
fi

# ----------------------------------------------------------------------------
# Resolve workspace root (vitte-core)
# ----------------------------------------------------------------------------

# scripts/ -> workspace root
_vitte_env_this_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
VITTE_WORKSPACE_ROOT="$(cd "${_vitte_env_this_dir}/.." && pwd)"

# Bootstrap root (bootstrap/ directory inside workspace)
VITTE_BOOTSTRAP_ROOT="${VITTE_WORKSPACE_ROOT}/bootstrap"

export VITTE_WORKSPACE_ROOT
export VITTE_BOOTSTRAP_ROOT

# ----------------------------------------------------------------------------
# Optional: local bin directories
#
# You can adjust this section once your Vitte binaries are stabilised
# (for example, vittec, vitte-run, vitte-tools, etc.). For now, we just
# prepare a few common candidate paths.
# ----------------------------------------------------------------------------

_vitte_env_bin_debug="${VITTE_WORKSPACE_ROOT}/target/debug"
_vitte_env_bin_release="${VITTE_WORKSPACE_ROOT}/target/release"
_vitte_env_local_bin="${VITTE_WORKSPACE_ROOT}/.local/bin"

# Helper to prepend a directory to PATH if it exists and is not already there.
_vitte_env_prepend_path() {
    local dir="$1"
    if [[ -d "${dir}" ]]; then
        case ":${PATH}:" in
            *:"${dir}":*)
                # already in PATH
                ;;
            *)
                PATH="${dir}:${PATH}"
                ;;
        esac
    fi
}

_vitte_env_prepend_path "${_vitte_env_bin_debug}"
_vitte_env_prepend_path "${_vitte_env_bin_release}"
_vitte_env_prepend_path "${_vitte_env_local_bin}"

export PATH

# ----------------------------------------------------------------------------
# Optional: Vitte-specific toggles / configuration knobs
#
# These can be used by your tools and scripts. Adjust as you refine
# the behavior of the Vitte toolchain.
# ----------------------------------------------------------------------------

: "${VITTE_EDITION:=2025}"
: "${VITTE_PROFILE:=dev}"
: "${VITTE_BOOTSTRAP_PROFILE:=dev}"

export VITTE_EDITION
export VITTE_PROFILE
export VITTE_BOOTSTRAP_PROFILE

# ----------------------------------------------------------------------------
# Summary (minimal, to avoid noise)
# ----------------------------------------------------------------------------

if [[ -z "${VITTE_ENV_SILENT:-}" ]]; then
    echo "[vitte][env] Workspace root : ${VITTE_WORKSPACE_ROOT}"
    echo "[vitte][env] Bootstrap root : ${VITTE_BOOTSTRAP_ROOT}"
    echo "[vitte][env] Edition        : ${VITTE_EDITION}"
    echo "[vitte][env] Profile        : ${VITTE_PROFILE}"
fi

# Cleanup internal helper variables
unset _vitte_env_this_dir
unset _vitte_env_bin_debug
unset _vitte_env_bin_release
unset _vitte_env_local_bin
unset -f _vitte_env_prepend_path
