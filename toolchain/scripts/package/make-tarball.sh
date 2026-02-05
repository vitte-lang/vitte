#!/usr/bin/env bash
# ============================================================
# vitte — tarball maker
# Location: toolchain/scripts/package/make-tarball.sh
# ============================================================

set -euo pipefail

# ----------------------------
# Configuration
# ----------------------------
ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/../../.." && pwd)}"

SRC="${SRC:-}"                         # directory or file
OUT_DIR="${OUT_DIR:-$ROOT_DIR/target/packages}"
NAME="${NAME:-tarball}"               # base name
VERSION="${VERSION:-}"                # optional
COMPRESSION="${COMPRESSION:-gz}"      # gz | xz
DETERMINISTIC="${DETERMINISTIC:-1}"   # 1|0
INCLUDE_META="${INCLUDE_META:-1}"     # 1|0
CHECKSUM="${CHECKSUM:-sha256}"        # sha256 | sha512 | none
VERBOSE="${VERBOSE:-0}"

# Space-separated glob patterns
EXCLUDES="${EXCLUDES:-.git .DS_Store target}"

# ----------------------------
# Helpers
# ----------------------------
log() { printf "[make-tarball] %s\n" "$*"; }
die() { printf "[make-tarball][error] %s\n" "$*" >&2; exit 1; }

run() {
  if [ "$VERBOSE" = "1" ]; then log "exec: $*"; fi
  "$@"
}

has() { command -v "$1" >/dev/null 2>&1; }

checksum_cmd() {
  case "$CHECKSUM" in
    sha256)
      if has sha256sum; then echo "sha256sum"
      elif has shasum; then echo "shasum -a 256"
      else die "no sha256 tool found"
      fi
      ;;
    sha512)
      if has sha512sum; then echo "sha512sum"
      elif has shasum; then echo "shasum -a 512"
      else die "no sha512 tool found"
      fi
      ;;
    none) echo "" ;;
    *) die "unsupported CHECKSUM: $CHECKSUM" ;;
  esac
}

# ----------------------------
# Init
# ----------------------------
cd "$ROOT_DIR"
[ -n "$SRC" ] || die "SRC not set"
[ -e "$SRC" ] || die "SRC not found: $SRC"

mkdir -p "$OUT_DIR"

STAMP="$(date -u +"%Y%m%dT%H%M%SZ")"
BASE="$NAME"
[ -n "$VERSION" ] && BASE="$BASE-$VERSION"
EXT="tar.$COMPRESSION"
ARCHIVE="$OUT_DIR/$BASE.$EXT"

STAGE="$(mktemp -d)"
trap 'rm -rf "$STAGE"' EXIT
PAYLOAD="$STAGE/payload"
mkdir -p "$PAYLOAD"

# ----------------------------
# Stage payload
# ----------------------------
log "staging payload"
if [ -d "$SRC" ]; then
  run rsync -a \
    $(for e in $EXCLUDES; do printf -- "--exclude=%s " "$e"; done) \
    "$SRC/" "$PAYLOAD/"
else
  run cp -a "$SRC" "$PAYLOAD/"
fi

# ----------------------------
# Metadata
# ----------------------------
if [ "$INCLUDE_META" = "1" ]; then
  log "writing metadata"
  cat > "$PAYLOAD/METADATA" <<EOF
name=$NAME
version=${VERSION:-n/a}
compression=$COMPRESSION
built_at=$STAMP
git_commit=$(git rev-parse --short HEAD 2>/dev/null || echo unknown)
EOF
fi

# ----------------------------
# Determinism
# ----------------------------
if [ "$DETERMINISTIC" = "1" ]; then
  log "normalizing timestamps/owners"
  find "$PAYLOAD" -exec touch -h -t 197001010000 {} +
  TAR_OPTS=(--sort=name --owner=0 --group=0 --numeric-owner)
else
  TAR_OPTS=()
fi

# ----------------------------
# Create tarball
# ----------------------------
log "creating tarball: $ARCHIVE"
case "$COMPRESSION" in
  gz)
    run tar -czf "$ARCHIVE" "${TAR_OPTS[@]}" -C "$PAYLOAD" .
    ;;
  xz)
    run tar -cJf "$ARCHIVE" "${TAR_OPTS[@]}" -C "$PAYLOAD" .
    ;;
  *)
    die "unsupported COMPRESSION: $COMPRESSION"
    ;;
esac

# ----------------------------
# Checksum
# ----------------------------
if [ "$CHECKSUM" != "none" ]; then
  CMD="$(checksum_cmd)"
  log "writing checksum ($CHECKSUM)"
  (cd "$OUT_DIR" && $CMD "$(basename "$ARCHIVE")" > "$(basename "$ARCHIVE").$CHECKSUM")
fi

log "tarball ready"
echo "$ARCHIVE"