#!/bin/sh
set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
VERSION=${VERSION:-$(tr -d ' \r\n' < "$ROOT_DIR/toolchain/scripts/package/PACKAGE_VERSION")}
OUT_DIR=${OUT_DIR:-$ROOT_DIR/pkgout}
ARCH=${ARCH:-all}
PACKAGE_NAME=${PACKAGE_NAME:-vitte}
FREEBSD_MAJOR=${FREEBSD_MAJOR:-14}

die() {
  printf '[build-freebsd-packages][error] %s\n' "$*" >&2
  exit 1
}

require() {
  command -v "$1" >/dev/null 2>&1 || die "missing required tool: $1"
}

copy_tree() {
  source=$1
  destination=$2
  [ -e "$source" ] || return 0
  mkdir -p "$destination"
  COPYFILE_DISABLE=1 tar -cf - -C "$source" . | tar -xf - -C "$destination"
}

stage_payload() {
  data_root=$1
  mkdir -p "$data_root/usr/local/bin" "$data_root/usr/local/libexec/vitte" "$data_root/usr/local/share/vitte"
  for command in vitte vittec vittec0; do
    [ -x "$ROOT_DIR/bin/$command" ] || die "missing executable payload: bin/$command"
    install -m 0755 "$ROOT_DIR/bin/$command" "$data_root/usr/local/libexec/vitte/$command"
    cat > "$data_root/usr/local/bin/$command" <<EOF
#!/bin/sh
set -eu
export VITTE_ROOT=\${VITTE_ROOT:-/usr/local/share/vitte}
exec /usr/local/libexec/vitte/$command "\$@"
EOF
    chmod 0755 "$data_root/usr/local/bin/$command"
  done
  copy_tree "$ROOT_DIR/src/vitte" "$data_root/usr/local/share/vitte/src/vitte"
  copy_tree "$ROOT_DIR/toolchain/seed" "$data_root/usr/local/share/vitte/toolchain/seed"
  copy_tree "$ROOT_DIR/locales" "$data_root/usr/local/share/vitte/locales"
  copy_tree "$ROOT_DIR/completions" "$data_root/usr/local/share/vitte/completions"
}

build_one() {
  arch=$1
  case "$arch" in
    amd64) freebsd_arch=amd64 ;;
    arm64) freebsd_arch=aarch64 ;;
    *) die "unsupported FreeBSD architecture: $arch" ;;
  esac
  abi=FreeBSD:$FREEBSD_MAJOR:$freebsd_arch
  stage=$ROOT_DIR/target/installer-freebsd-$arch
  metadata=$stage/metadata
  data_root=$stage/data
  package_file=$OUT_DIR/${PACKAGE_NAME}-${VERSION}-freebsd-${arch}.pkg

  rm -rf "$stage"
  mkdir -p "$metadata" "$data_root" "$OUT_DIR"
  stage_payload "$data_root"
  python3 - "$data_root" "$metadata" "$PACKAGE_NAME" "$VERSION" "$abi" <<'PY'
import hashlib
import json
import os
import stat
import sys
from pathlib import Path

data_root = Path(sys.argv[1])
metadata = Path(sys.argv[2])
name, version, abi = sys.argv[3:6]
files = {}
directories = set()
for path in sorted(data_root.rglob("*")):
    relative = "/" + path.relative_to(data_root).as_posix()
    if path.is_dir():
        directories.add(relative)
        continue
    digest = hashlib.sha256(path.read_bytes()).hexdigest()
    files[relative] = "1$" + digest
manifest = {
    "name": name,
    "version": version,
    "origin": "devel/vitte",
    "comment": "Vitte systems language toolchain",
    "maintainer": "maintainers@vitte-lang.org",
    "www": "https://vitte-lang.org/",
    "abi": abi,
    "arch": abi,
    "prefix": "/usr/local",
    "flatsize": sum(path.stat().st_size for path in data_root.rglob("*") if path.is_file()),
    "desc": "Processor-specific Vitte compiler, runtime, standard library, and tools.",
    "categories": ["devel", "lang"],
    "licenses": ["GPLv3"],
    "files": files,
    "directories": {directory: "y" for directory in sorted(directories)},
}
text = json.dumps(manifest, ensure_ascii=True, separators=(",", ":"), sort_keys=True) + "\n"
(metadata / "+COMPACT_MANIFEST").write_text(text, encoding="utf-8")
(metadata / "+MANIFEST").write_text(json.dumps(manifest, indent=2, sort_keys=True) + "\n", encoding="utf-8")
PY
  COPYFILE_DISABLE=1 bsdtar -cJf "$package_file" -C "$metadata" +COMPACT_MANIFEST +MANIFEST -C "$data_root" .
  bsdtar -tf "$package_file" | grep -Fx '+COMPACT_MANIFEST' >/dev/null || die "missing compact manifest"
  bsdtar -tf "$package_file" | grep -Fx './usr/local/bin/vitte' >/dev/null || die "missing vitte command"
  shasum -a 256 "$package_file" > "$package_file.sha256"
  printf '[build-freebsd-packages] wrote %s (%s bytes)\n' "$package_file" "$(wc -c < "$package_file" | tr -d ' ')"
}

for tool in bsdtar install python3 shasum tar; do
  require "$tool"
done

case "$ARCH" in
  all) build_one amd64; build_one arm64 ;;
  x86_64|amd64) build_one amd64 ;;
  aarch64|arm64) build_one arm64 ;;
  *) die "unsupported FreeBSD architecture: $ARCH" ;;
esac

printf '[build-freebsd-packages] complete version=%s arch=%s out=%s\n' "$VERSION" "$ARCH" "$OUT_DIR"
