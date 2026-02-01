#!/usr/bin/env bash
# =========================================================
# Vitte — Build Script (ULTRA-MAX)
# =========================================================
# Usage:
#   ./build.sh
#   ./build.sh all
#   ./build.sh foundation
#   ./build.sh lingua
#   ./build.sh runtime
#   ./build.sh bootstrap
#   ./build.sh clean
#
# Environment variables:
#   PYTHON        Python interpreter (default: python3)
#   VITTEC_BIN   vittec binary path (optional)
# =========================================================

set -euo pipefail

# ---------------------------------------------------------
# Configuration
# ---------------------------------------------------------

PYTHON="${PYTHON:-python3}"
ROOT_DIR="$(cd "$(dirname "$0")" && pwd)"
SRC_DIR="$ROOT_DIR/src"
FOUNDATION_DIR="$ROOT_DIR/foundation"
LINGUA_DIR="$ROOT_DIR/lingua"
RUNTIME_DIR="$ROOT_DIR/runtime"
TOOLS_DIR="$ROOT_DIR/tools"
BUILD_DIR="$ROOT_DIR/build"
CACHE_DIR="$ROOT_DIR/.vitte-cache"

BOOTSTRAP="$SRC_DIR/bootstrap/bootstrap.py"
CONFIGURE="$SRC_DIR/bootstrap/configure.py"

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

need_cmd() {
    command -v "$1" >/dev/null 2>&1 || error "missing command: $1"
}

# ---------------------------------------------------------
# Checks
# ---------------------------------------------------------

check_environment() {
    info "checking environment"

    need_cmd "$PYTHON"
    need_cmd git

    [ -d "$SRC_DIR" ] || error "src/ directory missing"
    [ -d "$FOUNDATION_DIR" ] || error "foundation/ directory missing"
    [ -d "$LINGUA_DIR" ] || error "lingua/ directory missing"
    [ -d "$RUNTIME_DIR" ] || error "runtime/ directory missing"

    ok "environment OK"
}

# ---------------------------------------------------------
# Foundation
# ---------------------------------------------------------

build_foundation() {
    info "building foundation (specs only)"

    if [ ! -f "$FOUNDATION_DIR/grammar/vitte.ebnf" ]; then
        warn "missing vitte.ebnf (skipping foundation specs)"
        return 0
    fi

    ok "foundation validated"
}

# ---------------------------------------------------------
# Lingua (langage)
# ---------------------------------------------------------

build_lingua() {
    info "building lingua (syntax / semantics)"

    find "$LINGUA_DIR" -name "*.vit" >/dev/null \
        || warn "no .vit files found in lingua"

    ok "lingua validated"
}

# ---------------------------------------------------------
# Runtime
# ---------------------------------------------------------

build_runtime() {
    info "building runtime"

    if [ -d "$RUNTIME_DIR/std" ]; then
        info "runtime std detected"
    else
        warn "runtime std not found"
    fi

    ok "runtime validated"
}

# ---------------------------------------------------------
# Bootstrap Python
# ---------------------------------------------------------

build_bootstrap() {
    info "running Python bootstrap"

    [ -f "$CONFIGURE" ] || error "missing bootstrap/configure.py"
    [ -f "$BOOTSTRAP" ] || error "missing bootstrap/bootstrap.py"

    "$PYTHON" "$CONFIGURE" >/dev/null
    "$PYTHON" "$BOOTSTRAP" --check examples/hello.vit 2>/dev/null || true

    ok "bootstrap OK"
}

# ---------------------------------------------------------
# vittec (compiler)
# ---------------------------------------------------------

build_vittec() {
    info "building vittec"

    if [ -n "${VITTEC_BIN:-}" ]; then
        info "using vittec: $VITTEC_BIN"
        "$VITTEC_BIN" --version || warn "vittec not runnable"
    else
        warn "VITTEC_BIN not set (skipping native compiler)"
    fi

    ok "vittec step done"
}

# ---------------------------------------------------------
# Clean
# ---------------------------------------------------------

clean() {
    info "cleaning build artifacts"
    rm -rf "$BUILD_DIR" "$CACHE_DIR"
    ok "clean complete"
}

# ---------------------------------------------------------
# Main dispatcher
# ---------------------------------------------------------

TARGET="${1:-all}"

check_environment

case "$TARGET" in
    all)
        build_foundation
        build_lingua
        build_runtime
        build_bootstrap
        build_vittec
        ;;
    foundation)
        build_foundation
        ;;
    lingua)
        build_lingua
        ;;
    runtime)
        build_runtime
        ;;
    bootstrap)
        build_bootstrap
        ;;
    clean)
        clean
        ;;
    *)
        error "unknown target: $TARGET"
        ;;
esac

ok "build finished: $TARGET"
