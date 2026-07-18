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

copy_tree() {
  source=$1
  destination=$2
  [ -e "$source" ] || return 0
  mkdir -p "$destination"
  COPYFILE_DISABLE=1 tar -cf - -C "$source" . | tar -xf - -C "$destination"
}

write_wrappers() {
  data_root=$1
  mkdir -p "$data_root/usr/local/bin"
  for command in vitte vittec vittec0; do
    cat > "$data_root/usr/local/bin/$command" <<EOF
#!/bin/sh
set -eu
export VITTE_ROOT=\${VITTE_ROOT:-/usr/local/share/vitte}
exec /usr/local/libexec/vitte/$command "\$@"
EOF
    chmod 0755 "$data_root/usr/local/bin/$command"
  done
}

stage_payload() {
  data_root=$1
  mkdir -p "$data_root/usr/local/libexec/vitte" "$data_root/usr/local/share/vitte"
  for command in vitte vittec vittec0; do
    [ -x "$ROOT_DIR/bin/$command" ] || die "missing executable payload: bin/$command"
    install -m 0755 "$ROOT_DIR/bin/$command" "$data_root/usr/local/libexec/vitte/$command"
  done
  write_wrappers "$data_root"
  copy_tree "$ROOT_DIR/src/vitte" "$data_root/usr/local/share/vitte/src/vitte"
  copy_tree "$ROOT_DIR/toolchain/seed" "$data_root/usr/local/share/vitte/toolchain/seed"
  copy_tree "$ROOT_DIR/locales" "$data_root/usr/local/share/vitte/locales"
  copy_tree "$ROOT_DIR/completions" "$data_root/usr/local/share/vitte/completions"
  if [ -d "$ROOT_DIR/man" ]; then
    copy_tree "$ROOT_DIR/man" "$data_root/usr/local/share/man/man1"
  fi
}

build_one() {
  arch=$1
  stage=$ROOT_DIR/target/installer-linux-$arch
  control_root=$stage/control
  data_root=$stage/data
  package_file=$OUT_DIR/${PACKAGE_NAME}_${VERSION}_${arch}.deb

  rm -rf "$stage"
  mkdir -p "$control_root" "$data_root" "$OUT_DIR"
  stage_payload "$data_root"
  installed_size=$(du -sk "$data_root" | awk '{print $1}')
  cat > "$control_root/control" <<EOF
Package: $PACKAGE_NAME
Version: $VERSION
Section: devel
Priority: optional
Architecture: $arch
Maintainer: Vitte Team <maintainers@vitte-lang.org>
Depends: bash, python3, make
Installed-Size: $installed_size
Homepage: https://vitte-lang.org/
X-Vitte-Processor: $arch
Description: Vitte systems language toolchain
 Processor-specific Debian installer generated from the canonical Vitte payload.
EOF
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
  shasum -a 256 "$package_file" > "$package_file.sha256"
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
