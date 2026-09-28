#!/bin/sh
set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
SCRIPT_NAME=stage-installer-payload
. "$ROOT_DIR/scripts/common.sh"

if [ "${1:-}" = --dry-run ]; then
  shift
  printf '[stage-installer-payload][dry-run] native C payload: %s\n' "${1:-DEST}"
  exit 0
fi
if [ "${1:-}" = --help ] || [ "${1:-}" = -h ]; then
  printf '%s\n' 'usage: stage-installer-payload.sh [--dry-run] DEST PLATFORM ARCH [unix|windows]'
  exit 0
fi

DEST=${1:?missing destination}
PLATFORM=${2:?missing platform}
ARCH=${3:?missing architecture}
LAYOUT=${4:-unix}
VERSION=${VERSION:-$(scripts_build_package_version)}
VITTE_BIN=${VITTE_BIN:-$ROOT_DIR/target/vitte/vitte}

if [ ! -x "$VITTE_BIN" ]; then
  make -C "$ROOT_DIR" all
fi
verify_vitte "$VITTE_BIN" >/dev/null

case "$LAYOUT" in
  unix)
    prefix=$DEST/usr/local
    mkdir -p "$prefix/bin" "$prefix/libexec/vitte" "$prefix/share/vitte"
    for command in vitte vittec; do
      install -m 0755 "$VITTE_BIN" "$prefix/libexec/vitte/$command"
      {
        printf '%s\n' '#!/bin/sh' 'set -eu'
        printf '%s\n' 'self=$0' 'bindir=${self%/*}'
        printf '%s\n' 'case "$bindir" in /*) ;; *) bindir=$(CDPATH= cd -- "$bindir" && pwd) ;; esac'
        printf '%s\n' 'prefix=${VITTE_INSTALL_PREFIX:-${bindir%/bin}}'
        printf 'exec "$prefix/libexec/vitte/%s" "$@"\n' "$command"
      } > "$prefix/bin/$command"
      chmod 0755 "$prefix/bin/$command"
    done
    install -m 0755 "$ROOT_DIR/scripts/installer-doctor.sh" "$prefix/bin/vitte-installer-doctor"
    ;;
  windows)
    prefix=$DEST
    mkdir -p "$prefix/bin" "$prefix/share/vitte"
    install -m 0755 "$VITTE_BIN" "$prefix/bin/vitte.exe"
    install -m 0755 "$VITTE_BIN" "$prefix/bin/vittec.exe"
    ;;
  *)
    scripts_build_die "unsupported layout: $LAYOUT"
    ;;
esac

for directory in docs examples modules editors completions; do
  [ ! -d "$ROOT_DIR/$directory" ] || scripts_build_copy_tree "$ROOT_DIR/$directory" "$prefix/share/vitte/$directory"
done
for file in README.md LICENSE CHANGELOG.md VERSION; do
  [ ! -f "$ROOT_DIR/$file" ] || install -m 0644 "$ROOT_DIR/$file" "$prefix/share/vitte/$file"
done

printf '[stage-installer-payload] staged native C compiler platform=%s arch=%s dest=%s version=%s\n' \
  "$PLATFORM" "$ARCH" "$DEST" "$VERSION"
