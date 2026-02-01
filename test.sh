

#!/usr/bin/env bash
# =========================================================
# Vitte — Test Script (ULTRA-MAX)
# =========================================================
# Rôle :
# - valider l’intégrité globale du projet Vitte
# - exécuter des tests de structure, syntaxe et bootstrap
# - servir de base pour CI locale
#
# Usage :
#   ./test.sh
#   ./test.sh foundation
#   ./test.sh lingua
#   ./test.sh runtime
#   ./test.sh bootstrap
#   ./test.sh all
# =========================================================

set -euo pipefail

# ---------------------------------------------------------
# Configuration
# ---------------------------------------------------------

ROOT_DIR="$(cd "$(dirname "$0")" && pwd)"
FOUNDATION_DIR="$ROOT_DIR/foundation"
LINGUA_DIR="$ROOT_DIR/lingua"
RUNTIME_DIR="$ROOT_DIR/runtime"
SRC_DIR="$ROOT_DIR/src"
TOOLS_DIR="$ROOT_DIR/tools"

PYTHON="${PYTHON:-python3}"

COLOR_RESET="\033[0m"
COLOR_INFO="\033[1;34m"
COLOR_OK="\033[1;32m"
COLOR_WARN="\033[1;33m"
COLOR_ERR="\033[1;31m"

# ---------------------------------------------------------
# Helpers
# ---------------------------------------------------------

info()   { echo -e "${COLOR_INFO}[info]${COLOR_RESET} $*"; }
ok()     { echo -e "${COLOR_OK}[ok]${COLOR_RESET} $*"; }
warn()   { echo -e "${COLOR_WARN}[warn]${COLOR_RESET} $*"; }
error()  { echo -e "${COLOR_ERR}[error]${COLOR_RESET} $*" >&2; exit 1; }

# ---------------------------------------------------------
# Tests foundation
# ---------------------------------------------------------

test_foundation() {
    info "testing foundation"

    [ -d "$FOUNDATION_DIR" ] || error "foundation/ missing"
    [ -f "$FOUNDATION_DIR/grammar/vitte.ebnf" ] \
        || error "missing grammar/vitte.ebnf"

    ok "foundation OK"
}

# ---------------------------------------------------------
# Tests lingua
# ---------------------------------------------------------

test_lingua() {
    info "testing lingua"

    [ -d "$LINGUA_DIR" ] || error "lingua/ missing"

    find "$LINGUA_DIR" -name "*.vit" | while read -r f; do
        [ -s "$f" ] || warn "empty file: $f"
    done

    ok "lingua OK"
}

# ---------------------------------------------------------
# Tests runtime
# ---------------------------------------------------------

test_runtime() {
    info "testing runtime"

    [ -d "$RUNTIME_DIR" ] || error "runtime/ missing"

    if [ -d "$RUNTIME_DIR/std" ]; then
        info "runtime std present"
    else
        warn "runtime std missing"
    fi

    ok "runtime OK"
}

# ---------------------------------------------------------
# Tests bootstrap Python
# ---------------------------------------------------------

test_bootstrap() {
    info "testing bootstrap"

    [ -d "$SRC_DIR/bootstrap" ] || error "bootstrap missing"
    [ -f "$SRC_DIR/bootstrap/bootstrap.py" ] \
        || error "bootstrap.py missing"

    "$PYTHON" "$SRC_DIR/bootstrap/configure.py" >/dev/null

    ok "bootstrap OK"
}

# ---------------------------------------------------------
# Tests tools (vittec)
# ---------------------------------------------------------

test_tools() {
    info "testing tools"

    [ -d "$TOOLS_DIR/vittec" ] || warn "vittec not found"
    ok "tools OK"
}

# ---------------------------------------------------------
# Dispatcher
# ---------------------------------------------------------

TARGET="${1:-all}"

case "$TARGET" in
    foundation)
        test_foundation
        ;;
    lingua)
        test_lingua
        ;;
    runtime)
        test_runtime
        ;;
    bootstrap)
        test_bootstrap
        ;;
    tools)
        test_tools
        ;;
    all)
        test_foundation
        test_lingua
        test_runtime
        test_bootstrap
        test_tools
        ;;
    *)
        error "unknown test target: $TARGET"
        ;;
esac

ok "tests finished: $TARGET"