#!/bin/sh
#
# vittec-stage1 – lanceur du compilateur Vitte (stage1)
# 100 % POSIX sh, aucun Python.

set -eu

# --------------------------------------------------------------------
# Résolution du workspace root
# --------------------------------------------------------------------

this_dir=$(cd "$(dirname "$0")" && pwd)

# Si VITTE_WORKSPACE_ROOT est défini, on le respecte.
# Sinon on remonte deux niveaux depuis bootstrap/stage1/
#   <root>/bootstrap/stage1/vittec-stage1.sh
if [ "${VITTE_WORKSPACE_ROOT-}" != "" ]; then
    VITTE_ROOT="${VITTE_WORKSPACE_ROOT}"
else
    VITTE_ROOT=$(cd "${this_dir}/../.." && pwd)
fi

STAGE1_BIN="${VITTE_ROOT}/target/bootstrap/stage1/vittec-stage1"

# --------------------------------------------------------------------
# Helpers
# --------------------------------------------------------------------

log_info() {
    printf '[vittec-stage1][sh][INFO] %s\n' "$*" >&2
}

log_error() {
    printf '[vittec-stage1][sh][ERROR] %s\n' "$*" >&2
}

die() {
    log_error "$*"
    exit 1
}

# --------------------------------------------------------------------
# Main
# --------------------------------------------------------------------

log_info "workspace root: ${VITTE_ROOT}"
log_info "stage1 binary: ${STAGE1_BIN}"

if [ ! -x "${STAGE1_BIN}" ]; then
    log_error "vittec-stage1 binary not found or not executable:"
    log_error "  ${STAGE1_BIN}"
    log_info "hint: from the workspace root, run:"
    log_info "  ./scripts/hooks/build_vittec_stage1.sh"
    die "cannot continue without a compiled stage1 binary"
fi

# Remplace ce process par le vrai compilateur.
exec "${STAGE1_BIN}" "$@"