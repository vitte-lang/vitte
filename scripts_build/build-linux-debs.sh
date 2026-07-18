#!/bin/sh
set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
VERSION=${VERSION:-$(tr -d ' \r\n' < "$ROOT_DIR/toolchain/scripts/package/PACKAGE_VERSION")}
OUT_DIR=${OUT_DIR:-$ROOT_DIR/pkgout}
ARCH=${ARCH:-all}
PACKAGE_NAME=${PACKAGE_NAME:-vitte}
SOURCE_DATE_EPOCH=${SOURCE_DATE_EPOCH:-$(git -C "$ROOT_DIR" log -1 --format=%ct 2>/dev/null || date +%s)}

die() {
  printf '[build-linux-debs][error] %s\n' "$*" >&2
  exit 1
}

require() {
  command -v "$1" >/dev/null 2>&1 || die "missing required tool: $1"
}

build_one() {
  arch=$1
  stage=$ROOT_DIR/target/installer-linux-$arch
  control_root=$stage/control
  data_root=$stage/data
  package_file=$OUT_DIR/${PACKAGE_NAME}_${VERSION}_${arch}.deb

  rm -rf "$stage"
  mkdir -p "$control_root" "$data_root" "$OUT_DIR"
  VERSION=$VERSION "$ROOT_DIR/scripts_build/stage-installer-payload.sh" "$data_root" linux "$arch" unix
  installed_size=$(du -sk "$data_root" | awk '{print $1}')
  cat > "$control_root/control" <<EOF
Package: $PACKAGE_NAME
Version: $VERSION
Section: devel
Priority: optional
Architecture: $arch
Maintainer: Vitte Team <maintainers@vitte-lang.org>
Depends: bash, python3, make
Recommends: git, clang | gcc
Suggests: vim | neovim, emacs, nano, geany
Provides: vitte-compiler, vitte-toolchain
Installed-Size: $installed_size
Homepage: https://vitte-lang.org/
Vcs-Browser: https://github.com/vitte-lang/vitte
X-Vitte-Processor: $arch
Description: Complete Vitte systems language toolchain
 Compiler, runtime, standard library, sources, documentation, examples,
 editor support, shell completions, locales, and visual assets.
EOF
  python3 - "$data_root" "$control_root/md5sums" <<'PY'
import hashlib
import sys
from pathlib import Path

root = Path(sys.argv[1])
output = Path(sys.argv[2])
lines = []
for path in sorted(root.rglob("*")):
    if path.is_file():
        lines.append(f"{hashlib.md5(path.read_bytes()).hexdigest()}  {path.relative_to(root).as_posix()}")
output.write_text("\n".join(lines) + "\n", encoding="utf-8")
PY
  cat > "$control_root/postinst" <<'EOF'
#!/bin/sh
set -eu
chmod 0755 /usr/local/libexec/vitte/vitte /usr/local/libexec/vitte/vittec /usr/local/libexec/vitte/vittec0
exit 0
EOF
  chmod 0755 "$control_root/postinst"

  printf '2.0\n' > "$stage/debian-binary"
  COPYFILE_DISABLE=1 bsdtar --uid 0 --gid 0 --uname root --gname root -czf "$stage/control.tar.gz" -C "$control_root" .
  COPYFILE_DISABLE=1 bsdtar --uid 0 --gid 0 --uname root --gname root -czf "$stage/data.tar.gz" -C "$data_root" .
  rm -f "$package_file"
  (cd "$stage" && ar -qcS "$package_file" debian-binary control.tar.gz data.tar.gz)

  members=$(ar -t "$package_file" | tr '\n' ' ')
  [ "$members" = 'debian-binary control.tar.gz data.tar.gz ' ] || die "invalid Debian member order: $members"
  (cd "$OUT_DIR" && shasum -a 256 "$(basename "$package_file")" > "$(basename "$package_file.sha256")")
  printf '[build-linux-debs] wrote %s (%s bytes)\n' "$package_file" "$(wc -c < "$package_file" | tr -d ' ')"
}

for tool in ar awk bsdtar du install shasum tar; do
  require "$tool"
done

case "$ARCH" in
  all)
    build_one amd64
    build_one arm64
    build_one i386
    ;;
  x86_64|amd64) build_one amd64 ;;
  aarch64|arm64) build_one arm64 ;;
  i386|i486|i586|i686|x86) build_one i386 ;;
  *) die "unsupported Linux architecture: $ARCH" ;;
esac

printf '[build-linux-debs] complete version=%s arch=%s out=%s\n' "$VERSION" "$ARCH" "$OUT_DIR"
