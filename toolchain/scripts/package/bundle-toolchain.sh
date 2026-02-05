#!/usr/bin/env bash
# ============================================================
# vitte — toolchain bundle packager
# Location: toolchain/scripts/package/bundle-toolchain.sh
# ============================================================

set -euo pipefail

# ----------------------------
# Configuration
# ----------------------------
ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/../../.." && pwd)}"
TARGET_DIR="${TARGET_DIR:-$ROOT_DIR/target}"

OUT_DIR="${OUT_DIR:-$TARGET_DIR/packages}"
NAME="${NAME:-vitte-toolchain}"
VERSION="${VERSION:-0.0.0}"
OS="${OS:-$(uname -s | tr '[:upper:]' '[:lower:]')}"
ARCH="${ARCH:-$(uname -m)}"

INCLUDE_RUNTIME="${INCLUDE_RUNTIME:-1}" # 1|0
INCLUDE_STDLIB="${INCLUDE_STDLIB:-1}"   # 1|0
INCLUDE_SCRIPTS="${INCLUDE_SCRIPTS:-0}" # 1|0 (toolchain/scripts)
INCLUDE_DOCS="${INCLUDE_DOCS:-0}"       # 1|0
INCLUDE_TESTS="${INCLUDE_TESTS:-0}"     # 1|0

STRIP_BIN="${STRIP_BIN:-0}"             # 1|0 (if strip available)
VERBOSE="${VERBOSE:-0}"

# Source stdlib (adapter si besoin)
if [ -n "${STDLIB_SRC:-}" ]; then
  STDLIB_SRC="$STDLIB_SRC"
elif [ -d "$ROOT_DIR/src/vitte/std" ]; then
  STDLIB_SRC="$ROOT_DIR/src/vitte/std"
else
  STDLIB_SRC="$ROOT_DIR/runtime/std"
fi

# ----------------------------
# Helpers
# ----------------------------
log() { printf "[bundle-toolchain] %s\n" "$*"; }
die() { printf "[bundle-toolchain][error] %s\n" "$*" >&2; exit 1; }

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
# Copy runtime payload
# ----------------------------
if [ "$INCLUDE_RUNTIME" = "1" ]; then
  if [ -d "$TARGET_DIR/bin" ]; then
    log "including bin/"
    mkdir -p "$PKG_ROOT/bin"
    run rsync -a "$TARGET_DIR/bin/" "$PKG_ROOT/bin/"
    if [ "$STRIP_BIN" = "1" ] && has strip; then
      find "$PKG_ROOT/bin" -type f -perm -111 -exec strip {} + || true
    fi
  fi

  if [ -d "$TARGET_DIR/lib" ]; then
    log "including lib/"
    mkdir -p "$PKG_ROOT/lib"
    run rsync -a "$TARGET_DIR/lib/" "$PKG_ROOT/lib/"
  fi

  if [ -d "$TARGET_DIR/include" ]; then
    log "including include/"
    mkdir -p "$PKG_ROOT/include"
    run rsync -a "$TARGET_DIR/include/" "$PKG_ROOT/include/"
  fi

  if [ -d "$TARGET_DIR/share" ]; then
    log "including share/"
    mkdir -p "$PKG_ROOT/share"
    run rsync -a "$TARGET_DIR/share/" "$PKG_ROOT/share/"
  fi
fi

# ----------------------------
# Copy stdlib
# ----------------------------
if [ "$INCLUDE_STDLIB" = "1" ]; then
  [ -d "$STDLIB_SRC" ] || die "stdlib source not found: $STDLIB_SRC"
  log "including stdlib"
  mkdir -p "$PKG_ROOT/std"
  run rsync -a \
    --exclude ".git" \
    --exclude "target" \
    "$STDLIB_SRC/" "$PKG_ROOT/std/"

  if [ "$INCLUDE_DOCS" = "0" ]; then
    log "excluding stdlib docs"
    rm -rf "$PKG_ROOT/std/docs" "$PKG_ROOT/std/doc" || true
  fi

  if [ "$INCLUDE_TESTS" = "0" ]; then
    log "excluding stdlib tests"
    rm -rf "$PKG_ROOT/std/tests" "$PKG_ROOT/std/test" || true
  fi
fi

# ----------------------------
# Optional scripts
# ----------------------------
if [ "$INCLUDE_SCRIPTS" = "1" ] && [ -d "$ROOT_DIR/toolchain/scripts" ]; then
  log "including toolchain/scripts"
  mkdir -p "$PKG_ROOT/toolchain/scripts"
  run rsync -a \
    --exclude ".git" \
    "$ROOT_DIR/toolchain/scripts/" "$PKG_ROOT/toolchain/scripts/"
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
stdlib_source=$STDLIB_SRC
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
