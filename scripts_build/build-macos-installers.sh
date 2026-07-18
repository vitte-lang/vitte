#!/bin/sh
set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
VERSION=${VERSION:-$(tr -d ' \r\n' < "$ROOT_DIR/toolchain/scripts/package/PACKAGE_VERSION")}
OUT_DIR=${OUT_DIR:-$ROOT_DIR/pkgout}
ARCH=${ARCH:-all}
PKG_BUILDER=$ROOT_DIR/toolchain/scripts/package/make-macos-pkg.sh

die() {
  printf '[build-macos-installers][error] %s\n' "$*" >&2
  exit 1
}

require() {
  command -v "$1" >/dev/null 2>&1 || die "missing required tool: $1"
}

check_arch() {
  binary=$1
  expected=$2
  lipo -archs "$binary" | tr ' ' '\n' | grep -Fx "$expected" >/dev/null ||
    die "$binary does not contain required architecture $expected"
}

create_dmg() {
  package_file=$1
  dmg_file=$2
  volume_name=$3
  stage=$ROOT_DIR/target/macos-dmg-stage/$volume_name
  rm -rf "$stage"
  mkdir -p "$stage"
  cp "$package_file" "$stage/"
  hdiutil create -quiet -ov -format UDZO -fs HFS+ -volname "$volume_name" -srcfolder "$stage" "$dmg_file"
  [ -f "$dmg_file" ] || die "hdiutil did not create $dmg_file"
}

build_one() {
  label=$1
  binary=$2
  expected_archs=$3
  package_file=$OUT_DIR/vitte-$VERSION-macos-$label.pkg
  dmg_file=$OUT_DIR/vitte-$VERSION-macos-$label.dmg

  old_ifs=$IFS
  IFS=,
  for expected in $expected_archs; do
    check_arch "$binary" "$expected"
  done
  IFS=$old_ifs

  printf '[build-macos-installers] packaging %s from %s\n' "$label" "$binary"
  VERSION=$VERSION \
    OUT_DIR=$OUT_DIR \
    OUT_FILE_NAME=$(basename "$package_file") \
    VITTE_BIN_OVERRIDE=$binary \
    VITTEC_BIN_OVERRIDE=$binary \
    CHECKSUM_TARGET_BIN=$binary \
    "$PKG_BUILDER"
  [ -f "$package_file" ] || die "package builder did not create $package_file"
  create_dmg "$package_file" "$dmg_file" "Vitte-$VERSION-$label"
  (cd "$OUT_DIR" && shasum -a 256 "$(basename "$package_file")" > "$(basename "$package_file.sha256")")
  (cd "$OUT_DIR" && shasum -a 256 "$(basename "$dmg_file")" > "$(basename "$dmg_file.sha256")")
}

[ "$(uname -s)" = Darwin ] || die "macOS installers require a Darwin host"
for tool in make cc lipo pkgbuild productbuild hdiutil shasum; do
  require "$tool"
done
[ -x "$PKG_BUILDER" ] || die "missing package builder: $PKG_BUILDER"
mkdir -p "$OUT_DIR"

case "$ARCH" in
  all|arm64|x86_64|universal) ;;
  amd64) ARCH=x86_64 ;;
  *) die "unsupported macOS architecture: $ARCH" ;;
esac

make -C "$ROOT_DIR" macos-universal-bin

case "$ARCH" in
  all|arm64)
    build_one arm64 "$ROOT_DIR/target/macos-arm64/vitte" arm64
    ;;
esac
case "$ARCH" in
  all|x86_64)
    build_one x86_64 "$ROOT_DIR/target/macos-x86_64/vitte" x86_64
    ;;
esac
case "$ARCH" in
  all|universal)
    build_one universal "$ROOT_DIR/target/universal/vitte" arm64,x86_64
    ;;
esac

printf '[build-macos-installers] complete version=%s arch=%s out=%s\n' "$VERSION" "$ARCH" "$OUT_DIR"
