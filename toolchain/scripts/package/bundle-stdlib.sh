#!/usr/bin/env bash
# ============================================================
# vitte — stdlib bundle packager
# Location: toolchain/scripts/package/bundle-stdlib.sh
# ============================================================

set -euo pipefail

# ----------------------------
# Configuration
# ----------------------------
ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/../../.." && pwd)}"

# Source stdlib (adapter si besoin)
if [ -n "${STDLIB_SRC:-}" ]; then
  STDLIB_SRC="$STDLIB_SRC"
elif [ -d "$ROOT_DIR/src/vitte/packages" ]; then
  STDLIB_SRC="$ROOT_DIR/src/vitte/packages"
else
  STDLIB_SRC="$ROOT_DIR/runtime/std"
fi
STDLIB_NAME="${STDLIB_NAME:-vitte-stdlib}"

OUT_DIR="${OUT_DIR:-$ROOT_DIR/target/packages}"
VERSION="${VERSION:-0.0.0}"
OS="${OS:-any}"
ARCH="${ARCH:-any}"

INCLUDE_DOCS="${INCLUDE_DOCS:-1}"     # 1|0
INCLUDE_TESTS="${INCLUDE_TESTS:-0}"   # 1|0
VERBOSE="${VERBOSE:-0}"

# ----------------------------
# Helpers
# ----------------------------
log() { printf "[bundle-stdlib] %s\n" "$*"; }
die() { printf "[bundle-stdlib][error] %s\n" "$*" >&2; exit 1; }

run() {
  if [ "$VERBOSE" = "1" ]; then log "exec: $*"; fi
  "$@"
}

has() { command -v "$1" >/dev/null 2>&1; }

# ----------------------------
# Init
# ----------------------------
cd "$ROOT_DIR"

[ -d "$STDLIB_SRC" ] || die "stdlib source not found: $STDLIB_SRC"

mkdir -p "$OUT_DIR"

STAGE="$(mktemp -d)"
trap 'rm -rf "$STAGE"' EXIT

PKG_ROOT="$STAGE/$STDLIB_NAME-$VERSION"
mkdir -p "$PKG_ROOT"

log "name=$STDLIB_NAME version=$VERSION"
log "src=$STDLIB_SRC"
log "stage=$PKG_ROOT"

# ----------------------------
# Copy stdlib sources
# ----------------------------
log "copying stdlib"
run rsync -a \
  --exclude ".git" \
  --exclude "target" \
  "$STDLIB_SRC/" "$PKG_ROOT/std/"

# ----------------------------
# Optional pruning
# ----------------------------
if [ "$INCLUDE_DOCS" = "0" ]; then
  log "excluding docs"
  rm -rf "$PKG_ROOT/std/docs" "$PKG_ROOT/std/doc" || true
fi

if [ "$INCLUDE_TESTS" = "0" ]; then
  log "excluding tests"
  rm -rf "$PKG_ROOT/std/tests" "$PKG_ROOT/std/test" || true
fi

# ----------------------------
# Metadata
# ----------------------------
log "writing metadata"
cat > "$PKG_ROOT/METADATA" <<EOF
name=$STDLIB_NAME
version=$VERSION
os=$OS
arch=$ARCH
built_at=$(date -u +"%Y-%m-%dT%H:%M:%SZ")
git_commit=$(git rev-parse --short HEAD 2>/dev/null || echo unknown)
source=$STDLIB_SRC
EOF

# ----------------------------
# Archive
# ----------------------------
ARCHIVE="$OUT_DIR/$STDLIB_NAME-$VERSION.tar.gz"
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
