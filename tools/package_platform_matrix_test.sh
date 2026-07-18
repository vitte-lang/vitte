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
expect amd64 normalize-arch AMD64
expect arm64 normalize-arch aarch64
expect i386 normalize-arch i686
expect macos normalize-os Darwin
expect freebsd normalize-os DragonFly
expect solaris normalize-os SunOS

$MATRIX lookup linux x86_64 deb >/dev/null
$MATRIX lookup macos arm64 dmg >/dev/null
$MATRIX lookup windows amd64 exe >/dev/null
$MATRIX lookup freebsd amd64 pkg >/dev/null
$MATRIX lookup solaris amd64 pkg >/dev/null

if $MATRIX lookup windows arm64 exe >/dev/null 2>&1; then
  echo '[package-platform-matrix][error] unsupported Windows arm64 tuple accepted' >&2
  exit 1
fi
if $MATRIX lookup solaris arm64 pkg >/dev/null 2>&1; then
  echo '[package-platform-matrix][error] unsupported Solaris arm64 tuple accepted' >&2
  exit 1
fi

rows=$($MATRIX list | wc -l | tr -d ' ')
[ "$rows" -eq 13 ] || {
  echo "[package-platform-matrix][error] expected 13 matrix rows, got $rows" >&2
  exit 1
}

echo '[package-platform-matrix] OK rows=13'
