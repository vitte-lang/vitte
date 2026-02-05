#!/usr/bin/env bash
# ============================================================
# vitte — runtime bundle packager
# Location: toolchain/scripts/package/bundle-runtime.sh
# ============================================================

set -euo pipefail

# ----------------------------
# Configuration
# ----------------------------
ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/../../.." && pwd)}"
TARGET_DIR="${TARGET_DIR:-$ROOT_DIR/target}"

OUT_DIR="${OUT_DIR:-$TARGET_DIR/packages}"
NAME="${NAME:-vitte-runtime}"
VERSION="${VERSION:-0.0.0}"
OS="${OS:-$(uname -s | tr '[:upper:]' '[:lower:]')}"
ARCH="${ARCH:-$(uname -m)}"

INCLUDE_BINS="${INCLUDE_BINS:-1}"      # 1|0
INCLUDE_LIBS="${INCLUDE_LIBS:-1}"      # 1|0
INCLUDE_HEADERS="${INCLUDE_HEADERS:-0}"# 1|0
INCLUDE_SHARE="${INCLUDE_SHARE:-1}"    # 1|0

STRIP_BIN="${STRIP_BIN:-0}"             # 1|0 (if strip available)
VERBOSE="${VERBOSE:-0}"

# ----------------------------
# Helpers
# ----------------------------
log() { printf "[bundle-runtime] %s\n" "$*"; }
die() { printf "[bundle-runtime][error] %s\n" "$*" >&2; exit 1; }

run() {
  if [ "$VERBOSE" = "1" ]; then log "exec: $*"; fi
  "$@"
}

has() { command -v "$1" >/dev/null 2>&1; }

# ----------------------------
# Init
# ----------------------------
cd "$ROOT_DIR"
mkdir -p "$OUT_DIR"

STAGE="$(mktemp -d)"
trap 'rm -rf "$STAGE"' EXIT

PKG_ROOT="$STAGE/$NAME-$VERSION-$OS-$ARCH"
mkdir -p "$PKG_ROOT"

log "name=$NAME version=$VERSION os=$OS arch=$ARCH"
log "stage=$PKG_ROOT"

# ----------------------------
# Copy payload
# ----------------------------
if [ "$INCLUDE_BINS" = "1" ] && [ -d "$TARGET_DIR/bin" ]; then
  log "including bin/"
  mkdir -p "$PKG_ROOT/bin"
  run rsync -a "$TARGET_DIR/bin/" "$PKG_ROOT/bin/"
  if [ "$STRIP_BIN" = "1" ] && has strip; then
    find "$PKG_ROOT/bin" -type f -perm -111 -exec strip {} + || true
  fi
fi

if [ "$INCLUDE_LIBS" = "1" ] && [ -d "$TARGET_DIR/lib" ]; then
  log "including lib/"
  mkdir -p "$PKG_ROOT/lib"
  run rsync -a "$TARGET_DIR/lib/" "$PKG_ROOT/lib/"
fi

if [ "$INCLUDE_HEADERS" = "1" ] && [ -d "$TARGET_DIR/include" ]; then
  log "including include/"
  mkdir -p "$PKG_ROOT/include"
  run rsync -a "$TARGET_DIR/include/" "$PKG_ROOT/include/"
fi

if [ "$INCLUDE_SHARE" = "1" ] && [ -d "$TARGET_DIR/share" ]; then
  log "including share/"
  mkdir -p "$PKG_ROOT/share"
  run rsync -a "$TARGET_DIR/share/" "$PKG_ROOT/share/"
fi

# ----------------------------
# Metadata
# ----------------------------
log "writing metadata"
cat > "$PKG_ROOT/METADATA" <<EOF
name=$NAME
version=$VERSION
os=$OS
arch=$ARCH
built_at=$(date -u +"%Y-%m-%dT%H:%M:%SZ")
git_commit=$(git rev-parse --short HEAD 2>/dev/null || echo unknown)
EOF

# ----------------------------
# Archive
# ----------------------------
ARCHIVE="$OUT_DIR/$NAME-$VERSION-$OS-$ARCH.tar.gz"
log "creating archive $ARCHIVE"
tar -czf "$ARCHIVE" -C "$STAGE" "$(basename "$PKG_ROOT")"

# ----------------------------
# Checksums
# ----------------------------
if has sha256sum; then
  (cd "$OUT_DIR" && sha256sum "$(basename "$ARCHIVE")" > "$(basename "$ARCHIVE").sha256")
elif has shasum; then
  (cd "$OUT_DIR" && shasum -a 256 "$(basename "$ARCHIVE")" > "$(basename "$ARCHIVE").sha256")
fi

log "bundle ready"
echo "$ARCHIVE"