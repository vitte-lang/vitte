#!/bin/sh
set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
VERSION=${VERSION:-$(tr -d ' \r\n' < "$ROOT_DIR/toolchain/scripts/package/PACKAGE_VERSION")}
OUT_DIR=${OUT_DIR:-$ROOT_DIR/pkgout}
STRICT_NATIVE=${STRICT_NATIVE:-0}
FREEBSD_MAJOR=${FREEBSD_MAJOR:-14}

die() {
  printf '[verify-installers][error] %s\n' "$*" >&2
  exit 1
}

require_file() {
  [ -s "$1" ] || die "missing artifact: $1"
}

verify_sum() {
  file=$1
  require_file "$file"
  require_file "$file.sha256"
  (cd "$OUT_DIR" && shasum -a 256 -c "$(basename "$file.sha256")" >/dev/null)
}

verify_deb() {
  arch=$1
  file=$OUT_DIR/vitte_${VERSION}_${arch}.deb
  verify_sum "$file"
  members=$(ar -t "$file" | tr '\n' ' ')
  [ "$members" = 'debian-binary control.tar.gz data.tar.gz ' ] || die "invalid deb members: $file"
  ar -p "$file" control.tar.gz | tar -xzOf - ./control | grep -Fx "Architecture: $arch" >/dev/null ||
    die "wrong deb architecture: $file"
}

verify_freebsd() {
  arch=$1
  abi_arch=$2
  file=$OUT_DIR/vitte-${VERSION}-freebsd-${arch}.pkg
  verify_sum "$file"
  bsdtar -xOf "$file" +COMPACT_MANIFEST | python3 -c \
    'import json,sys; value=json.load(sys.stdin); expected=sys.argv[1]; assert value["abi"] == expected' \
    "FreeBSD:$FREEBSD_MAJOR:$abi_arch" || die "wrong FreeBSD ABI: $file"
}

verify_deb amd64
verify_deb arm64
verify_deb i386
verify_freebsd amd64 amd64
verify_freebsd i386 i386
verify_freebsd arm64 aarch64

for family in freebsd openbsd netbsd dragonfly; do
  for arch in amd64 i386; do
    bsd_kit=$OUT_DIR/vitte-${VERSION}-${family}-${arch}-installer.tar.xz
    verify_sum "$bsd_kit"
    tar -xJOf "$bsd_kit" INSTALL.txt | grep -Fx "Vitte $VERSION complete installer for $family $arch" >/dev/null ||
      die "invalid BSD installer metadata: $bsd_kit"
    tar -tJf "$bsd_kit" | grep -Fx 'root/usr/local/share/vitte/assets/logo.png' >/dev/null ||
      die "missing BSD installer logo: $bsd_kit"
  done
done

if [ "$(uname -s)" = Darwin ]; then
  for arch in arm64 x86_64 universal; do
    package=$OUT_DIR/vitte-${VERSION}-macos-${arch}.pkg
    dmg=$OUT_DIR/vitte-${VERSION}-macos-${arch}.dmg
    verify_sum "$package"
    verify_sum "$dmg"
    pkgutil --payload-files "$package" | grep -Fx './usr/local/bin/vitte' >/dev/null || die "invalid macOS pkg: $package"
    hdiutil verify "$dmg" >/dev/null || die "invalid macOS dmg: $dmg"
  done
fi

for arch in amd64 i386; do
  solaris_kit=$OUT_DIR/vitte-${VERSION}-solaris-${arch}-spool.tar.gz
  verify_sum "$solaris_kit"
  tar -xOzf "$solaris_kit" pkginfo | grep -Fx 'ARCH=i386' >/dev/null || die "invalid Solaris ARCH"
  tar -xOzf "$solaris_kit" pkginfo | grep -Fx "VITTE_PROCESSOR=$arch" >/dev/null || die "invalid Solaris processor"
done

for arch_machine in 'amd64 0x8664' 'i386 0x014c'; do
  set -- $arch_machine
  windows_kit=$OUT_DIR/vitte-${VERSION}-windows-$1-nsis.tar.gz
  verify_sum "$windows_kit"
  tar -xOzf "$windows_kit" BUILD.txt | grep -Fx "Processor: $1 (PE machine $2)" >/dev/null ||
    die "invalid Windows processor manifest"
  tar -tzf "$windows_kit" | grep -Fx 'payload/share/vitte/assets/logo.png' >/dev/null ||
    die "missing Windows installer logo"
done

if [ "$STRICT_NATIVE" -eq 1 ]; then
  verify_sum "$OUT_DIR/vitte-${VERSION}-solaris-amd64.pkg"
  verify_sum "$OUT_DIR/vitte-${VERSION}-solaris-i386.pkg"
  verify_sum "$OUT_DIR/vitte-${VERSION}-windows-amd64-installer.exe"
  verify_sum "$OUT_DIR/vitte-${VERSION}-windows-i386-installer.exe"
else
  for arch in amd64 i386; do
    solaris_pkg=$OUT_DIR/vitte-${VERSION}-solaris-${arch}.pkg
    [ ! -e "$solaris_pkg" ] || verify_sum "$solaris_pkg"
  done
  for arch in amd64 i386; do
    windows_exe=$OUT_DIR/vitte-${VERSION}-windows-${arch}-installer.exe
    [ ! -e "$windows_exe" ] || verify_sum "$windows_exe"
  done
fi

printf '[verify-installers] OK version=%s strict_native=%s\n' "$VERSION" "$STRICT_NATIVE"
