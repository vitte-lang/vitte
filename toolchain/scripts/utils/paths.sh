#!/usr/bin/env bash
# ============================================================
# vitte — paths utility
# Location: toolchain/scripts/utils/paths.sh
# ============================================================

# Usage:
#   source toolchain/scripts/utils/paths.sh
#   path_init
#   path_add PATH "/some/bin"
#   path_prepend CPATH "/some/include"
#   path_export_all

set -euo pipefail

# ----------------------------
# Configuration
# ----------------------------
ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/../../.." && pwd)}"
EXPORT="${EXPORT:-1}"            # 1: export vars, 0: print only
VERBOSE="${VERBOSE:-0}"

# ----------------------------
# Helpers
# ----------------------------
log() { [ "$VERBOSE" = "1" ] && printf "[paths] %s\n" "$*"; }

_put() {
  local k="$1" v="$2"
  if [ "$EXPORT" = "1" ]; then
    export "$k=$v"
  else
    printf "%s=%q\n" "$k" "$v"
  fi
}

_norm() {
  # Normalize path (best-effort, no realpath dependency)
  local p="$1"
  (cd "$p" 2>/dev/null && pwd) || echo "$p"
}

_unique_join() {
  # Join paths with ':' ensuring uniqueness and preserving order
  local sep=":" out="" seen=""
  for p in "$@"; do
    [ -z "$p" ] && continue
    case ":$seen:" in
      *":$p:"*) ;;
      *) out="${out:+$out$sep}$p"; seen="${seen:+$seen$sep}$p" ;;
    esac
  done
  printf "%s" "$out"
}

# ----------------------------
# Public API
# ----------------------------
path_init() {
  log "initializing standard paths"

  _put VITTE_ROOT "$ROOT_DIR"
  _put VITTE_TARGET_DIR "${VITTE_TARGET_DIR:-$ROOT_DIR/target}"
  _put VITTE_CACHE_DIR  "${VITTE_CACHE_DIR:-$ROOT_DIR/.cache/vitte}"
  _put VITTE_STORE_DIR  "${VITTE_STORE_DIR:-$ROOT_DIR/.store/vitte}"

  _put VITTE_BIN_DIR    "${VITTE_BIN_DIR:-$VITTE_TARGET_DIR/bin}"
  _put VITTE_LIB_DIR    "${VITTE_LIB_DIR:-$VITTE_TARGET_DIR/lib}"
  _put VITTE_INC_DIR    "${VITTE_INC_DIR:-$VITTE_TARGET_DIR/include}"
}

path_add() {
  # Append path to env var (PATH-like)
  local var="$1" p="$2"
  p="$(_norm "$p")"
  log "add $p to $var"
  local cur="${!var:-}"
  _put "$var" "$(_unique_join "$cur" "$p")"
}

path_prepend() {
  # Prepend path to env var (PATH-like)
  local var="$1" p="$2"
  p="$(_norm "$p")"
  log "prepend $p to $var"
  local cur="${!var:-}"
  _put "$var" "$(_unique_join "$p" "$cur")"
}

path_remove() {
  # Remove a path from env var (PATH-like)
  local var="$1" p="$2"
  local cur="${!var:-}" out=""
  IFS=":" read -r -a arr <<< "$cur"
  for x in "${arr[@]}"; do
    [ "$x" = "$p" ] && continue
    out="${out:+$out:}$x"
  done
  log "remove $p from $var"
  _put "$var" "$out"
}

path_export_all() {
  # Export conventional env vars based on VITTE_* dirs
  log "exporting conventional paths"
  [ -d "$VITTE_BIN_DIR" ] && path_prepend PATH "$VITTE_BIN_DIR"
  [ -d "$VITTE_LIB_DIR" ] && path_prepend LIBRARY_PATH "$VITTE_LIB_DIR"
  [ -d "$VITTE_INC_DIR" ] && path_prepend CPATH "$VITTE_INC_DIR"
}

# ----------------------------
# Optional auto-init
# ----------------------------
if [ "${PATHS_AUTO_INIT:-1}" = "1" ]; then
  path_init
fi