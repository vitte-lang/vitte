#!/bin/sh
set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
VERSION=${VERSION:-$(tr -d ' \r\n' < "$ROOT_DIR/toolchain/scripts/package/PACKAGE_VERSION")}
OUT_DIR=${OUT_DIR:-$ROOT_DIR/pkgout}
ARCH=${ARCH:-amd64}
PACKAGE_NAME=${PACKAGE_NAME:-vitte}
SVR4_PACKAGE=${SVR4_PACKAGE:-VITTE}

die() {
  printf '[build-solaris-package][error] %s\n' "$*" >&2
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

case "$ARCH" in
  x86_64|amd64) ARCH=amd64 ;;
  *) die "unsupported Solaris architecture: $ARCH (only amd64 is supported)" ;;
esac

for tool in install python3 shasum tar; do
  require "$tool"
done

stage=$ROOT_DIR/target/installer-solaris-$ARCH
metadata=$stage/metadata
data_root=$stage/root
spool=$stage/spool
package_file=$OUT_DIR/${PACKAGE_NAME}-${VERSION}-solaris-${ARCH}.pkg
kit_file=$OUT_DIR/${PACKAGE_NAME}-${VERSION}-solaris-${ARCH}-spool.tar.gz

rm -rf "$stage"
mkdir -p "$metadata" "$data_root" "$spool" "$OUT_DIR"
stage_payload "$data_root"

cat > "$metadata/pkginfo" <<EOF
PKG=$SVR4_PACKAGE
NAME=Vitte systems language toolchain
ARCH=i386
VERSION=$VERSION
CATEGORY=application,development
VENDOR=Vitte Team
EMAIL=maintainers@vitte-lang.org
HOTLINE=https://vitte-lang.org/
BASEDIR=/
VITTE_PROCESSOR=amd64
DESC=Processor-specific Vitte compiler, runtime, standard library, and tools.
EOF

python3 - "$data_root" "$metadata/prototype" <<'PY'
import os
import stat
import sys
from pathlib import Path

root = Path(sys.argv[1])
prototype = Path(sys.argv[2])
lines = ["i pkginfo"]
for path in sorted(root.rglob("*")):
    relative = path.relative_to(root).as_posix()
    mode = stat.S_IMODE(path.lstat().st_mode)
    if path.is_symlink():
        lines.append(f"s none {relative}={os.readlink(path)}")
    elif path.is_dir():
        lines.append(f"d none {relative} {mode:04o} root sys")
    elif path.is_file():
        lines.append(f"f none {relative} {mode:04o} root bin")
prototype.write_text("\n".join(lines) + "\n", encoding="ascii")
PY

cp "$metadata/pkginfo" "$spool/pkginfo"
cp "$metadata/prototype" "$spool/prototype"
mkdir -p "$spool/root"
copy_tree "$data_root" "$spool/root"
COPYFILE_DISABLE=1 tar -czf "$kit_file" -C "$spool" pkginfo prototype root
(cd "$OUT_DIR" && shasum -a 256 "$(basename "$kit_file")" > "$(basename "$kit_file.sha256")")
printf '[build-solaris-package] wrote build kit %s (%s bytes)\n' "$kit_file" "$(wc -c < "$kit_file" | tr -d ' ')"

if command -v pkgmk >/dev/null 2>&1 && command -v pkgtrans >/dev/null 2>&1; then
  package_spool=$stage/package-spool
  mkdir -p "$package_spool"
  pkgmk -o -r "$data_root" -d "$package_spool" -f "$metadata/prototype"
  rm -f "$package_file"
  pkgtrans -s "$package_spool" "$package_file" "$SVR4_PACKAGE"
  [ -s "$package_file" ] || die "pkgtrans did not create $package_file"
  (cd "$OUT_DIR" && shasum -a 256 "$(basename "$package_file")" > "$(basename "$package_file.sha256")")
  printf '[build-solaris-package] wrote %s (%s bytes)\n' "$package_file" "$(wc -c < "$package_file" | tr -d ' ')"
else
  printf '[build-solaris-package] pkgmk/pkgtrans unavailable; portable SVR4 kit generated, native .pkg deferred\n' >&2
fi

printf '[build-solaris-package] complete version=%s arch=%s out=%s\n' "$VERSION" "$ARCH" "$OUT_DIR"
