#!/bin/sh
set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
MATRIX=$ROOT_DIR/scripts_build/package-matrix.sh

expect() {
  expected=$1
  shift
  actual=$($MATRIX "$@")
  [ "$actual" = "$expected" ] || {
    printf '[package-platform-matrix][error] expected `%s`, got `%s`\n' "$expected" "$actual" >&2
    exit 1
  }
}

expect amd64 normalize-arch x86_64
expect amd64 normalize-arch X86_64
expect amd64 normalize-arch AMD64
expect arm64 normalize-arch aarch64
expect arm64 normalize-arch AArch64
expect i386 normalize-arch i686
expect riscv64 normalize-arch RISC-V64
expect macos normalize-os Darwin
expect dragonfly normalize-os DragonFly
expect openbsd normalize-os OpenBSD
expect netbsd normalize-os NetBSD
expect solaris normalize-os SunOS

$MATRIX lookup linux x86_64 deb >/dev/null
$MATRIX lookup linux AArch64 deb >/dev/null
$MATRIX lookup linux RISC-V64 deb >/dev/null
$MATRIX lookup macos arm64 dmg >/dev/null
$MATRIX lookup macos x86_64 pkg >/dev/null
$MATRIX lookup windows-11 amd64 exe >/dev/null
$MATRIX lookup windows-xp i386 exe >/dev/null
$MATRIX lookup freebsd amd64 pkg >/dev/null
$MATRIX lookup freebsd i386 pkg >/dev/null
$MATRIX lookup freebsd RISC-V64 pkg >/dev/null
$MATRIX lookup openbsd i386 tar.xz >/dev/null
$MATRIX lookup netbsd amd64 tar.xz >/dev/null
$MATRIX lookup dragonfly amd64 tar.xz >/dev/null
$MATRIX lookup solaris amd64 pkg >/dev/null
$MATRIX lookup solaris i386 pkg >/dev/null
$MATRIX lookup macos macos2006 config >/dev/null

if $MATRIX lookup windows arm64 exe >/dev/null 2>&1; then
  echo '[package-platform-matrix][error] unsupported Windows arm64 tuple accepted' >&2
  exit 1
fi
if $MATRIX lookup solaris arm64 pkg >/dev/null 2>&1; then
  echo '[package-platform-matrix][error] unsupported Solaris arm64 tuple accepted' >&2
  exit 1
fi

rows=$($MATRIX list | wc -l | tr -d ' ')
[ "$rows" -eq 45 ] || {
  echo "[package-platform-matrix][error] expected 45 matrix rows, got $rows" >&2
  exit 1
}

echo '[package-platform-matrix] OK rows=45'
