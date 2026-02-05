# package/make-archive.sh
#!/usr/bin/env bash
# ============================================================
# vitte — generic archive maker
# Location: toolchain/scripts/package/make-archive.sh
# ============================================================

set -euo pipefail

# ----------------------------
# Configuration
# ----------------------------
ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/../../.." && pwd)}"

SRC="${SRC:-}"                         # file or directory to archive
OUT_DIR="${OUT_DIR:-$ROOT_DIR/target/packages}"
NAME="${NAME:-archive}"               # base name (without extension)
VERSION="${VERSION:-}"                # optional
FORMAT="${FORMAT:-tar.gz}"            # tar.gz | tar.xz | zip
DETERMINISTIC="${DETERMINISTIC:-1}"   # 1|0 (mtime/owner normalization)
INCLUDE_META="${INCLUDE_META:-1}"     # 1|0
CHECKSUM="${CHECKSUM:-sha256}"        # sha256 | sha512 | none
VERBOSE="${VERBOSE:-0}"

# Exclusions (space-separated glob patterns)
EXCLUDES="${EXCLUDES:-.git .DS_Store target}"

# ----------------------------
# Helpers
# ----------------------------
log() { printf "[make-archive] %s\n" "$*"; }
die() { printf "[make-archive][error] %s\n" "$*" >&2; exit 1; }

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
ARCHIVE="$OUT_DIR/$BASE.$FORMAT"

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
format=$FORMAT
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
fi

# ----------------------------
# Create archive
# ----------------------------
log "creating archive: $ARCHIVE"
case "$FORMAT" in
  tar.gz)
    TAR_OPTS=(--sort=name --owner=0 --group=0 --numeric-owner)
    run tar -czf "$ARCHIVE" "${TAR_OPTS[@]}" -C "$PAYLOAD" .
    ;;
  tar.xz)
    TAR_OPTS=(--sort=name --owner=0 --group=0 --numeric-owner)
    run tar -cJf "$ARCHIVE" "${TAR_OPTS[@]}" -C "$PAYLOAD" .
    ;;
  zip)
    (cd "$PAYLOAD" && run zip -qr "$ARCHIVE" .)
    ;;
  *)
    die "unsupported FORMAT: $FORMAT"
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

log "archive ready"
echo "$ARCHIVE"