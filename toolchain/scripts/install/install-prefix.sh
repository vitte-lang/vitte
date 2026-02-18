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
MANDIR="$PREFIX/share/man/man1"
BASHCOMPDIR="$PREFIX/etc/bash_completion.d"
ZSHCOMPDIR="$PREFIX/share/zsh/site-functions"
FISHCOMPDIR="$PREFIX/share/fish/vendor_completions.d"

# Binaries to install
REQUIRED_BINS=(
  "vitte"
)

OPTIONAL_BINS=(
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

resolve_src_bin() {
  local name="$1"
  local a="$TARGET_DIR/bin/$name"
  local b="$ROOT_DIR/bin/$name"
  if [ -x "$a" ]; then
    printf "%s\n" "$a"
    return 0
  fi
  if [ -x "$b" ]; then
    printf "%s\n" "$b"
    return 0
  fi
  return 1
}

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
  for b in "${REQUIRED_BINS[@]}" "${OPTIONAL_BINS[@]}"; do
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
  for b in "${REQUIRED_BINS[@]}" "${OPTIONAL_BINS[@]}"; do
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
for b in "${REQUIRED_BINS[@]}"; do
  SRC="$(resolve_src_bin "$b" || true)"
  [ -n "$SRC" ] || die "required binary not found: $b (searched $TARGET_DIR/bin and $ROOT_DIR/bin)"
  DST="$BINDIR/$b"
  install_file "$SRC" "$DST" 0755
  strip_bin "$DST"
  log "installed: $DST"
done

for b in "${OPTIONAL_BINS[@]}"; do
  SRC="$(resolve_src_bin "$b" || true)"
  [ -n "$SRC" ] || { log "optional binary missing: $b (skipped)"; continue; }
  DST="$BINDIR/$b"
  install_file "$SRC" "$DST" 0755
  strip_bin "$DST"
  log "installed: $DST"
done

# Compatibility links for expected command names.
if [ ! -e "$BINDIR/vittec" ]; then
  run ln -sf "vitte" "$BINDIR/vittec"
  log "linked: $BINDIR/vittec -> vitte"
fi
if [ ! -e "$BINDIR/vitte-linker" ]; then
  run ln -sf "vitte" "$BINDIR/vitte-linker"
  log "linked: $BINDIR/vitte-linker -> vitte"
fi

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

# Editor support files (for pkg/macOS and prefix installs)
if [ -d "$ROOT_DIR/editors" ]; then
  ensure_dir "$SHAREDIR/editors"
  run rsync -a --delete \
    --exclude 'vscode/node_modules' \
    --exclude '.DS_Store' \
    "$ROOT_DIR/editors/" "$SHAREDIR/editors/"
  log "editor files synced to $SHAREDIR/editors"
fi

# Shell completions
if [ -f "$ROOT_DIR/completions/bash/vitte" ]; then
  ensure_dir "$BASHCOMPDIR"
  install_file "$ROOT_DIR/completions/bash/vitte" "$BASHCOMPDIR/vitte" 0644
  log "bash completion installed: $BASHCOMPDIR/vitte"
fi
if [ -f "$ROOT_DIR/completions/zsh/_vitte" ]; then
  ensure_dir "$ZSHCOMPDIR"
  install_file "$ROOT_DIR/completions/zsh/_vitte" "$ZSHCOMPDIR/_vitte" 0644
  log "zsh completion installed: $ZSHCOMPDIR/_vitte"
fi
if [ -f "$ROOT_DIR/completions/fish/vitte.fish" ]; then
  ensure_dir "$FISHCOMPDIR"
  install_file "$ROOT_DIR/completions/fish/vitte.fish" "$FISHCOMPDIR/vitte.fish" 0644
  log "fish completion installed: $FISHCOMPDIR/vitte.fish"
fi

# Man pages
if [ -d "$ROOT_DIR/man" ]; then
  ensure_dir "$MANDIR"
  for m in vitte.1 vittec.1 vitte-linker.1; do
    if [ -f "$ROOT_DIR/man/$m" ]; then
      install_file "$ROOT_DIR/man/$m" "$MANDIR/$m" 0644
      log "man installed: $MANDIR/$m"
    fi
  done
fi

# Environment helper
if [ -f "$ROOT_DIR/toolchain/scripts/install/templates/env.sh" ]; then
  ensure_dir "$SHAREDIR"
  install_file "$ROOT_DIR/toolchain/scripts/install/templates/env.sh" "$SHAREDIR/env.sh" 0644
  log "env helper installed: $SHAREDIR/env.sh"
fi

log "installation complete"
