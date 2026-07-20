#!/bin/sh
set -eu

normalize_arch() {
  case "$1" in
    x86_64 | amd64 | AMD64) printf 'amd64\n' ;;
    aarch64 | arm64 | ARM64) printf 'arm64\n' ;;
    i386 | i486 | i586 | i686 | x86) printf 'i386\n' ;;
    armhf | armv7 | armv7l) printf 'armv7\n' ;;
    armel | armv6 | armv6l) printf 'armv6\n' ;;
    ppc64el | powerpc64le | ppc64le) printf 'ppc64el\n' ;;
    ppc64 | powerpc64) printf 'powerpc64\n' ;;
    ppc | powerpc) printf 'powerpc\n' ;;
    riscv64 | s390x | mips64el | mipsel | sparc64 | universal | universal2 | macos2006) printf '%s\n' "$1" ;;
    *) printf 'unsupported architecture: %s\n' "$1" >&2; exit 1 ;;
  esac
}

normalize_os() {
  case "$1" in
    Linux | linux) printf 'linux\n' ;;
    Darwin | macOS | macos) printf 'macos\n' ;;
    FreeBSD | freebsd) printf 'freebsd\n' ;;
    OpenBSD | openbsd) printf 'openbsd\n' ;;
    NetBSD | netbsd) printf 'netbsd\n' ;;
    DragonFly | dragonfly) printf 'dragonfly\n' ;;
    SunOS | Solaris | solaris) printf 'solaris\n' ;;
    Windows | windows | windows-xp | windows-vista | windows-7 | windows-8 | windows-8.1 | windows-10 | windows-11 | MINGW* | MSYS* | CYGWIN*) printf '%s\n' "$1" | tr '[:upper:]' '[:lower:]' ;;
    *) printf 'unsupported os: %s\n' "$1" >&2; exit 1 ;;
  esac
}

emit_rows() {
  for arch in amd64 arm64 armv7 armv6 i386 riscv64 ppc64el s390x mips64el mipsel powerpc sparc64; do
    printf 'linux %s deb\n' "$arch"
  done
  printf '%s\n' \
    'macos arm64 dmg' \
    'macos x86_64 pkg' \
    'macos universal pkg' \
    'macos macos2006 config' \
    'windows-xp i386 exe' \
    'windows-vista i386 exe' \
    'windows-7 amd64 exe' \
    'windows-8 amd64 exe' \
    'windows-8.1 amd64 exe' \
    'windows-10 amd64 exe' \
    'windows-11 amd64 exe' \
    'windows-11 arm64 exe' \
    'freebsd amd64 pkg' \
    'freebsd i386 pkg' \
    'freebsd arm64 pkg' \
    'freebsd riscv64 pkg' \
    'freebsd powerpc64 pkg' \
    'openbsd amd64 tar.xz' \
    'openbsd i386 tar.xz' \
    'openbsd arm64 tar.xz' \
    'openbsd sparc64 tar.xz' \
    'netbsd amd64 tar.xz' \
    'netbsd i386 tar.xz' \
    'netbsd sparc64 tar.xz' \
    'netbsd mips64 tar.xz' \
    'dragonfly amd64 tar.xz' \
    'midnightbsd amd64 tar.xz' \
    'ghostbsd amd64 tar.xz' \
    'hardenedbsd amd64 tar.xz' \
    'nomadbsd amd64 tar.xz' \
    'hellosystem amd64 tar.xz' \
    'solaris amd64 pkg' \
    'solaris i386 pkg'
}

case "${1:-}" in
  normalize-arch)
    normalize_arch "${2:?missing architecture}"
    ;;
  normalize-os)
    normalize_os "${2:?missing os}"
    ;;
  lookup)
    os=$(normalize_os "${2:?missing os}")
    arch=$(normalize_arch "${3:?missing architecture}")
    format=${4:?missing package format}
    emit_rows | awk -v os="$os" -v arch="$arch" -v format="$format" '$1 == os && $2 == arch && $3 == format { print; found=1 } END { exit found ? 0 : 1 }'
    ;;
  list)
    emit_rows
    ;;
  *)
    printf 'usage: package-matrix.sh normalize-arch ARCH | normalize-os OS | lookup OS ARCH FORMAT | list\n' >&2
    exit 2
    ;;
esac
