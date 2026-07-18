#!/bin/sh
set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
VERSION=${VERSION:-$(tr -d ' \r\n' < "$ROOT_DIR/toolchain/scripts/package/PACKAGE_VERSION")}
OUT_DIR=${OUT_DIR:-$ROOT_DIR/pkgout}
ARCH=${ARCH:-all}
BSD_FAMILY=${BSD_FAMILY:-all}

die() { printf '[build-bsd-installers][error] %s\n' "$*" >&2; exit 1; }

build_one() {
  family=$1
  arch=$2
  stage=$ROOT_DIR/target/installer-$family-$arch-portable
  root=$stage/root
  archive=$OUT_DIR/vitte-$VERSION-$family-$arch-installer.tar.xz
  rm -rf "$stage"
  mkdir -p "$stage" "$OUT_DIR"
  VERSION=$VERSION "$ROOT_DIR/scripts_build/stage-installer-payload.sh" "$root" "$family" "$arch" unix
  cat > "$stage/install.sh" <<'SH'
#!/bin/sh
set -eu
[ "$(id -u)" -eq 0 ] || { echo "Vitte installation requires root" >&2; exit 1; }
HERE=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
tar -cf - -C "$HERE/root" . | tar -xf - -C /
printf 'Vitte installed in /usr/local. Run: vitte --help\n'
SH
  chmod 0755 "$stage/install.sh"
  cat > "$stage/INSTALL.txt" <<EOF
Vitte $VERSION complete installer for $family $arch

As root, run: ./install.sh
Installs compiler commands, runtime and stdlib sources, documentation, examples,
editor integrations, completions, locales, license, and assets/logo.png.
EOF
  COPYFILE_DISABLE=1 tar -cJf "$archive" -C "$stage" install.sh INSTALL.txt root
  (cd "$OUT_DIR" && shasum -a 256 "$(basename "$archive")" > "$(basename "$archive.sha256")")
  printf '[build-bsd-installers] wrote %s\n' "$archive"
}

case "$ARCH" in
  all) arches='amd64 i386' ;;
  x86_64|amd64) arches=amd64 ;;
  i386|i486|i586|i686|x86) arches=i386 ;;
  *) die "unsupported BSD architecture: $ARCH" ;;
esac
case "$BSD_FAMILY" in
  all) families='freebsd openbsd netbsd dragonfly' ;;
  freebsd|openbsd|netbsd|dragonfly) families=$BSD_FAMILY ;;
  *) die "unsupported BSD family: $BSD_FAMILY" ;;
esac

for family in $families; do
  for arch in $arches; do
    build_one "$family" "$arch"
  done
done
