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

run 'Linux deb: amd64, arm64, i386' all "$ROOT_DIR/scripts_build/build-linux-debs.sh"
run 'FreeBSD pkg: amd64, i386, arm64' all "$ROOT_DIR/scripts_build/build-freebsd-packages.sh"
run 'BSD complete installers: FreeBSD, OpenBSD, NetBSD, DragonFly amd64+i386' all "$ROOT_DIR/scripts_build/build-bsd-installers.sh"

if [ "$(uname -s)" = Darwin ]; then
  run 'macOS pkg+dmg: arm64, x86_64, universal' all "$ROOT_DIR/scripts_build/build-macos-installers.sh"
else
  printf '[build-all-installers] macOS pkg+dmg deferred: requires a Darwin host\n' >&2
fi

run 'Solaris SVR4: amd64, i386' all "$ROOT_DIR/scripts_build/build-solaris-package.sh"
run 'Windows NSIS: amd64' amd64 "$ROOT_DIR/scripts_build/build-windows-installer.sh"
VERSION=$VERSION OUT_DIR=$OUT_DIR "$ROOT_DIR/scripts_build/verify-installers.sh"

printf '[build-all-installers] complete version=%s out=%s\n' "$VERSION" "$OUT_DIR"
