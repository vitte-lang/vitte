#!/usr/bin/env bash
# ============================================================
# vitte — permissions normalizer
# Location: toolchain/scripts/install/permissions.sh
# ============================================================

set -euo pipefail

# ----------------------------
# Configuration
# ----------------------------
ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/../../.." && pwd)}"
TARGET_DIR="${TARGET_DIR:-$ROOT_DIR/target}"
PREFIX="${PREFIX:-}"                 # optional: /usr/local, /opt/vitte, ~/.local
MODE="${MODE:-auto}"                 # auto | target | prefix
VERBOSE="${VERBOSE:-0}"              # 0 | 1

# Permission policy
DIR_MODE="0755"
BIN_MODE="0755"
LIB_MODE="0644"
HDR_MODE="0644"
SHARE_MODE="0644"
SCRIPT_MODE="0755"

# ----------------------------
# Helpers
# ----------------------------
log() { printf "[permissions] %s\n" "$*"; }
die() { printf "[permissions][error] %s\n" "$*" >&2; exit 1; }

run() {
  if [ "$VERBOSE" = "1" ]; then log "exec: $*"; fi
  "$@"
}

apply_dir() {
  local dir="$1"
  [ -d "$dir" ] || return 0
  run find "$dir" -type d -exec chmod "$DIR_MODE" {} +
}

apply_files() {
  local dir="$1" mode="$2"
  [ -d "$dir" ] || return 0
  run find "$dir" -type f -exec chmod "$mode" {} +
}

apply_exec_by_ext() {
  local dir="$1"
  [ -d "$dir" ] || return 0
  run find "$dir" -type f \( -name "*.sh" -o -name "*.bash" \) -exec chmod "$SCRIPT_MODE" {} +
}

# ----------------------------
# Init
# ----------------------------
cd "$ROOT_DIR"

if [ "$MODE" = "auto" ]; then
  if [ -n "$PREFIX" ]; then
    MODE="prefix"
  else
    MODE="target"
  fi
fi

log "mode=$MODE"

# ----------------------------
# Target tree (build outputs)
# ----------------------------
if [ "$MODE" = "target" ]; then
  log "normalizing permissions in target/"
  [ -d "$TARGET_DIR" ] || die "target dir not found: $TARGET_DIR"

  apply_dir "$TARGET_DIR"

  apply_files "$TARGET_DIR/bin"    "$BIN_MODE"
  apply_files "$TARGET_DIR/lib"    "$LIB_MODE"
  apply_files "$TARGET_DIR/include" "$HDR_MODE"
  apply_files "$TARGET_DIR/share"  "$SHARE_MODE"

  apply_exec_by_ext "$TARGET_DIR"

  log "target permissions normalized"
fi

# ----------------------------
# Prefix tree (installed layout)
# ----------------------------
if [ "$MODE" = "prefix" ]; then
  [ -n "$PREFIX" ] || die "PREFIX not set for prefix mode"
  log "normalizing permissions in prefix=$PREFIX"

  BINDIR="$PREFIX/bin"
  LIBDIR="$PREFIX/lib/vitte"
  INCLUDEDIR="$PREFIX/include/vitte"
  SHAREDIR="$PREFIX/share/vitte"

  apply_dir "$PREFIX"
  apply_files "$BINDIR"     "$BIN_MODE"
  apply_files "$LIBDIR"     "$LIB_MODE"
  apply_files "$INCLUDEDIR" "$HDR_MODE"
  apply_files "$SHAREDIR"   "$SHARE_MODE"

  apply_exec_by_ext "$PREFIX"

  log "prefix permissions normalized"
fi

log "done"