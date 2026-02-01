

#!/usr/bin/env bash
# =========================================================
# Vitte — Global Bootstrap Script (ULTRA-MAX)
# =========================================================
# Rôle :
# - point d’entrée unique pour initialiser le projet Vitte
# - prépare l’environnement (Python, paths, permissions)
# - appelle build.sh et test.sh
# - utilisable en local comme en CI
#
# Usage :
#   ./bootstrap.sh
#   ./bootstrap.sh init
#   ./bootstrap.sh build
#   ./bootstrap.sh test
# =========================================================

set -euo pipefail

# ---------------------------------------------------------
# Configuration globale
# ---------------------------------------------------------

ROOT_DIR="$(cd "$(dirname "$0")" && pwd)"
PYTHON="${PYTHON:-python3}"

BUILD_SCRIPT="$ROOT_DIR/build.sh"
TEST_SCRIPT="$ROOT_DIR/test.sh"

COLOR_RESET="\033[0m"
COLOR_INFO="\033[1;34m"
COLOR_OK="\033[1;32m"
COLOR_WARN="\033[1;33m"
COLOR_ERR="\033[1;31m"

# ---------------------------------------------------------
# Helpers
# ---------------------------------------------------------

info()  { echo -e "${COLOR_INFO}[info]${COLOR_RESET} $*"; }
ok()    { echo -e "${COLOR_OK}[ok]${COLOR_RESET} $*"; }
warn()  { echo -e "${COLOR_WARN}[warn]${COLOR_RESET} $*"; }
error() { echo -e "${COLOR_ERR}[error]${COLOR_RESET} $*" >&2; exit 1; }

need_cmd() {
    command -v "$1" >/dev/null 2>&1 || error "missing command: $1"
}

# ---------------------------------------------------------
# Initialisation
# ---------------------------------------------------------

init_project() {
    info "initializing Vitte project"

    need_cmd git
    need_cmd "$PYTHON"

    chmod +x "$BUILD_SCRIPT" "$TEST_SCRIPT" || true

    if [ ! -d "$ROOT_DIR/.git" ]; then
        warn "not a git repository"
    fi

    ok "initialization complete"
}

# ---------------------------------------------------------
# Build
# ---------------------------------------------------------

run_build() {
    info "running build.sh"
    [ -f "$BUILD_SCRIPT" ] || error "build.sh not found"
    "$BUILD_SCRIPT" all
    ok "build finished"
}

# ---------------------------------------------------------
# Tests
# ---------------------------------------------------------

run_tests() {
    info "running test.sh"
    [ -f "$TEST_SCRIPT" ] || error "test.sh not found"
    "$TEST_SCRIPT" all
    ok "tests finished"
}

# ---------------------------------------------------------
# Dispatcher
# ---------------------------------------------------------

TARGET="${1:-all}"

case "$TARGET" in
    init)
        init_project
        ;;
    build)
        init_project
        run_build
        ;;
    test)
        init_project
        run_tests
        ;;
    all)
        init_project
        run_build
        run_tests
        ;;
    *)
        error "unknown target: $TARGET"
        ;;
esac

ok "bootstrap completed: $TARGET"