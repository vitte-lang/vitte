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
SHAREDIR="$PREFIX/share/vitte"
MANDIR="$PREFIX/share/man/man1"
BASHCOMPDIR="$PREFIX/etc/bash_completion.d"
ZSHCOMPDIR="$PREFIX/share/zsh/site-functions"
FISHCOMPDIR="$PREFIX/share/fish/vendor_completions.d"
SYMLINK="${SYMLINK:-1}"                 # 1: symlink, 0: copy
VERBOSE="${VERBOSE:-0}"                 # 0 | 1

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
log() { printf "[install] %s\n" "$*"; }
die() { printf "[install][error] %s\n" "$*" >&2; exit 1; }

run() {
  if [ "$VERBOSE" = "1" ]; then log "exec: $*"; fi
  "$@"
}

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
for b in "${REQUIRED_BINS[@]}"; do
  SRC="$(resolve_src_bin "$b" || true)"
  [ -n "$SRC" ] || die "required binary not found: $b (searched $TARGET_DIR/bin and $ROOT_DIR/bin)"
  install_bin "$SRC" "$BINDIR/$b"
  log "installed: $BINDIR/$b"
done

for b in "${OPTIONAL_BINS[@]}"; do
  SRC="$(resolve_src_bin "$b" || true)"
  [ -n "$SRC" ] || { log "optional binary missing: $b (skipped)"; continue; }
  install_bin "$SRC" "$BINDIR/$b"
  log "installed: $BINDIR/$b"
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

# Editor support files
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
  run install -m 0644 "$ROOT_DIR/completions/bash/vitte" "$BASHCOMPDIR/vitte"
  log "bash completion installed: $BASHCOMPDIR/vitte"
fi
if [ -f "$ROOT_DIR/completions/zsh/_vitte" ]; then
  ensure_dir "$ZSHCOMPDIR"
  run install -m 0644 "$ROOT_DIR/completions/zsh/_vitte" "$ZSHCOMPDIR/_vitte"
  log "zsh completion installed: $ZSHCOMPDIR/_vitte"
fi
if [ -f "$ROOT_DIR/completions/fish/vitte.fish" ]; then
  ensure_dir "$FISHCOMPDIR"
  run install -m 0644 "$ROOT_DIR/completions/fish/vitte.fish" "$FISHCOMPDIR/vitte.fish"
  log "fish completion installed: $FISHCOMPDIR/vitte.fish"
fi

# Man pages
if [ -d "$ROOT_DIR/man" ]; then
  ensure_dir "$MANDIR"
  for m in vitte.1 vittec.1 vitte-linker.1; do
    if [ -f "$ROOT_DIR/man/$m" ]; then
      run install -m 0644 "$ROOT_DIR/man/$m" "$MANDIR/$m"
      log "man installed: $MANDIR/$m"
    fi
  done
fi

# Environment helper
if [ -f "$ROOT_DIR/toolchain/scripts/install/templates/env.sh" ]; then
  ensure_dir "$SHAREDIR"
  run install -m 0644 "$ROOT_DIR/toolchain/scripts/install/templates/env.sh" "$SHAREDIR/env.sh"
  log "env helper installed: $SHAREDIR/env.sh"
fi

log "installation complete"
log "ensure \$PREFIX/bin is in PATH"
