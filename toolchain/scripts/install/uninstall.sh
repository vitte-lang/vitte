#!/usr/bin/env bash
# ============================================================
# vitte — unified uninstaller
# Location: toolchain/scripts/install/uninstall.sh
# ============================================================

set -euo pipefail

# ----------------------------
# Configuration
# ----------------------------
ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/../../.." && pwd)}"
MODE="${MODE:-auto}"                  # auto | local | prefix | check
PREFIX="${PREFIX:-}"                  # e.g. /usr/local, /opt/vitte, ~/.local
VERBOSE="${VERBOSE:-0}"               # 0 | 1
DRY_RUN="${DRY_RUN:-0}"               # 0 | 1

# Known binaries
BINS=(
  "vittec"
  "vitte-linker"
)

# Layout
LOCAL_PREFIX="${LOCAL_PREFIX:-$HOME/.local}"

# ----------------------------
# Helpers
# ----------------------------
log() { printf "[uninstall] %s\n" "$*"; }
die() { printf "[uninstall][error] %s\n" "$*" >&2; exit 1; }

run() {
  if [ "$DRY_RUN" = "1" ]; then
    log "dry-run: $*"
  else
    if [ "$VERBOSE" = "1" ]; then log "exec: $*"; fi
    "$@"
  fi
}

rm_if_exists() {
  local p="$1"
  [ -e "$p" ] && run rm -rf "$p"
}

# ----------------------------
# Init
# ----------------------------
cd "$ROOT_DIR"

if [ "$MODE" = "auto" ]; then
  if [ -n "$PREFIX" ]; then
    MODE="prefix"
  else
    MODE="local"
  fi
fi

# ----------------------------
# Check
# ----------------------------
if [ "$MODE" = "check" ]; then
  log "checking installed vitte binaries"
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
# Local uninstall (~/.local)
# ----------------------------
if [ "$MODE" = "local" ]; then
  PREFIX="$LOCAL_PREFIX"
fi

# ----------------------------
# Prefix uninstall
# ----------------------------
[ -n "$PREFIX" ] || die "PREFIX not set"

BINDIR="$PREFIX/bin"
LIBDIR="$PREFIX/lib/vitte"
INCLUDEDIR="$PREFIX/include/vitte"
SHAREDIR="$PREFIX/share/vitte"

log "uninstalling from prefix=$PREFIX (mode=$MODE dry-run=$DRY_RUN)"

# Binaries
for b in "${BINS[@]}"; do
  rm_if_exists "$BINDIR/$b"
  log "removed: $BINDIR/$b"
done

# Vitte-specific trees only
rm_if_exists "$LIBDIR"
rm_if_exists "$INCLUDEDIR"
rm_if_exists "$SHAREDIR"

log "uninstall complete"