#!/usr/bin/env bash
# ============================================================
# vitte — prefix installer
# Location: toolchain/scripts/install/install-prefix.sh
# ============================================================

set -euo pipefail

# ----------------------------
# Configuration
# ----------------------------
ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/../../.." && pwd)}"
TARGET_DIR="${TARGET_DIR:-$ROOT_DIR/target}"

MODE="${MODE:-install}"             # install | uninstall | check
PREFIX="${PREFIX:-/usr/local}"      # e.g. /usr/local, /opt/vitte, /usr
SYMLINK="${SYMLINK:-0}"             # 1: symlink, 0: copy (default safer)
VERBOSE="${VERBOSE:-0}"             # 0 | 1
STRIP_BIN="${STRIP_BIN:-0}"         # 1: strip binaries if available

BINDIR="$PREFIX/bin"
LIBDIR="$PREFIX/lib/vitte"
INCLUDEDIR="$PREFIX/include/vitte"
SHAREDIR="$PREFIX/share/vitte"

# Binaries to install
BINS=(
  "vittec"
  "vitte-linker"
)

# ----------------------------
# Helpers
# ----------------------------
log() { printf "[install-prefix] %s\n" "$*"; }
die() { printf "[install-prefix][error] %s\n" "$*" >&2; exit 1; }

run() {
  if [ "$VERBOSE" = "1" ]; then log "exec: $*"; fi
  "$@"
}

has() { command -v "$1" >/dev/null 2>&1; }

ensure_dir() { run mkdir -p "$1"; }

install_file() {
  local src="$1" dst="$2" mode="$3"
  if [ "$SYMLINK" = "1" ]; then
    run ln -sf "$src" "$dst"
  else
    run install -m "$mode" "$src" "$dst"
  fi
}

remove_path() {
  local p="$1"
  [ -e "$p" ] && run rm -rf "$p"
}

strip_bin() {
  local f="$1"
  if [ "$STRIP_BIN" = "1" ] && has strip; then
    run strip "$f" || true
  fi
}

# ----------------------------
# Init
# ----------------------------
cd "$ROOT_DIR"

# ----------------------------
# Check
# ----------------------------
if [ "$MODE" = "check" ]; then
  log "checking prefix installation at $PREFIX"
  for b in "${BINS[@]}"; do
    if command -v "$b" >/dev/null 2>&1; then
      log "found: $(command -v "$b")"
    else
      log "missing: $b"
    fi
  done
  exit 0
fi

# ----------------------------
# Uninstall
# ----------------------------
if [ "$MODE" = "uninstall" ]; then
  log "uninstalling from $PREFIX"
  for b in "${BINS[@]}"; do
    remove_path "$BINDIR/$b"
  done
  # Remove only vitte-specific trees
  remove_path "$LIBDIR"
  remove_path "$INCLUDEDIR"
  remove_path "$SHAREDIR"
  log "done"
  exit 0
fi

# ----------------------------
# Install
# ----------------------------
[ "$MODE" = "install" ] || die "unknown MODE: $MODE"

log "installing to $PREFIX (symlink=$SYMLINK strip=$STRIP_BIN)"

ensure_dir "$BINDIR"
ensure_dir "$LIBDIR"
ensure_dir "$INCLUDEDIR"
ensure_dir "$SHAREDIR"

# Install binaries
for b in "${BINS[@]}"; do
  SRC="$TARGET_DIR/bin/$b"
  DST="$BINDIR/$b"
  [ -x "$SRC" ] || die "binary not found or not executable: $SRC"
  install_file "$SRC" "$DST" 0755
  strip_bin "$DST"
  log "installed: $DST"
done

# Headers
if [ -d "$TARGET_DIR/include" ]; then
  run rsync -a --delete "$TARGET_DIR/include/" "$INCLUDEDIR/"
  log "headers synced to $INCLUDEDIR"
fi

# Libraries
if [ -d "$TARGET_DIR/lib" ]; then
  run rsync -a --delete "$TARGET_DIR/lib/" "$LIBDIR/"
  log "libs synced to $LIBDIR"
fi

# Shared data (optional)
if [ -d "$TARGET_DIR/share" ]; then
  run rsync -a --delete "$TARGET_DIR/share/" "$SHAREDIR/"
  log "share synced to $SHAREDIR"
fi

log "installation complete"