#!/bin/sh
set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
SCRIPT_NAME=build-portable-tarball

. "$ROOT_DIR/scripts/common.sh"

scripts_build_parse_common_flags "$@"

VERSION=${VERSION:-$(scripts_build_package_version)}
OUT_DIR=${OUT_DIR:-$ROOT_DIR/pkgout}

case "$OUT_DIR" in
  /*) ;;
  *) OUT_DIR=$ROOT_DIR/$OUT_DIR ;;
esac

PLATFORM=${PLATFORM:-linux}
ARCH=${ARCH:-$(uname -m 2>/dev/null || printf '%s\n' unknown)}

PAYLOAD_SCRIPT=$ROOT_DIR/scripts/stage-installer-payload.sh

scripts_build_maybe_help \
  "usage: build-portable-tarball.sh [--dry-run] [--list-targets] [--print-env]"

die() {
  scripts_build_die "$*"
}

normalize_arch() {
  case "$1" in
    x86_64 | X86_64 | amd64 | AMD64)
      printf '%s\n' amd64
      ;;

    i386 | i486 | i586 | i686 | x86)
      printf '%s\n' i386
      ;;

    aarch64 | AArch64 | AARCH64 | arm64 | ARM64)
      printf '%s\n' arm64
      ;;

    armv7 | armv7l | armhf)
      printf '%s\n' armv7
      ;;

    armv6 | armv6l | armel)
      printf '%s\n' armv6
      ;;

    riscv64 | RISC-V64 | RISCV64)
      printf '%s\n' riscv64
      ;;

    *)
      die "unsupported portable architecture: $1"
      ;;
  esac
}

normalize_platform() {
  case "$1" in
    linux | Linux)
      printf '%s\n' linux
      ;;

    freebsd | FreeBSD)
      printf '%s\n' freebsd
      ;;

    openbsd | OpenBSD)
      printf '%s\n' openbsd
      ;;

    netbsd | NetBSD)
      printf '%s\n' netbsd
      ;;

    dragonfly | DragonFly | DragonFlyBSD)
      printf '%s\n' dragonfly
      ;;

    macos | darwin | Darwin)
      printf '%s\n' macos
      ;;

    solaris | SunOS)
      printf '%s\n' solaris
      ;;

    *)
      die "unsupported portable platform: $1"
      ;;
  esac
}

portable_arches_for_platform() {
  case "$1" in
    linux)
      printf '%s\n' \
        'amd64 i386 arm64 armv7 armv6 riscv64'
      ;;

    freebsd)
      printf '%s\n' \
        'amd64 i386 arm64 armv7 armv6 riscv64'
      ;;

    openbsd)
      printf '%s\n' \
        'amd64 i386 arm64 armv7 riscv64'
      ;;

    netbsd)
      printf '%s\n' \
        'amd64 i386 arm64 armv7 armv6 riscv64'
      ;;

    dragonfly)
      printf '%s\n' amd64
      ;;

    macos)
      printf '%s\n' \
        'amd64 arm64'
      ;;

    solaris)
      printf '%s\n' \
        'amd64 i386'
      ;;

    *)
      die "unsupported portable platform: $1"
      ;;
  esac
}

arch_supported() {
  platform=$1
  requested=$2

  for supported in $(portable_arches_for_platform "$platform"); do
    if [ "$supported" = "$requested" ]; then
      return 0
    fi
  done

  return 1
}

verify_directory_not_empty() {
  directory=$1
  description=$2

  [ -d "$directory" ] ||
    die "$description directory missing: $directory"

  find "$directory" \
    -type f \
    -print \
    -quit |
    grep -q . ||
    die "$description directory is empty: $directory"
}

verify_staged_payload() {
  root=$1
  platform=$2
  arch=$3

  [ -d "$root/usr/local" ] ||
    die "invalid staged payload for $platform/$arch: usr/local is missing"

  [ -d "$root/usr/local/bin" ] ||
    die "invalid staged payload for $platform/$arch: usr/local/bin is missing"

  [ -d "$root/usr/local/libexec/vitte" ] ||
    die "invalid staged payload for $platform/$arch: libexec/vitte is missing"

  [ -d "$root/usr/local/share/vitte" ] ||
    die "invalid staged payload for $platform/$arch: share/vitte is missing"

  [ -x "$root/usr/local/libexec/vitte/vitte" ] ||
    die "native Vitte compiler missing from staged payload for $platform/$arch"

  [ -x "$root/usr/local/libexec/vitte/vittec" ] ||
    die "native vittec compiler missing from staged payload for $platform/$arch"

  verify_directory_not_empty \
    "$root/usr/local/share/vitte" \
    "Vitte shared payload"
}

write_wrapper() {
  output=$1
  command=$2

  cat > "$output" <<EOF
#!/bin/sh
set -eu

self=\$0

case "\$self" in
  */*)
    ;;
  *)
    found=\$(command -v "\$self" 2>/dev/null || true)

    [ -n "\$found" ] || {
      printf '%s\n' \
        "portable Vitte wrapper must be invoked through an installed path" \
        >&2
      exit 127
    }

    self=\$found
    ;;
esac

bindir=\${self%/*}

case "\$bindir" in
  /*)
    ;;
  *)
    bindir=\$(CDPATH= cd -- "\$bindir" && pwd)
    ;;
esac

root=\${bindir%/bin}

export VITTE_ROOT=\${VITTE_ROOT:-\$root/share/vitte}

compiler="\$root/libexec/vitte/$command"

[ -x "\$compiler" ] || {
  printf 'portable Vitte executable missing: %s\n' \
    "\$compiler" \
    >&2
  exit 127
}

exec "\$compiler" "\$@"
EOF

  chmod 0755 "$output"
}

write_doctor_wrapper() {
  output=$1

  cat > "$output" <<'EOF'
#!/bin/sh
set -eu

self=$0

case "$self" in
  */*)
    ;;
  *)
    found=$(command -v "$self" 2>/dev/null || true)

    [ -n "$found" ] || {
      printf '%s\n' \
        "portable Vitte doctor must be invoked through an installed path" \
        >&2
      exit 127
    }

    self=$found
    ;;
esac

bindir=${self%/*}

case "$bindir" in
  /*)
    ;;
  *)
    bindir=$(CDPATH= cd -- "$bindir" && pwd)
    ;;
esac

root=${bindir%/bin}

printf 'Vitte portable installation\n'
printf 'root: %s\n' "$root"

status=0

check_executable() {
  path=$1

  if [ -x "$path" ]; then
    printf 'ok: %s\n' "$path"
  else
    printf 'missing: %s\n' "$path" >&2
    status=1
  fi
}

check_directory() {
  path=$1

  if [ -d "$path" ]; then
    printf 'ok: %s\n' "$path"
  else
    printf 'missing: %s\n' "$path" >&2
    status=1
  fi
}

check_executable "$root/bin/vitte"
check_executable "$root/bin/vittec"
check_executable "$root/libexec/vitte/vitte"
check_executable "$root/libexec/vitte/vittec"

check_directory "$root/share/vitte"

if [ "$status" -eq 0 ]; then
  printf '%s\n' 'Vitte portable installation is valid.'
fi

exit "$status"
EOF

  chmod 0755 "$output"
}

verify_portable_tree() {
  root=$1

  [ -x "$root/bin/vitte" ] ||
    die "portable vitte wrapper is missing"

  [ -x "$root/bin/vittec" ] ||
    die "portable vittec wrapper is missing"

  [ -x "$root/bin/vitte-installer-doctor" ] ||
    die "portable installer doctor is missing"

  [ -x "$root/libexec/vitte/vitte" ] ||
    die "portable native Vitte compiler is missing"

  [ -x "$root/libexec/vitte/vittec" ] ||
    die "portable native vittec compiler is missing"

  verify_directory_not_empty \
    "$root/share/vitte" \
    "portable Vitte share"
}

write_readme() {
  root=$1
  platform=$2
  arch=$3

  cat > "$root/README.portable" <<EOF
Vitte $VERSION portable archive
Platform: $platform
Architecture: $arch

USAGE

Run Vitte directly from the extracted directory:

    ./bin/vitte --version
    ./bin/vitte --help

Compile a Vitte source file:

    ./bin/vitte build program.vit -o program

Check a Vitte source file:

    ./bin/vitte check program.vit

VITTEC

    ./bin/vittec --help

INSTALLATION DOCTOR

    ./bin/vitte-installer-doctor

DIRECTORY LAYOUT

    bin/
        vitte
        vittec
        vitte-installer-doctor

    libexec/vitte/
        Native Vitte executables

    share/vitte/
        Runtime
        Standard library
        Sources
        Documentation
        Examples
        Editor integrations
        Shell completions
        Locales
        Assets

PORTABILITY

The wrappers automatically determine the archive root and set VITTE_ROOT
to the bundled share/vitte directory.

The archive does not need to be installed into /usr/local.

It may be extracted anywhere, provided the internal directory structure
is preserved.
EOF
}

build_one() {
  platform=$1
  arch=$2

  package_dir=vitte-$VERSION-portable-$platform-$arch
  build_dir=$ROOT_DIR/target/portable-$platform-$arch

  stage_root=$build_dir/stage/root
  portable_root=$build_dir/$package_dir

  archive=$OUT_DIR/$package_dir.tar.gz
  checksum=$archive.sha256
  manifest=$archive.MANIFEST.json

  printf '[build-portable-tarball] building %s/%s\n' \
    "$platform" \
    "$arch"

  rm -rf "$build_dir"

  rm -f \
    "$archive" \
    "$checksum" \
    "$manifest"

  mkdir -p \
    "$stage_root" \
    "$portable_root/bin" \
    "$OUT_DIR"

  VERSION=$VERSION \
    "$PAYLOAD_SCRIPT" \
    "$stage_root" \
    "$platform" \
    "$arch" \
    unix

  verify_staged_payload \
    "$stage_root" \
    "$platform" \
    "$arch"

  scripts_build_copy_tree \
    "$stage_root/usr/local/libexec" \
    "$portable_root/libexec"

  scripts_build_copy_tree \
    "$stage_root/usr/local/share" \
    "$portable_root/share"

  write_wrapper \
    "$portable_root/bin/vitte" \
    vitte

  write_wrapper \
    "$portable_root/bin/vittec" \
    vittec

  write_doctor_wrapper \
    "$portable_root/bin/vitte-installer-doctor"

  write_readme \
    "$portable_root" \
    "$platform" \
    "$arch"

  verify_portable_tree \
    "$portable_root"

  scripts_build_tar_gz \
    "$archive" \
    "$build_dir" \
    "$package_dir"

  scripts_build_sha256_write \
    "$archive" \
    "$checksum"

  scripts_build_sha256_check \
    "$archive" \
    "$checksum"

  scripts_build_write_artifact_manifest \
    "$archive" \
    "$platform" \
    "$arch" \
    "$VERSION"

  archive_size=$(wc -c < "$archive" | tr -d ' ')

  [ "$archive_size" -gt 1024 ] ||
    die "portable archive is unexpectedly small: $archive"

  printf '[build-portable-tarball] wrote %s (%s bytes)\n' \
    "$archive" \
    "$archive_size"

  printf '[build-portable-tarball] wrote %s\n' \
    "$checksum"

  printf '[build-portable-tarball] wrote %s\n' \
    "$manifest"
}

scripts_build_require find
scripts_build_require grep
scripts_build_require tar
scripts_build_require wc

[ -x "$PAYLOAD_SCRIPT" ] ||
  die "payload staging script is missing or not executable: $PAYLOAD_SCRIPT"

PLATFORM=$(normalize_platform "$PLATFORM")

if [ "$LIST_TARGETS" -eq 1 ]; then
  for platform in \
    linux \
    freebsd \
    openbsd \
    netbsd \
    dragonfly \
    macos \
    solaris
  do
    for arch in $(portable_arches_for_platform "$platform"); do
      printf '%s %s\n' \
        "$platform" \
        "$arch"
    done
  done

  exit 0
fi

if [ "$PRINT_ENV" -eq 1 ]; then
  printf 'VERSION=%s\n' "$VERSION"
  printf 'OUT_DIR=%s\n' "$OUT_DIR"
  printf 'PLATFORM=%s\n' "$PLATFORM"
  printf 'ARCH=%s\n' "$ARCH"
  exit 0
fi

case "$ARCH" in
  all)
    for portable_arch in $(portable_arches_for_platform "$PLATFORM"); do
      build_one \
        "$PLATFORM" \
        "$portable_arch"
    done
    ;;

  *)
    normalized_arch=$(normalize_arch "$ARCH")

    arch_supported "$PLATFORM" "$normalized_arch" ||
      die "architecture $normalized_arch is not supported for portable platform $PLATFORM"

    build_one \
      "$PLATFORM" \
      "$normalized_arch"
    ;;
esac

printf \
  '[build-portable-tarball] complete version=%s platform=%s arch=%s out=%s\n' \
  "$VERSION" \
  "$PLATFORM" \
  "$ARCH" \
  "$OUT_DIR"