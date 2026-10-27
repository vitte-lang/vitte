#!/bin/sh
set -eu

# Build a self-contained Raspberry Pi installer from a native Pi build.

ROOT_DIR=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
SCRIPT_NAME=build-raspberry-installer
. "$ROOT_DIR/scripts/common.sh"
scripts_build_parse_common_flags "$@"

VERSION=${VERSION:-$(scripts_build_package_version)}
OUT_DIR=${OUT_DIR:-$ROOT_DIR/pkgout}
ARCH=${ARCH:-$(uname -m 2>/dev/null || printf '%s\n' unknown)}
VITTE_BIN=${VITTE_BIN:-$ROOT_DIR/build/bin/vitte}

case "$OUT_DIR" in
  /*) ;;
  *) OUT_DIR=$ROOT_DIR/$OUT_DIR ;;
esac

scripts_build_maybe_help \
  "usage: build-raspberry-installer.sh [--dry-run] [--list-targets] [--print-env]"

normalize_arch() {
  case "$1" in
    armv6|armv6l|armel) printf '%s\n' armv6 ;;
    armv7|armv7l|armhf) printf '%s\n' armv7 ;;
    aarch64|arm64|AArch64|ARM64) printf '%s\n' arm64 ;;
    *) scripts_build_die "unsupported Raspberry Pi architecture: $1" ;;
  esac
}

if [ "$LIST_TARGETS" -eq 1 ]; then
  printf '%s\n' 'raspberry armv6 tar.gz' 'raspberry armv7 tar.gz' 'raspberry arm64 tar.gz'
  exit 0
fi

ARCH=$(normalize_arch "$ARCH")

if [ "$PRINT_ENV" -eq 1 ]; then
  printf 'VERSION=%s\nOUT_DIR=%s\nARCH=%s\nVITTE_BIN=%s\n' \
    "$VERSION" "$OUT_DIR" "$ARCH" "$VITTE_BIN"
  exit 0
fi

scripts_build_maybe_dry_run \
  "would build Raspberry Pi installer version=$VERSION arch=$ARCH out=$OUT_DIR"

[ "$(uname -s)" = Linux ] ||
  scripts_build_die 'Raspberry Pi installers must be built on Linux using a native Pi build'

host_arch=$(normalize_arch "$(uname -m)")
[ "$host_arch" = "$ARCH" ] ||
  scripts_build_die "requested $ARCH but native host is $host_arch; build on the target Pi architecture"

for tool in tar find grep install mktemp; do
  scripts_build_require "$tool"
done
[ -x "$ROOT_DIR/scripts/stage-installer-payload.sh" ] ||
  scripts_build_die 'payload staging script is missing or not executable'

if [ ! -x "$VITTE_BIN" ]; then
  make -C "$ROOT_DIR" MODE=release all
fi
scripts_build_verify_release_manifest "$VITTE_BIN"

stage=$ROOT_DIR/target/installer-raspberry-$ARCH
root=$stage/root
archive=$OUT_DIR/vitte-$VERSION-raspberry-$ARCH-installer.tar.gz
checksum=$archive.sha256

rm -rf "$stage"
rm -f "$archive" "$checksum"
mkdir -p "$root" "$OUT_DIR"

VERSION=$VERSION VITTE_BIN=$VITTE_BIN \
  "$ROOT_DIR/scripts/stage-installer-payload.sh" \
  "$root" raspberry "$ARCH" unix

[ -d "$root/usr/local/share/vitte/editors" ] ||
  scripts_build_die 'installer is missing editor integrations'
[ -d "$root/usr/local/share/vitte/completions" ] ||
  scripts_build_die 'installer is missing shell completions'
scripts_build_verify_modules "$root/usr/local/share/vitte"

cat > "$stage/install.sh" <<'SH'
#!/bin/sh
set -eu
PREFIX=${PREFIX:-/usr/local}
DESTDIR=${DESTDIR:-}
case "$PREFIX" in /*) ;; *) printf 'PREFIX must be absolute: %s\n' "$PREFIX" >&2; exit 1 ;; esac
[ "$PREFIX" != / ] || { printf 'PREFIX must not be /\n' >&2; exit 1; }
HERE=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
[ -d "$HERE/root/usr/local" ] || { printf 'Vitte payload is missing\n' >&2; exit 1; }
target=${DESTDIR:-/}
if [ -z "$DESTDIR" ] && [ "$(id -u)" -ne 0 ] && [ ! -w "$PREFIX" ]; then
  printf 'installation requires root or a writable PREFIX\n' >&2
  exit 1
fi
mkdir -p "$target$PREFIX"
tar -cf - -C "$HERE/root/usr/local" . | tar -xf - -C "$target$PREFIX"
printf 'Vitte Raspberry Pi installed in %s%s\n' "$DESTDIR" "$PREFIX"
printf 'Compiler: %s/bin/vitte\n' "$PREFIX"
printf 'Editors:  %s/share/vitte/editors\n' "$PREFIX"
SH
chmod 0755 "$stage/install.sh"

cat > "$stage/uninstall.sh" <<'SH'
#!/bin/sh
set -eu
PREFIX=${PREFIX:-/usr/local}
DESTDIR=${DESTDIR:-}
case "$PREFIX" in /*) ;; *) printf 'PREFIX must be absolute: %s\n' "$PREFIX" >&2; exit 1 ;; esac
root=${DESTDIR:-/}
if [ -z "$DESTDIR" ] && [ "$(id -u)" -ne 0 ] && [ ! -w "$PREFIX" ]; then
  printf 'uninstallation requires root or a writable PREFIX\n' >&2
  exit 1
fi
rm -f "$root$PREFIX/bin/vitte" "$root$PREFIX/bin/vittec" "$root$PREFIX/bin/vitte-installer-doctor"
rm -rf "$root$PREFIX/libexec/vitte" "$root$PREFIX/share/vitte"
printf 'Vitte Raspberry Pi removed from %s%s\n' "$DESTDIR" "$PREFIX"
SH
chmod 0755 "$stage/uninstall.sh"

cat > "$stage/INSTALL.txt" <<EOF
Vitte $VERSION Raspberry Pi installer
Architecture: $ARCH

Install:          ./install.sh
Custom prefix:    PREFIX=\$HOME/.local ./install.sh
Staged install:   DESTDIR=/tmp/vitte-stage ./install.sh
Uninstall:        ./uninstall.sh

The payload includes the compiler, runtime, modules, documentation, examples,
editor integrations, shell completions and installer diagnostics.
EOF

scripts_build_tar_gz "$archive" "$stage" install.sh uninstall.sh INSTALL.txt root
scripts_build_sha256_write "$archive" "$checksum"
scripts_build_sha256_check "$archive" "$checksum"
scripts_build_write_artifact_manifest "$archive" raspberry "$ARCH" "$VERSION"

printf '[build-raspberry-installer] wrote %s\n' "$archive"
printf '[build-raspberry-installer] wrote %s\n' "$checksum"
