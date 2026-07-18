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
  cp "$ROOT_DIR/assets/logo.png" "$stage/Vitte-logo.png"
  cat > "$stage/INSTALL.txt" <<EOF
Vitte $VERSION for macOS ($volume_name)

Open $(basename "$package_file") to install the complete language toolchain in /usr/local.
The package includes compiler commands, sources, stdlib, documentation, examples,
editor integrations, completions, locales, and uninstall support.
EOF
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
  component_dir=$ROOT_DIR/target/macos-product-$label
  resources=$component_dir/resources
  component_pkg=$component_dir/vitte-component.pkg
  mkdir -p "$resources"
  mv "$package_file" "$component_pkg"
  cp "$ROOT_DIR/assets/logo.png" "$resources/logo.png"
  cat > "$component_dir/Distribution.xml" <<EOF
<?xml version="1.0" encoding="utf-8"?>
<installer-gui-script minSpecVersion="1">
  <title>Vitte $VERSION</title>
  <background file="logo.png" alignment="center" scaling="proportional"/>
  <options customize="never" require-scripts="false" hostArchitectures="$expected_archs"/>
  <choices-outline><line choice="vitte"/></choices-outline>
  <choice id="vitte" visible="false"><pkg-ref id="org.vitte.toolchain"/></choice>
  <pkg-ref id="org.vitte.toolchain" version="$VERSION" onConclusion="none">vitte-component.pkg</pkg-ref>
</installer-gui-script>
EOF
  productbuild --distribution "$component_dir/Distribution.xml" --resources "$resources" \
    --package-path "$component_dir" "$package_file"
  create_dmg "$package_file" "$dmg_file" "Vitte-$VERSION-$label"
  (cd "$OUT_DIR" && shasum -a 256 "$(basename "$package_file")" > "$(basename "$package_file.sha256")")
  (cd "$OUT_DIR" && shasum -a 256 "$(basename "$dmg_file")" > "$(basename "$dmg_file.sha256")")
}

create_macos2006_profile() {
  stage=$ROOT_DIR/target/macos2006-profile
  archive=$OUT_DIR/vitte-$VERSION-macos2006-config.tar.gz
  rm -rf "$stage"
  mkdir -p "$stage"
  cp "$ROOT_DIR/assets/logo.png" "$stage/logo.png"
  cat > "$stage/MacOS2006.conf" <<'EOF'
MACOSX_DEPLOYMENT_TARGET=10.4
VITTE_MACOS_LEGACY_SDK=MacOSX10.4u.sdk
VITTE_MACOS_LEGACY_ARCHS=i386,ppc
VITTE_MACOS_LEGACY_CC=gcc-4.0
EOF
  cat > "$stage/README.txt" <<EOF
Vitte $VERSION historical macOS 2006 build profile

This configuration targets Mac OS X 10.4-era i386/PowerPC toolchains and requires
an original Xcode 2.x SDK. It is a reproducible configuration kit, not a claim
that current arm64/x86_64 Vitte binaries execute on 2006 systems.
EOF
  COPYFILE_DISABLE=1 tar -czf "$archive" -C "$stage" .
  (cd "$OUT_DIR" && shasum -a 256 "$(basename "$archive")" > "$(basename "$archive.sha256")")
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
create_macos2006_profile

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
