#!/bin/sh
set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
VERSION=${VERSION:-$(tr -d ' \r\n' < "$ROOT_DIR/toolchain/scripts/package/PACKAGE_VERSION")}
OUT_DIR=${OUT_DIR:-$ROOT_DIR/pkgout}
ARCH=${ARCH:-all}
BSD_FAMILY=${BSD_FAMILY:-all}
BSD_RELEASE=${BSD_RELEASE:-all}

EDITORS_DIR=$ROOT_DIR/editors
COMPLETIONS_DIR=$ROOT_DIR/completions

die() {
  printf '[build-bsd-installers][error] %s\n' "$*" >&2
  exit 1
}

copy_tree() {
  source_dir=$1
  destination_dir=$2
  description=$3

  [ -d "$source_dir" ] ||
    die "$description directory not found: $source_dir"

  mkdir -p "$destination_dir"
  cp -R "$source_dir"/. "$destination_dir"/

  printf '[build-bsd-installers] added %s: %s\n' \
    "$description" \
    "$destination_dir"
}

add_integrations() {
  root=$1

  vitte_share=$root/usr/local/share/vitte
  editors_destination=$vitte_share/editors
  completions_destination=$vitte_share/completions

  copy_tree \
    "$EDITORS_DIR" \
    "$editors_destination" \
    "editor integrations"

  copy_tree \
    "$COMPLETIONS_DIR" \
    "$completions_destination" \
    "shell completions"
}

verify_payload() {
  root=$1
  family=$2
  release=$3
  arch=$4

  [ -d "$root/usr/local/share/vitte/editors" ] ||
    die "editor integrations missing from $family $release $arch payload"

  [ -d "$root/usr/local/share/vitte/completions" ] ||
    die "shell completions missing from $family $release $arch payload"

  find "$root/usr/local/share/vitte/editors" \
    -type f \
    -print \
    -quit |
    grep -q . ||
    die "editor integrations directory is empty for $family $release $arch"

  find "$root/usr/local/share/vitte/completions" \
    -type f \
    -print \
    -quit |
    grep -q . ||
    die "shell completions directory is empty for $family $release $arch"
}

write_install_script() {
  stage=$1

  cat > "$stage/install.sh" <<'SH'
#!/bin/sh
set -eu

[ "$(id -u)" -eq 0 ] || {
  printf 'Vitte installation requires root\n' >&2
  exit 1
}

HERE=$(CDPATH= cd -- "$(dirname "$0")" && pwd)

[ -d "$HERE/root" ] || {
  printf 'Vitte installer payload is missing: %s/root\n' "$HERE" >&2
  exit 1
}

tar -cf - -C "$HERE/root" . |
  tar -xf - -C /

printf 'Vitte installed in /usr/local.\n'
printf 'Run: vitte --help\n'
printf 'Editor integrations: /usr/local/share/vitte/editors\n'
printf 'Shell completions: /usr/local/share/vitte/completions\n'
SH

  chmod 0755 "$stage/install.sh"
}

write_install_documentation() {
  stage=$1
  family=$2
  release=$3
  arch=$4

  cat > "$stage/INSTALL.txt" <<EOF
Vitte $VERSION complete installer for $family $release $arch

INSTALLATION

Run as root:

    ./install.sh

INSTALLATION PREFIX

    /usr/local

INSTALLED COMPONENTS

    - Vitte compiler commands
    - Vitte runtime sources and libraries
    - Vitte standard library sources
    - Documentation and manual pages
    - Examples
    - Editor integrations
    - Shell completions
    - Locales
    - License files
    - assets/logo.png

EDITOR INTEGRATIONS

Installed in:

    /usr/local/share/vitte/editors

SHELL COMPLETIONS

Installed in:

    /usr/local/share/vitte/completions

After installation, run:

    vitte --help
    vitte doctor
EOF
}

build_one() {
  family=$1
  release=$2
  arch=$3

  stage=$ROOT_DIR/target/installer-$family-$release-$arch-portable
  root=$stage/root
  archive=$OUT_DIR/vitte-$VERSION-$family-$release-$arch-installer.tar.xz
  checksum=$archive.sha256

  printf '[build-bsd-installers] building %s %s %s\n' \
    "$family" \
    "$release" \
    "$arch"

  rm -rf "$stage"
  mkdir -p "$stage" "$OUT_DIR"

  VERSION=$VERSION \
    "$ROOT_DIR/scripts_build/stage-installer-payload.sh" \
    "$root" \
    "$family" \
    "$arch" \
    unix

  add_integrations "$root"
  verify_payload "$root" "$family" "$release" "$arch"

  write_install_script "$stage"
  write_install_documentation "$stage" "$family" "$release" "$arch"

  rm -f "$archive" "$checksum"

  COPYFILE_DISABLE=1 \
    tar -cJf "$archive" \
      -C "$stage" \
      install.sh \
      INSTALL.txt \
      root

  (
    cd "$OUT_DIR"
    archive_name=$(basename "$archive")
    shasum -a 256 "$archive_name" > "$archive_name.sha256"
  )

  printf '[build-bsd-installers] wrote %s\n' "$archive"
  printf '[build-bsd-installers] wrote %s\n' "$checksum"
}

normalize_arch() {
  case "$1" in
  x86_64 | amd64)
    printf 'amd64\n'
    ;;
  i386 | i486 | i586 | i686 | x86)
    printf 'i386\n'
    ;;
  aarch64 | arm64)
    printf 'arm64\n'
    ;;
  armv7 | armv7l | armhf)
    printf 'armv7\n'
    ;;
  armv6 | armel)
    printf 'armv6\n'
    ;;
  riscv64)
    printf 'riscv64\n'
    ;;
  powerpc | ppc)
    printf 'powerpc\n'
    ;;
  powerpc64 | ppc64)
    printf 'powerpc64\n'
    ;;
  powerpc64le | ppc64le)
    printf 'powerpc64le\n'
    ;;
  sparc64)
    printf 'sparc64\n'
    ;;
  mips | mipseb)
    printf 'mips\n'
    ;;
  mipsel)
    printf 'mipsel\n'
    ;;
  mips64 | mips64eb)
    printf 'mips64\n'
    ;;
  mips64el)
    printf 'mips64el\n'
    ;;
  *)
    die "unsupported BSD architecture: $ARCH"
    ;;
  esac
}

bsd_arches_for_family() {
  case "$1" in
    freebsd | hardenedbsd)
      printf '%s\n' 'amd64 i386 arm64 armv7 armv6 riscv64 powerpc powerpc64 powerpc64le'
      ;;
    openbsd)
      printf '%s\n' 'amd64 i386 arm64 armv7 riscv64 sparc64 powerpc64'
      ;;
    netbsd)
      printf '%s\n' 'amd64 i386 arm64 armv7 armv6 riscv64 sparc64 powerpc powerpc64 mips mipsel mips64 mips64el'
      ;;
    dragonfly)
      printf '%s\n' 'amd64'
      ;;
    midnightbsd)
      printf '%s\n' 'amd64 i386'
      ;;
    ghostbsd | nomadbsd | hellosystem)
      printf '%s\n' 'amd64'
      ;;
    *)
      die "unsupported BSD family: $1"
      ;;
  esac
}

bsd_releases_for_family() {
  case "$1" in
    freebsd)
      printf '%s\n' '12 13 14 15'
      ;;
    openbsd)
      printf '%s\n' '7.4 7.5 7.6 7.7 7.8'
      ;;
    netbsd)
      printf '%s\n' '9 10'
      ;;
    dragonfly)
      printf '%s\n' '6.4'
      ;;
    midnightbsd)
      printf '%s\n' '3.0'
      ;;
    ghostbsd)
      printf '%s\n' '24'
      ;;
    hardenedbsd)
      printf '%s\n' '13 14'
      ;;
    nomadbsd)
      printf '%s\n' '141'
      ;;
    hellosystem)
      printf '%s\n' '0.8'
      ;;
    *)
      die "unsupported BSD family: $1"
      ;;
  esac
}

normalize_family() {
  case "$1" in
    helloSystem | hellosystem)
      printf 'hellosystem\n'
      ;;
    freebsd | openbsd | netbsd | dragonfly | midnightbsd | ghostbsd | hardenedbsd | nomadbsd)
      printf '%s\n' "$1"
      ;;
    *)
      die "unsupported BSD family: $1"
      ;;
  esac
}

[ -f "$ROOT_DIR/toolchain/scripts/package/PACKAGE_VERSION" ] ||
  die "PACKAGE_VERSION not found"

[ -x "$ROOT_DIR/scripts_build/stage-installer-payload.sh" ] ||
  die "payload staging script is missing or not executable"

[ -d "$EDITORS_DIR" ] ||
  die "editors directory not found: $EDITORS_DIR"

[ -d "$COMPLETIONS_DIR" ] ||
  die "completions directory not found: $COMPLETIONS_DIR"

case "$BSD_FAMILY" in
  all)
    families='freebsd openbsd netbsd dragonfly midnightbsd ghostbsd hardenedbsd nomadbsd hellosystem'
    ;;
  *)
    families=$(normalize_family "$BSD_FAMILY")
    ;;
esac

case "$ARCH" in
  all)
    requested_arches=all
    ;;
  *)
    requested_arches=$(normalize_arch "$ARCH")
    ;;
esac

for family in $families; do
  if [ "$requested_arches" = all ]; then
    arches=$(bsd_arches_for_family "$family")
  else
    arches=$requested_arches
  fi

  case "$BSD_RELEASE" in
    all)
      releases=$(bsd_releases_for_family "$family")
      ;;
    *)
      releases=$BSD_RELEASE
      ;;
  esac

  for release in $releases; do
    for arch in $arches; do
      build_one "$family" "$release" "$arch"
    done
  done
done

printf '[build-bsd-installers] all BSD installers completed family=%s release=%s arch=%s\n' \
  "$BSD_FAMILY" "$BSD_RELEASE" "$ARCH"
