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
  test -d "$dest/usr/local/share/vitte/src/vitte/stdlib"
  test -d "$dest/usr/local/share/vitte/docs"
  test -d "$dest/usr/local/share/vitte/locales"
  test -d "$dest/usr/local/share/vitte/completions"
  test -d "$dest/usr/local/share/vitte/editors"
  test "$(python3 - "$dest/usr/local/bin/vitte" <<'PY'
import stat
import sys
from pathlib import Path
print(oct(stat.S_IMODE(Path(sys.argv[1]).stat().st_mode)))
PY
)" = "0o755"
  test "$(python3 - "$dest/usr/local/share/vitte/INSTALLATION.json" <<'PY'
import stat
import sys
from pathlib import Path
print(oct(stat.S_IMODE(Path(sys.argv[1]).stat().st_mode)))
PY
)" = "0o644"
  grep -F "/usr/local/libexec/vitte/vitte" "$dest/usr/local/bin/vitte" >/dev/null
  for command in vitte vittec vittec0; do
    grep -F 'export VITTE_ROOT=${VITTE_ROOT:-/usr/local/share/vitte}' "$dest/usr/local/bin/$command" >/dev/null
    grep -F "/usr/local/libexec/vitte/$command" "$dest/usr/local/bin/$command" >/dev/null
  done
}

stage_windows() {
  dest=$TMP_ROOT/out/windows-amd64
  VERSION=$VERSION "$ROOT_DIR/scripts_build/stage-installer-payload.sh" "$dest" windows amd64 windows
  test -d "$dest/bin"
  test -s "$dest/share/vitte/INSTALLATION.json"
  test -s "$dest/share/vitte/VERSION"
  test -d "$dest/share/vitte/src/vitte/stdlib"
  test -d "$dest/share/vitte/docs"
  test -d "$dest/share/vitte/locales"
  test -d "$dest/share/vitte/completions"
  test -d "$dest/share/vitte/editors"
  for command in vitte vittec vittec0; do
    test -s "$dest/bin/$command.cmd"
    test -s "$dest/bin/$command.ps1"
    grep -F 'set "VITTE_ROOT=%~dp0..\share\vitte"' "$dest/bin/$command.cmd" >/dev/null
    grep -F "if exist \"%~dp0$command.exe\"" "$dest/bin/$command.cmd" >/dev/null
    grep -F 'Join-Path $PSScriptRoot "..\share\vitte"' "$dest/bin/$command.ps1" >/dev/null
    grep -F "Join-Path \$PSScriptRoot \"$command.exe\"" "$dest/bin/$command.ps1" >/dev/null
  done
}

"$ROOT_DIR/scripts_build/package-matrix.sh" list >/dev/null

stage_unix linux amd64
stage_unix freebsd amd64
stage_unix macos arm64
stage_unix solaris amd64
stage_windows

printf '[scripts-build-staging] OK\n'
