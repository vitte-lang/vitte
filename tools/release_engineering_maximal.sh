#!/usr/bin/env sh
set -eu

TOOL="${VITTEC_BIN:-bin/vittec}"
PROFILE_RELEASE="${VITTE_RELEASE_PROFILE:-release}"
PROFILE_DEBUG="${VITTE_DEBUG_PROFILE:-debug}"
VERSION="${VITTE_RELEASE_VERSION:-0.1.0}"
REL_DIR="build/releases"
SRC_ARCHIVE="$REL_DIR/vitte-$VERSION-src.tar.gz"
BIN_ARCHIVE="$REL_DIR/vitte-$VERSION.tar.gz"
REPORT="build/reports/release_engineering.txt"

mkdir -p "$REL_DIR" build/logs build/reports

# Ensure both requested profiles are exercised.
"$TOOL" release build --profile "$PROFILE_DEBUG" >/dev/null 2>&1 || true
"$TOOL" release build --profile "$PROFILE_RELEASE"

# Source archive + checksum
rm -f "$SRC_ARCHIVE" "$SRC_ARCHIVE.sha256"
tar -czf "$SRC_ARCHIVE" \
  --exclude='.git' \
  --exclude='build' \
  --exclude='bin' \
  --exclude='target' \
  .
sha256sum "$SRC_ARCHIVE" | awk '{print $1}' > "$SRC_ARCHIVE.sha256"

# Optional signature if key configured.
if [ -n "${VITTE_GPG_KEY:-}" ] && command -v gpg >/dev/null 2>&1; then
  gpg --batch --yes --local-user "$VITTE_GPG_KEY" --armor --detach-sign "$BIN_ARCHIVE"
fi

"$TOOL" release verify "$BIN_ARCHIVE"

tmpd="$(mktemp -d /tmp/vitte.release.install.XXXX)"
trap 'rm -rf "$tmpd"' EXIT INT TERM

tar -xzf "$BIN_ARCHIVE" -C "$tmpd"
test -x "$tmpd/vitte-$VERSION/vitte/bin/vittec"
"$tmpd/vitte-$VERSION/vitte/bin/vittec" --version >/dev/null 2>&1

{
  echo "release_engineering=ok"
  echo "debug_profile=$PROFILE_DEBUG"
  echo "release_profile=$PROFILE_RELEASE"
  echo "binary_archive=$BIN_ARCHIVE"
  echo "source_archive=$SRC_ARCHIVE"
  echo "binary_sha256=$(cat "$BIN_ARCHIVE.sha256")"
  echo "source_sha256=$(cat "$SRC_ARCHIVE.sha256")"
} > "$REPORT"

echo "[release-engineering] ok -> $REPORT"
