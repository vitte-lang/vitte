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
VITTE_BIN=${VITTE_BIN:-$ROOT_DIR/build/bin/vitte}

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

for directory in docs examples editors completions assets; do
  [ ! -d "$ROOT_DIR/$directory" ] || scripts_build_copy_tree "$ROOT_DIR/$directory" "$prefix/share/vitte/$directory"
done

# Keep the distributable Vitte toolchain extensible across every installer
# family.  These trees are optional until the corresponding source surfaces
# are populated, but when present they must travel with the compiler.
if [ -d "$ROOT_DIR/include" ] &&
   find "$ROOT_DIR/include" -type f ! -name '.DS_Store' -print -quit |
   grep -q .
then
  scripts_build_copy_tree \
    "$ROOT_DIR/include" \
    "$prefix/share/vitte/include"
fi

if [ -d "$ROOT_DIR/src/runtime" ] &&
   find "$ROOT_DIR/src/runtime" -type f ! -name '.DS_Store' -print -quit |
   grep -q .
then
  scripts_build_require_file \
    "$ROOT_DIR/src/runtime/package.toml" \
    "Vitte runtime package metadata"
  scripts_build_require_file \
    "$ROOT_DIR/src/runtime/runtime.vit" \
    "Vitte runtime source"
  "$VITTE_BIN" check "$ROOT_DIR/src/runtime/runtime.vit"
  scripts_build_copy_tree \
    "$ROOT_DIR/src/runtime" \
    "$prefix/share/vitte/runtime"
fi

scripts_build_install_modules "$ROOT_DIR" "$prefix/share/vitte"
scripts_build_verify_modules "$prefix/share/vitte"
for file in README.md LICENSE CHANGELOG.md VERSION; do
  [ ! -f "$ROOT_DIR/$file" ] || install -m 0644 "$ROOT_DIR/$file" "$prefix/share/vitte/$file"
done

python3 - "$prefix/share/vitte/INSTALLATION.json" "$VERSION" "$PLATFORM" "$ARCH" <<'PY'
import json
import sys
from pathlib import Path

output = Path(sys.argv[1])
manifest = {
    "schema": "org.vitte.installation.v1",
    "name": "vitte",
    "version": sys.argv[2],
    "platform": sys.argv[3],
    "arch": sys.argv[4],
    "installed_commands": ["vitte", "vittec", "vitte-installer-doctor"],
    "installed_modules": sorted(
        path.parent.name
        for path in (output.parent / "modules").glob("*/package.toml")
    ),
    "installed_components": {
        "include": (output.parent / "include").is_dir(),
        "runtime": (output.parent / "runtime").is_dir(),
        "modules": (output.parent / "modules").is_dir(),
        "stdlib": (output.parent / "stdlib").is_dir(),
    },
}
output.write_text(json.dumps(manifest, indent=2, sort_keys=True) + "\n", encoding="utf-8")
PY

printf '[stage-installer-payload] staged native C compiler platform=%s arch=%s dest=%s version=%s\n' \
  "$PLATFORM" "$ARCH" "$DEST" "$VERSION"
