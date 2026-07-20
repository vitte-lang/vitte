#!/bin/sh
set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
SCRIPT_NAME=verify-installers
. "$ROOT_DIR/scripts_build/common.sh"
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
  scripts_build_sha256_check "$file" "$file.sha256"
}

verify_optional_sum() {
  file=$1
  [ ! -e "$file" ] || verify_sum "$file"
}

verify_deb() {
  arch=$1
  file=$OUT_DIR/vitte_${VERSION}_${arch}.deb
  verify_optional_sum "$file"
}

verify_freebsd() {
  arch=$1
  abi_arch=$2
  file=$OUT_DIR/vitte-${VERSION}-freebsd-${arch}.pkg
  [ -e "$file" ] || return 0
  verify_sum "$file"
  bsdtar -xOf "$file" +COMPACT_MANIFEST | python3 -c \
    'import json,sys; value=json.load(sys.stdin); expected=sys.argv[1]; assert value["abi"] == expected' \
    "FreeBSD:$FREEBSD_MAJOR:$abi_arch" || die "wrong FreeBSD ABI: $file"
}

for arch in amd64 arm64 armhf armel i386 riscv64 ppc64el s390x mips64el mipsel powerpc sparc64; do
  verify_deb "$arch"
done

for item in \
  'amd64 amd64' \
  'i386 i386' \
  'arm64 aarch64' \
  'armv7 armv7' \
  'armv6 armv6' \
  'riscv64 riscv64' \
  'powerpc powerpc' \
  'powerpc64 powerpc64' \
  'powerpc64le powerpc64le'
do
  set -- $item
  verify_freebsd "$1" "$2"
done

for bsd_kit in "$OUT_DIR"/vitte-"$VERSION"-*-*-installer.tar.xz; do
  [ -e "$bsd_kit" ] || continue
  verify_sum "$bsd_kit"
  scripts_build_tar_list_xz "$bsd_kit" | grep -Fx 'root/usr/local/share/vitte/assets/logo.png' >/dev/null ||
    die "missing BSD installer logo: $bsd_kit"
done

if [ "$(uname -s)" = Darwin ]; then
  for arch in arm64 x86_64 universal universal2 macos2006-i386; do
    verify_optional_sum "$OUT_DIR/vitte-${VERSION}-macos-${arch}.pkg"
    verify_optional_sum "$OUT_DIR/vitte-${VERSION}-macos-${arch}.dmg"
  done
fi

for arch in amd64 i386; do
  solaris_kit=$OUT_DIR/vitte-${VERSION}-solaris-${arch}-spool.tar.gz
  [ ! -e "$solaris_kit" ] || {
    verify_sum "$solaris_kit"
    tar -xOzf "$solaris_kit" pkginfo | grep -Fx "VITTE_PROCESSOR=$arch" >/dev/null ||
      die "invalid Solaris processor"
  }
done

for item in 'amd64 0x8664' 'i386 0x014c' 'arm64 0xaa64' 'armv7 0x01c4'; do
  set -- $item
  windows_kit=$OUT_DIR/vitte-${VERSION}-windows-$1-nsis.tar.gz
  [ ! -e "$windows_kit" ] || {
    verify_sum "$windows_kit"
    tar -xOzf "$windows_kit" BUILD.txt | grep -Fx "Processor: $1 (PE machine $2)" >/dev/null ||
      die "invalid Windows processor manifest"
    tar -tzf "$windows_kit" | grep -Fx 'payload/share/vitte/assets/logo.png' >/dev/null ||
      die "missing Windows installer logo"
  }
done

if [ "$STRICT_NATIVE" -eq 1 ]; then
  verify_sum "$OUT_DIR/vitte-${VERSION}-solaris-amd64.pkg"
  verify_sum "$OUT_DIR/vitte-${VERSION}-solaris-i386.pkg"
  verify_sum "$OUT_DIR/vitte-${VERSION}-windows-amd64-installer.exe"
  verify_sum "$OUT_DIR/vitte-${VERSION}-windows-i386-installer.exe"
else
  for arch in amd64 i386; do
    verify_optional_sum "$OUT_DIR/vitte-${VERSION}-solaris-${arch}.pkg"
  done
  for arch in amd64 i386 arm64 armv7; do
    verify_optional_sum "$OUT_DIR/vitte-${VERSION}-windows-${arch}-installer.exe"
  done
fi

printf '[verify-installers] OK version=%s strict_native=%s out=%s\n' "$VERSION" "$STRICT_NATIVE" "$OUT_DIR"
