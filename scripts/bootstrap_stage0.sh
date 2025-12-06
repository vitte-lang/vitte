#!/usr/bin/env bash
#
# ============================================================================
# Vitte bootstrap – stage0 (host layer)
#
# Role:
#   - Prepare the host-side environment for the Vitte bootstrap.
#   - Run initial checks (workspace layout, environment variables, tools).
#   - Optionally invoke host-side helpers (vittec0, host-test-runner, etc.).
#   - Produce basic logs under target/bootstrap/host/.
#
# Notes:
#   - This script is intentionally conservative: it does NOT enforce any
#     particular host toolchain. It only performs generic checks and leaves
#     concrete build steps for later integration.
#   - It is designed to be invoked from the repository root as:
#         ./scripts/bootstrap_stage0.sh
#     but will also work when called from within .vscode tasks.
# ============================================================================

set -euo pipefail

# ----------------------------------------------------------------------------
# Resolve workspace roots
# ----------------------------------------------------------------------------

this_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# scripts/ -> repo root
VITTE_WORKSPACE_ROOT="$(cd "${this_dir}/.." && pwd)"
VITTE_BOOTSTRAP_ROOT="${VITTE_WORKSPACE_ROOT}/bootstrap"

export VITTE_WORKSPACE_ROOT
export VITTE_BOOTSTRAP_ROOT

# ----------------------------------------------------------------------------
# Logging helpers
# ----------------------------------------------------------------------------

log_info() {
    printf '[vitte][stage0][INFO] %s\n' "$*"
}

log_warn() {
    printf '[vitte][stage0][WARN] %s\n' "$*" >&2
}

log_error() {
    printf '[vitte][stage0][ERROR] %s\n' "$*" >&2
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
STAGE0_LOG_DIR="${STAGE0_ROOT}/logs"
STAGE0_REPORT_DIR_GRAMMAR="${TARGET_ROOT}/bootstrap/host/grammar"
STAGE0_REPORT_DIR_SAMPLES="${TARGET_ROOT}/bootstrap/host/samples"

mkdir -p "${STAGE0_LOG_DIR}" \
         "${STAGE0_REPORT_DIR_GRAMMAR}" \
         "${STAGE0_REPORT_DIR_SAMPLES}"

STAGE0_MAIN_LOG="${STAGE0_LOG_DIR}/stage0.log"

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
# Generic host checks (minimal, language-agnostic)
# ----------------------------------------------------------------------------

check_workspace_layout() {
    log_info "Checking workspace layout..."

    [[ -f "${VITTE_WORKSPACE_ROOT}/muffin.muf" ]] || \
        die "Missing root muffin.muf at workspace root: ${VITTE_WORKSPACE_ROOT}"

    [[ -d "${VITTE_WORKSPACE_ROOT}/bootstrap" ]] || \
        die "Missing bootstrap/ directory at workspace root."

    [[ -d "${VITTE_WORKSPACE_ROOT}/src" ]] || \
        die "Missing src/ directory at workspace root."

    [[ -d "${VITTE_WORKSPACE_ROOT}/tests" ]] || \
        log_warn "Missing tests/ directory (not fatal at stage0)."

    log_info "Workspace layout OK."
}

check_manifest_presence() {
    log_info "Checking bootstrap manifests..."

    local manifests=(
        "bootstrap/mod.muf"
        "bootstrap/host/mod.muf"
        "bootstrap/front/mod.muf"
        "bootstrap/middle/mod.muf"
        "bootstrap/back/mod.muf"
        "bootstrap/cli/mod.muf"
        "bootstrap/core/mod.muf"
        "bootstrap/pipeline/mod.muf"
        "src/std/mod.muf"
    )

    local missing=0
    for rel in "${manifests[@]}"; do
        local path="${VITTE_WORKSPACE_ROOT}/${rel}"
        if [[ ! -f "${path}" ]]; then
            log_warn "Missing manifest: ${rel}"
            missing=1
        fi
    done

    if [[ "${missing}" -ne 0 ]]; then
        log_warn "Some bootstrap/std manifests are missing. Stage0 will continue but you should fix this."
    else
        log_info "All listed manifests are present."
    fi
}

check_basic_host_tools() {
    # Intentionally minimal and neutral: no hard dependency on a specific
    # compiler or build system here. We just check for a POSIX shell and git.
    log_info "Checking basic host tools..."

    if ! command -v git >/dev/null 2>&1; then
        log_warn "git is not available. Some workflows may rely on git metadata."
    fi

    if ! command -v bash >/dev/null 2>&1 && ! command -v zsh >/dev/null 2>&1; then
        log_warn "Neither bash nor zsh is available in PATH. Scripts may fail."
    fi

    log_info "Basic host tool checks done."
}

# ----------------------------------------------------------------------------
# Lightweight grammar validation helpers
# ----------------------------------------------------------------------------

validate_ebnf_file() {
    # Best-effort validation for the EBNF file:
    #   - ensure the file is not empty,
    #   - require at least one "::=" production,
    #   - check balanced (), [], {} delimiters.
    local ebnf_path="$1"
    local status="skipped"
    local message="python3-not-found"

    if ! command -v python3 >/dev/null 2>&1; then
        printf '%s|%s\n' "${status}" "${message}"
        return 0
    fi

    local output
    if output="$(python3 - "$ebnf_path" <<'PY' 2>&1)"; then
import sys
from pathlib import Path
import re

path = Path(sys.argv[1])
if not path.exists():
    sys.stderr.write(f"file-not-found:{path}")
    sys.exit(1)

text = path.read_text(encoding="utf-8")
if not text.strip():
    sys.stderr.write("empty-file")
    sys.exit(1)

rules = [line for line in text.splitlines() if "::=" in line]
if not rules:
    sys.stderr.write("no-productions-found")
    sys.exit(1)

pairs = {"(": ")", "[": "]", "{": "}"}
opening = set(pairs.keys())
closing = set(pairs.values())
stack = []

for idx, ch in enumerate(text):
    if ch in opening:
        stack.append((ch, idx))
    elif ch in closing:
        if not stack:
            sys.stderr.write(f"unbalanced-closing:{ch}@{idx}")
            sys.exit(1)
        last, pos = stack.pop()
        if pairs[last] != ch:
            sys.stderr.write(f"mismatched-delimiter:{last}->{ch}@{idx}")
            sys.exit(1)

if stack:
    last, pos = stack.pop()
    sys.stderr.write(f"unclosed-delimiter:{last}@{pos}")
    sys.exit(1)

sample = "; ".join(re.sub(r"\\s+", " ", r.strip()) for r in rules[:8])
sys.stdout.write(f"ok:productions={len(rules)};sample={sample}")
PY
    then
        status="ok"
        message="${output}"
    else
        status="failed"
        message="${output}"
    fi

    printf '%s|%s\n' "${status}" "${message}"
}

# ----------------------------------------------------------------------------
# Host-level reports (grammar, samples) – placeholder implementations
# ----------------------------------------------------------------------------

run_grammar_report() {
    log_info "Running grammar validation..."

    local report_file="${STAGE0_REPORT_DIR_GRAMMAR}/report.txt"
    local grammar_dir="${VITTE_WORKSPACE_ROOT}/grammar"
    local ebnf_file="${grammar_dir}/vitte.ebnf"
    local pest_file="${grammar_dir}/vitte.pest"

    local grammar_files
    grammar_files="$(find "${grammar_dir}" -maxdepth 1 -type f \( -name '*.ebnf' -o -name '*.pest' \) 2>/dev/null | sort || true)"

    local ebnf_status="missing"
    local ebnf_message="grammar/vitte.ebnf not found"
    if [[ -f "${ebnf_file}" ]]; then
        local validation_raw
        set +e
        validation_raw="$(validate_ebnf_file "${ebnf_file}" || true)"
        set -e
        ebnf_status="${validation_raw%%|*}"
        ebnf_message="${validation_raw#*|}"
    fi

    local pest_status="absent"
    if [[ -f "${pest_file}" ]]; then
        pest_status="present"
    fi

    {
        echo "# Vitte grammar report (stage0)"
        echo "#"
        echo "# Generated by scripts/bootstrap_stage0.sh."
        echo "# Validates presence and a few structural properties of the grammar files."
        echo
        echo "workspace_root=${VITTE_WORKSPACE_ROOT}"
        echo "bootstrap_root=${VITTE_BOOTSTRAP_ROOT}"
        echo "timestamp=$(date '+%Y-%m-%dT%H:%M:%S%z' 2>/dev/null || echo 'unknown')"
        echo "grammar_dir=${grammar_dir}"
        echo "ebnf_path=${ebnf_file}"
        echo "ebnf_validation_status=${ebnf_status}"
        echo "ebnf_validation_details=${ebnf_message}"
        echo "pest_file_status=${pest_status}"
        echo "files_detected=$(printf '%s\n' "${grammar_files}" | sed '/^$/d' | wc -l | tr -d ' ')"
        if [[ -n "${grammar_files}" ]]; then
            echo "files_list_start"
            printf '%s\n' "${grammar_files}"
            echo "files_list_end"
        fi
    } > "${report_file}"

    log_info "Grammar report written to ${report_file} (status: ${ebnf_status})"
}

run_samples_report() {
    log_info "Running samples/fixtures validation..."

    local report_file="${STAGE0_REPORT_DIR_SAMPLES}/report.txt"
    local tests_root="${VITTE_WORKSPACE_ROOT}/tests"

    local sample_roots=(
        "${tests_root}/data"
        "${tests_root}/smoke"
        "${tests_root}/mini_project"
        "${tests_root}/malformed"
    )

    local total_files=0
    local missing_roots=()
    local lines=()

    for dir in "${sample_roots[@]}"; do
        if [[ -d "${dir}" ]]; then
            local count
            count=$(find "${dir}" -type f 2>/dev/null | wc -l | tr -d ' ')
            total_files=$((total_files + count))
            lines+=("dir=${dir}|files=${count}")
        else
            missing_roots+=("${dir}")
            lines+=("dir=${dir}|files=missing")
        fi
    done

    local mini_project_manifest="${tests_root}/data/mini_project/muffin.muf"
    local mini_project_status="missing"
    if [[ -f "${mini_project_manifest}" ]]; then
        mini_project_status="present"
    fi

    {
        echo "# Vitte samples report (stage0)"
        echo "#"
        echo "# Generated by scripts/bootstrap_stage0.sh."
        echo "# Provides a lightweight inventory of fixtures and sample manifests."
        echo
        echo "workspace_root=${VITTE_WORKSPACE_ROOT}"
        echo "bootstrap_root=${VITTE_BOOTSTRAP_ROOT}"
        echo "timestamp=$(date '+%Y-%m-%dT%H:%M:%S%z' 2>/dev/null || echo 'unknown')"
        echo "tests_root=${tests_root}"
        echo "total_files=${total_files}"
        echo "mini_project_manifest_status=${mini_project_status}"
        if [[ "${#missing_roots[@]}" -gt 0 ]]; then
            echo "missing_roots_start"
            printf '%s\n' "${missing_roots[@]}"
            echo "missing_roots_end"
        fi
        echo "directories_overview_start"
        printf '%s\n' "${lines[@]}"
        echo "directories_overview_end"
        echo "status=ok"
    } > "${report_file}"

    log_info "Samples report written to ${report_file} (total files: ${total_files})"
}

# ----------------------------------------------------------------------------
# Optional host binaries – vittec0, host-test-runner, host-tools-bundle
# ----------------------------------------------------------------------------

run_optional_host_binaries() {
    log_info "Checking for optional host binaries (vittec0, host-test-runner, host-tools-bundle)..."

    local vittec0="${VITTE_WORKSPACE_ROOT}/bootstrap/host/vittec0"
    local host_test_runner="${VITTE_WORKSPACE_ROOT}/bootstrap/host/test_runner/host-test-runner"
    local host_tools_bundle="${VITTE_WORKSPACE_ROOT}/bootstrap/host/tools/host-tools-bundle"

    if [[ -x "${vittec0}" ]]; then
        log_info "Found vittec0 at ${vittec0} (not executed automatically)."
    else
        log_info "vittec0 not found (optional at stage0)."
    fi

    if [[ -x "${host_test_runner}" ]]; then
        log_info "Found host-test-runner at ${host_test_runner} (not executed automatically)."
    else
        log_info "host-test-runner not found (optional at stage0)."
    fi

    if [[ -x "${host_tools_bundle}" ]]; then
        log_info "Found host-tools-bundle at ${host_tools_bundle} (not executed automatically)."
    else
        log_info "host-tools-bundle not found (optional at stage0)."
    fi

    log_info "You can wire these binaries into this script later once they exist."
}

# ----------------------------------------------------------------------------
# Summary
# ----------------------------------------------------------------------------

print_summary() {
    log_info "Stage0 (host) bootstrap completed."
    log_info "  - Workspace root: ${VITTE_WORKSPACE_ROOT}"
    log_info "  - Bootstrap root: ${VITTE_BOOTSTRAP_ROOT}"
    log_info "  - Logs directory: ${STAGE0_LOG_DIR}"
    log_info "  - Grammar report: ${STAGE0_REPORT_DIR_GRAMMAR}/report.txt"
    log_info "  - Samples report: ${STAGE0_REPORT_DIR_SAMPLES}/report.txt"
}

# ----------------------------------------------------------------------------
# Main
# ----------------------------------------------------------------------------

main() {
    log_info "Starting Vitte bootstrap stage0 (host layer)…"
    log_info "Workspace root: ${VITTE_WORKSPACE_ROOT}"
    log_info "Bootstrap root: ${VITTE_BOOTSTRAP_ROOT}"
    log_info "Logging to: ${STAGE0_MAIN_LOG}"

    maybe_source_env_local
    check_workspace_layout
    check_manifest_presence
    check_basic_host_tools

    run_grammar_report
    run_samples_report
    run_optional_host_binaries

    print_summary
}

# Run main and tee to the main stage0 log.
main "$@" | tee "${STAGE0_MAIN_LOG}"
