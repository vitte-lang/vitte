#!/bin/sh
set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
TMP_ROOT=${TMPDIR:-/tmp}/vitte-scripts-build-arch-$$
VERSION=$(tr -d ' \r\n' < "$ROOT_DIR/toolchain/scripts/package/PACKAGE_VERSION")

cleanup() {
  rm -rf "$TMP_ROOT"
}
trap cleanup EXIT HUP INT TERM

mkdir -p "$TMP_ROOT/bin" "$TMP_ROOT/out"

for command in vitte vittec vittec0; do
  printf '#!/bin/sh\ncase "$1" in --help) echo Vitte help; exit 0;; esac\nexit 0\n' > "$TMP_ROOT/bin/$command"
  chmod 0755 "$TMP_ROOT/bin/$command"
done

for arch in all amd64 x86_64 arm64 aarch64 i386 riscv64 RISC-V64; do
  ARCH=$arch "$ROOT_DIR/scripts_build/build-linux-debs.sh" --dry-run >/dev/null
done

for family in freebsd openbsd netbsd dragonfly; do
  BSD_FAMILY=$family BSD_RELEASE=14 ARCH=amd64 "$ROOT_DIR/scripts_build/build-bsd-installers.sh" --dry-run >/dev/null
done

BSD_FAMILY=freebsd BSD_RELEASE=14 ARCH=riscv64 "$ROOT_DIR/scripts_build/build-bsd-installers.sh" --dry-run >/dev/null

for arch in amd64 i386; do
  ARCH=$arch "$ROOT_DIR/scripts_build/build-solaris-package.sh" --dry-run >/dev/null
done

for arch in arm64 x86_64 universal; do
  ARCH=$arch "$ROOT_DIR/scripts_build/build-macos-installers.sh" --dry-run >/dev/null
done

for arch in amd64 i386 arm64 armv7; do
  ARCH=$arch "$ROOT_DIR/scripts_build/build-windows-installer.sh" --dry-run >/dev/null
done

env \
  VERSION=$VERSION \
  VITTE_BIN_AMD64="$TMP_ROOT/bin/vitte" \
  VITTE_VITTEC_AMD64="$TMP_ROOT/bin/vittec" \
  VITTE_VITTEC0_AMD64="$TMP_ROOT/bin/vittec0" \
  OUT_DIR="$TMP_ROOT/out" \
  BSD_FAMILY=freebsd \
  BSD_RELEASE=14 \
  ARCH=amd64 \
  "$ROOT_DIR/scripts_build/build-bsd-installers.sh" >/dev/null

archive=$TMP_ROOT/out/vitte-$VERSION-freebsd-14-amd64-installer.tar.xz
mkdir -p "$TMP_ROOT/extract" "$TMP_ROOT/install-root"
tar -xJf "$archive" -C "$TMP_ROOT/extract"
PREFIX="$TMP_ROOT/install-root/opt/vitte" "$TMP_ROOT/extract/install.sh" >/dev/null
test -x "$TMP_ROOT/install-root/opt/vitte/bin/vitte"
"$TMP_ROOT/install-root/opt/vitte/bin/vitte" --help >/dev/null
PREFIX="$TMP_ROOT/install-root/opt/vitte" "$TMP_ROOT/extract/uninstall.sh" >/dev/null
test ! -e "$TMP_ROOT/install-root/opt/vitte/bin/vitte"

space_root="$TMP_ROOT/path with spaces"
mkdir -p "$space_root/out"
env \
  VERSION=$VERSION \
  VITTE_BIN_AMD64="$TMP_ROOT/bin/vitte" \
  VITTE_VITTEC_AMD64="$TMP_ROOT/bin/vittec" \
  VITTE_VITTEC0_AMD64="$TMP_ROOT/bin/vittec0" \
  "$ROOT_DIR/scripts_build/stage-installer-payload.sh" "$space_root/out/linux payload" linux amd64 unix >/dev/null
test -s "$space_root/out/linux payload/usr/local/share/vitte/INSTALLATION.json"

case_dir="$TMP_ROOT/case-check"
mkdir -p "$case_dir"
printf 'canonical\n' > "$case_dir/filetypes.Vitte.conf"
if [ -e "$case_dir/filetypes.vitte.conf" ]; then
  test -e "$case_dir/filetypes.Vitte.conf"
else
  printf 'lowercase\n' > "$case_dir/filetypes.vitte.conf"
  test -e "$case_dir/filetypes.Vitte.conf"
  test -e "$case_dir/filetypes.vitte.conf"
fi

tar -tf "$archive" >/dev/null
/bin/sh -n "$TMP_ROOT/extract/install.sh"
/bin/sh -n "$TMP_ROOT/extract/uninstall.sh"

printf '[scripts-build-arch-matrix] OK\n'
