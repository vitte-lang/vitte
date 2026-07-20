#!/bin/sh
set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
TMP_ROOT=${TMPDIR:-/tmp}/vitte-scripts-build-staging-$$
VERSION=$(tr -d ' \r\n' < "$ROOT_DIR/toolchain/scripts/package/PACKAGE_VERSION")

cleanup() {
  rm -rf "$TMP_ROOT"
}
trap cleanup EXIT HUP INT TERM

mkdir -p "$TMP_ROOT/bin" "$TMP_ROOT/out"

for command in vitte vittec vittec0; do
  printf '#!/bin/sh\nexit 0\n' > "$TMP_ROOT/bin/$command"
  chmod 0755 "$TMP_ROOT/bin/$command"
done

stage_unix() {
  platform=$1
  arch=$2
  dest=$TMP_ROOT/out/$platform-$arch

  env \
    VERSION=$VERSION \
    VITTE_BIN_AMD64="$TMP_ROOT/bin/vitte" \
    VITTE_VITTEC_AMD64="$TMP_ROOT/bin/vittec" \
    VITTE_VITTEC0_AMD64="$TMP_ROOT/bin/vittec0" \
    VITTE_BIN_ARM64="$TMP_ROOT/bin/vitte" \
    VITTE_VITTEC_ARM64="$TMP_ROOT/bin/vittec" \
    VITTE_VITTEC0_ARM64="$TMP_ROOT/bin/vittec0" \
    "$ROOT_DIR/scripts_build/stage-installer-payload.sh" "$dest" "$platform" "$arch" unix

  test -x "$dest/usr/local/bin/vitte"
  test -x "$dest/usr/local/libexec/vitte/vitte"
  test -s "$dest/usr/local/share/vitte/INSTALLATION.json"
  test -s "$dest/usr/local/share/vitte/VERSION"
}

stage_windows() {
  dest=$TMP_ROOT/out/windows-amd64
  VERSION=$VERSION "$ROOT_DIR/scripts_build/stage-installer-payload.sh" "$dest" windows amd64 windows
  test -d "$dest/bin"
  test -s "$dest/share/vitte/INSTALLATION.json"
  test -s "$dest/share/vitte/VERSION"
}

"$ROOT_DIR/scripts_build/package-matrix.sh" list >/dev/null

stage_unix linux amd64
stage_unix freebsd amd64
stage_unix macos arm64
stage_unix solaris amd64
stage_windows

printf '[scripts-build-staging] OK\n'
