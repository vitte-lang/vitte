#!/usr/bin/env bash
# ============================================================
# vitte — developer debug environment
# Location: toolchain/scripts/dev/debug-env.sh
# ============================================================

set -euo pipefail

# ----------------------------
# Configuration
# ----------------------------
ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/../../.." && pwd)}"
VERBOSE="${VERBOSE:-1}"

# ----------------------------
# Helpers
# ----------------------------
hr() { printf "\n------------------------------------------------------------\n"; }
log() { printf "[debug-env] %s\n" "$*"; }

show() {
  local key="$1" val="$2"
  printf "%-28s : %s\n" "$key" "$val"
}

has() {
  command -v "$1" >/dev/null 2>&1
}

cmd_ver() {
  local cmd="$1"
  if has "$cmd"; then
    printf "%-28s : %s\n" "$cmd" "$("$cmd" --version 2>/dev/null | head -n 1)"
  else
    printf "%-28s : %s\n" "$cmd" "not found"
  fi
}

# ----------------------------
# Banner
# ----------------------------
hr
log "Vitte developer environment diagnostics"
hr

# ----------------------------
# Paths / Core env
# ----------------------------
log "paths"
show "ROOT_DIR" "$ROOT_DIR"
show "PWD" "$(pwd)"
show "SHELL" "${SHELL:-unknown}"
show "PATH" "$PATH"

# ----------------------------
# System
# ----------------------------
hr
log "system"
show "OS" "$(uname -s)"
show "ARCH" "$(uname -m)"
show "KERNEL" "$(uname -r)"

if has sw_vers; then
  show "macOS" "$(sw_vers -productVersion)"
fi

if has lsb_release; then
  show "Linux distro" "$(lsb_release -ds)"
fi

# ----------------------------
# Toolchain
# ----------------------------
hr
log "toolchain"
cmd_ver git
cmd_ver bash
cmd_ver sh
cmd_ver python3
cmd_ver python
cmd_ver clang
cmd_ver clang++
cmd_ver gcc
cmd_ver g++
cmd_ver cmake
cmd_ver ninja
cmd_ver make
cmd_ver rustc
cmd_ver cargo

# ----------------------------
# CI / Build tools
# ----------------------------
hr
log "ci / build tools"
cmd_ver shfmt
cmd_ver shellcheck
cmd_ver clang-format
cmd_ver clang-tidy
cmd_ver cppcheck
cmd_ver prettier

# ----------------------------
# Vitte workspace sanity
# ----------------------------
hr
log "vitte workspace"

show "git repo" "$(git rev-parse --is-inside-work-tree 2>/dev/null || echo no)"
show "git branch" "$(git branch --show-current 2>/dev/null || echo n/a)"
show "git commit" "$(git rev-parse --short HEAD 2>/dev/null || echo n/a)"

if [ -d "$ROOT_DIR/toolchain" ]; then
  show "toolchain dir" "present"
else
  show "toolchain dir" "missing"
fi

if [ -d "$ROOT_DIR/target" ]; then
  show "target dir" "present"
else
  show "target dir" "missing"
fi

# ----------------------------
# Environment variables (filtered)
# ----------------------------
hr
log "env vars (filtered)"
for v in \
  ROOT_DIR \
  VITTE_ROOT \
  VITTE_HOME \
  CI CI_NAME CI_COMMIT CI_REF \
  CC CXX AR LD \
  CFLAGS CXXFLAGS LDFLAGS \
  RUSTFLAGS
do
  if [ -n "${!v-}" ]; then
    show "$v" "${!v}"
  fi
done

# ----------------------------
# Optional deep dump
# ----------------------------
if [ "$VERBOSE" = "1" ]; then
  hr
  log "full env (sorted)"
  env | sort
fi

# ----------------------------
# Done
# ----------------------------
hr
log "diagnostics complete"