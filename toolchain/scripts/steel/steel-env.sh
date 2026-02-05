#!/usr/bin/env bash
# ============================================================
# vitte — Steel environment setup
# Location: toolchain/scripts/steel/steel-env.sh
# ============================================================

set -euo pipefail

# ----------------------------
# Configuration
# ----------------------------
ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/../../.." && pwd)}"
PROFILE="${PROFILE:-debug}"            # debug | release | custom
TARGET="${TARGET:-}"                   # optional target script (e.g. linux-x86_64.sh)
STEEL_FILE="${STEEL_FILE:-steelconf}"  # default steel file
EXPORT_ENV="${EXPORT_ENV:-1}"          # 1: export vars, 0: print only
VERBOSE="${VERBOSE:-0}"                # 0 | 1

# ----------------------------
# Helpers
# ----------------------------
log() { printf "[steel-env] %s\n" "$*"; }
die() { printf "[steel-env][error] %s\n" "$*" >&2; exit 1; }

put() {
  local k="$1" v="$2"
  if [ "$EXPORT_ENV" = "1" ]; then
    export "$k=$v"
  else
    printf "%s=%q\n" "$k" "$v"
  fi
}

# ----------------------------
# Init
# ----------------------------
cd "$ROOT_DIR"

[ -f "$STEEL_FILE" ] || die "steel file not found: $STEEL_FILE"

log "root=$ROOT_DIR profile=$PROFILE steel_file=$STEEL_FILE"

# ----------------------------
# Base Steel environment
# ----------------------------
put STEEL_ROOT "$ROOT_DIR"
put STEEL_FILE "$STEEL_FILE"
put STEEL_PROFILE "$PROFILE"

# Conventional dirs
put STEEL_TARGET_DIR "${STEEL_TARGET_DIR:-$ROOT_DIR/target}"
put STEEL_CACHE_DIR  "${STEEL_CACHE_DIR:-$ROOT_DIR/.cache/steel}"
put STEEL_STORE_DIR  "${STEEL_STORE_DIR:-$ROOT_DIR/.store/steel}"

# CI metadata (best-effort)
put CI_NAME   "${CI_NAME:-local}"
put CI_COMMIT "${CI_COMMIT:-$(git rev-parse --short HEAD 2>/dev/null || echo unknown)}"
put CI_REF    "${CI_REF:-$(git branch --show-current 2>/dev/null || echo unknown)}"

# ----------------------------
# Load target (optional)
# ----------------------------
if [ -n "$TARGET" ]; then
  TARGET_SCRIPT="$ROOT_DIR/toolchain/scripts/targets/$TARGET"
  [ -f "$TARGET_SCRIPT" ] || die "target script not found: $TARGET_SCRIPT"
  log "loading target: $TARGET"
  # shellcheck source=/dev/null
  source "$TARGET_SCRIPT"
fi

# ----------------------------
# Profile-specific knobs
# ----------------------------
case "$PROFILE" in
  debug)
    put VITTE_BUILD_OPT "0"
    put VITTE_BUILD_DEBUG "1"
    ;;
  release)
    put VITTE_BUILD_OPT "2"
    put VITTE_BUILD_DEBUG "0"
    ;;
  *)
    log "custom profile: $PROFILE"
    ;;
esac

# ----------------------------
# PATH augmentation (local toolchain)
# ----------------------------
if [ -d "$ROOT_DIR/target/bin" ]; then
  put PATH "$ROOT_DIR/target/bin:${PATH}"
fi

# ----------------------------
# Summary
# ----------------------------
log "environment ready"
if [ "$VERBOSE" = "1" ]; then
  env | sort
fi