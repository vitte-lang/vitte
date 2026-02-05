#!/usr/bin/env bash
# ============================================================
# vitte — local installer
# Location: toolchain/scripts/install/install-local.sh
# ============================================================

set -euo pipefail

# ----------------------------
# Configuration
# ----------------------------
ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/../../.." && pwd)}"
TARGET_DIR="${TARGET_DIR:-$ROOT_DIR/target}"
MODE="${MODE:-install}"                 # install | uninstall | check
PREFIX="${PREFIX:-$HOME/.local}"        # install prefix
BINDIR="$PREFIX/bin"
LIBDIR="$PREFIX/lib/vitte"
INCLUDEDIR="$PREFIX/include/vitte"
SYMLINK="${SYMLINK:-1}"                 # 1: symlink, 0: copy
VERBOSE="${VERBOSE:-0}"                 # 0 | 1

# Binaries to install (extend as needed)
BINS=(
  "vittec"
  "vitte-linker"
)

# ----------------------------
# Helpers
# ----------------------------
log() { printf "[install] %s\n" "$*"; }
die() { printf "[install][error] %s\n" "$*" >&2; exit 1; }

run() {
  if [ "$VERBOSE" = "1" ]; then log "exec: $*"; fi
  "$@"
}

ensure_dir() { run mkdir -p "$1"; }

install_bin() {
  local src="$1" dst="$2"
  if [ "$SYMLINK" = "1" ]; then
    run ln -sf "$src" "$dst"
  else
    run install -m 0755 "$src" "$dst"
  fi
}

remove_bin() {
  local dst="$1"
  [ -e "$dst" ] && run rm -f "$dst"
}

# ----------------------------
# Init
# ----------------------------
cd "$ROOT_DIR"

# ----------------------------
# Check
# ----------------------------
if [ "$MODE" = "check" ]; then
  log "checking installation"
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
    remove_bin "$BINDIR/$b"
  done
  log "done"
  exit 0
fi

# ----------------------------
# Install
# ----------------------------
[ "$MODE" = "install" ] || die "unknown MODE: $MODE"

log "installing to $PREFIX (symlink=$SYMLINK)"

ensure_dir "$BINDIR"
ensure_dir "$LIBDIR"
ensure_dir "$INCLUDEDIR"

# Install binaries
for b in "${BINS[@]}"; do
  SRC="$TARGET_DIR/bin/$b"
  [ -x "$SRC" ] || die "binary not found or not executable: $SRC"
  install_bin "$SRC" "$BINDIR/$b"
  log "installed: $BINDIR/$b"
done

# Optional: headers / libs if present
if [ -d "$TARGET_DIR/include" ]; then
  ensure_dir "$INCLUDEDIR"
  run rsync -a --delete "$TARGET_DIR/include/" "$INCLUDEDIR/"
  log "headers synced to $INCLUDEDIR"
fi

if [ -d "$TARGET_DIR/lib" ]; then
  ensure_dir "$LIBDIR"
  run rsync -a --delete "$TARGET_DIR/lib/" "$LIBDIR/"
  log "libs synced to $LIBDIR"
fi

log "installation complete"
log "ensure \$PREFIX/bin is in PATH"