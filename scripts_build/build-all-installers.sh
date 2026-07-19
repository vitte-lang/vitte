#!/bin/sh
set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
VERSION=${VERSION:-$(tr -d ' \r\n' < "$ROOT_DIR/toolchain/scripts/package/PACKAGE_VERSION")}
OUT_DIR=${OUT_DIR:-$ROOT_DIR/pkgout}

run() {
  printf '[build-all-installers] %s\n' "$1"
  arch=$2
  shift 2
  VERSION=$VERSION OUT_DIR=$OUT_DIR ARCH=$arch "$@"
}

run 'Linux deb: amd64, arm64, armhf, armel, i386, riscv64, ppc64el, s390x, mips64el, mipsel, powerpc, sparc64' all "$ROOT_DIR/scripts_build/build-linux-debs.sh"
run 'FreeBSD pkg: amd64, i386, arm64, armv7, armv6, riscv64, powerpc, powerpc64, powerpc64le' all "$ROOT_DIR/scripts_build/build-freebsd-packages.sh"
run 'BSD portable installers: FreeBSD, OpenBSD, NetBSD, DragonFly, MidnightBSD, GhostBSD, HardenedBSD, NomadBSD, helloSystem releases and arches' all "$ROOT_DIR/scripts_build/build-bsd-installers.sh"

if [ "$(uname -s)" = Darwin ]; then
  run 'macOS pkg+dmg: arm64, x86_64, universal, universal2, MacOS2006 config and optional legacy i386' all "$ROOT_DIR/scripts_build/build-macos-installers.sh"
else
  printf '[build-all-installers] macOS pkg+dmg deferred: requires a Darwin host\n' >&2
fi

run 'Solaris SVR4: amd64, i386' all "$ROOT_DIR/scripts_build/build-solaris-package.sh"
run 'Windows NSIS kits and optional EXE: XP, Vista, 7, 8, 8.1, 10, 11 for i386, amd64, arm64, armv7' all "$ROOT_DIR/scripts_build/build-windows-installer.sh"
VERSION=$VERSION OUT_DIR=$OUT_DIR "$ROOT_DIR/scripts_build/verify-installers.sh"

printf '[build-all-installers] complete version=%s out=%s\n' "$VERSION" "$OUT_DIR"
