#!/usr/bin/env bash
# ============================================================
# vitte — CI artifacts collector
# Location: toolchain/scripts/ci/artifacts.sh
# ============================================================

set -euo pipefail

# ----------------------------
# Configuration
# ----------------------------
ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/../../.." && pwd)}"
ARTIFACTS_DIR="${ARTIFACTS_DIR:-$ROOT_DIR/target/ci-artifacts}"
BUILD_DIR="${BUILD_DIR:-$ROOT_DIR/target}"
STAMP="$(date -u +"%Y%m%dT%H%M%SZ")"
HOST="$(uname -s | tr '[:upper:]' '[:lower:]')-$(uname -m)"

# Optional CI vars
CI_NAME="${CI_NAME:-local}"
CI_COMMIT="${CI_COMMIT:-unknown}"
CI_REF="${CI_REF:-unknown}"

# ----------------------------
# Helpers
# ----------------------------
log() { printf "[artifacts] %s\n" "$*"; }

mkdir_clean() {
  rm -rf "$1"
  mkdir -p "$1"
}

copy_if_exists() {
  local src="$1" dst="$2"
  if [ -e "$src" ]; then
    mkdir -p "$(dirname "$dst")"
    cp -R "$src" "$dst"
  fi
}

# ----------------------------
# Layout
# ----------------------------
mkdir_clean "$ARTIFACTS_DIR"

META_DIR="$ARTIFACTS_DIR/meta"
LOGS_DIR="$ARTIFACTS_DIR/logs"
BINS_DIR="$ARTIFACTS_DIR/bin"
PKG_DIR="$ARTIFACTS_DIR/packages"

mkdir -p "$META_DIR" "$LOGS_DIR" "$BINS_DIR" "$PKG_DIR"

# ----------------------------
# Metadata
# ----------------------------
log "writing metadata"
cat > "$META_DIR/build.info" <<EOF
timestamp=$STAMP
host=$HOST
ci=$CI_NAME
commit=$CI_COMMIT
ref=$CI_REF
root=$ROOT_DIR
EOF

# ----------------------------
# Logs
# ----------------------------
log "collecting logs"
copy_if_exists "$BUILD_DIR/logs"            "$LOGS_DIR/logs"
copy_if_exists "$ROOT_DIR/.cache/vitte"     "$LOGS_DIR/cache"
copy_if_exists "$ROOT_DIR/target/debug.log" "$LOGS_DIR/debug.log"
copy_if_exists "$ROOT_DIR/target/build.log" "$LOGS_DIR/build.log"

# ----------------------------
# Binaries
# ----------------------------
log "collecting binaries"
copy_if_exists "$BUILD_DIR/debug"   "$BINS_DIR/debug"
copy_if_exists "$BUILD_DIR/release" "$BINS_DIR/release"
copy_if_exists "$ROOT_DIR/bin"      "$BINS_DIR/bin"

# ----------------------------
# Packages / Archives
# ----------------------------
log "collecting packages"
copy_if_exists "$BUILD_DIR/packages" "$PKG_DIR"
copy_if_exists "$BUILD_DIR/dist"     "$PKG_DIR/dist"

# ----------------------------
# Final archive (optional)
# ----------------------------
ARCHIVE="$ARTIFACTS_DIR/vitte-artifacts-$STAMP-$HOST.tar.gz"
log "creating archive: $(basename "$ARCHIVE")"
tar -czf "$ARCHIVE" -C "$ARTIFACTS_DIR" .

log "done"
echo "$ARCHIVE"