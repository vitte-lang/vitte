#!/bin/sh
set -eu

normalize_arch() {
  case "$1" in
    x86_64 | X86_64 | amd64 | AMD64) printf 'amd64\n' ;;
    aarch64 | AArch64 | AARCH64 | arm64 | ARM64) printf 'arm64\n' ;;
    i386 | i486 | i586 | i686 | x86) printf 'i386\n' ;;
    armhf | armv7 | armv7l) printf 'armv7\n' ;;
    armel | armv6 | armv6l) printf 'armv6\n' ;;
    ppc64el) printf 'ppc64el\n' ;;
    powerpc64le | ppc64le) printf 'powerpc64le\n' ;;
    ppc64 | powerpc64) printf 'powerpc64\n' ;;
    ppc | powerpc) printf 'powerpc\n' ;;
    mips64 | mips64eb) printf 'mips64\n' ;;
    mips | mipseb) printf 'mips\n' ;;
    riscv64 | RISC-V64 | RISCV64) printf 'riscv64\n' ;;
    s390x | mips64el | mipsel | sparc64 | universal | universal2 | macos2006) printf '%s\n' "$1" ;;
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
    MidnightBSD | midnightbsd) printf 'midnightbsd\n' ;;
    GhostBSD | ghostbsd) printf 'ghostbsd\n' ;;
    HardenedBSD | hardenedbsd) printf 'hardenedbsd\n' ;;
    NomadBSD | nomadbsd) printf 'nomadbsd\n' ;;
    helloSystem | hellosystem) printf 'hellosystem\n' ;;
    Raspberry | raspberry) printf 'raspberry\n' ;;
    SunOS | Solaris | solaris) printf 'solaris\n' ;;
    Windows | windows | windows-xp | windows-vista | windows-7 | windows-8 | windows-8.1 | windows-10 | windows-11 | MINGW* | MSYS* | CYGWIN*) printf '%s\n' "$1" | tr '[:upper:]' '[:lower:]' ;;
    *) printf 'unsupported os: %s\n' "$1" >&2; exit 1 ;;
  esac
}

emit_rows() {
  emit_arches() {
    os=$1
    format=$2
    arches=$3
    for arch in $arches; do
      printf '%s %s %s\n' "$os" "$arch" "$format"
    done
  }

  emit_arches linux deb 'amd64 arm64 armv7 armv6 i386 riscv64 ppc64el s390x mips64el mipsel powerpc sparc64'
  emit_arches raspberry tar.gz 'armv6 armv7 arm64'

  emit_arches linux tar.gz 'amd64 i386 arm64 armv7 armv6 riscv64'
  emit_arches freebsd tar.gz 'amd64 i386 arm64 armv7 armv6 riscv64'
  emit_arches openbsd tar.gz 'amd64 i386 arm64 armv7 riscv64'
  emit_arches netbsd tar.gz 'amd64 i386 arm64 armv7 armv6 riscv64'
  emit_arches dragonfly tar.gz 'amd64'
  emit_arches macos tar.gz 'amd64 arm64'
  emit_arches solaris tar.gz 'amd64 i386'

  emit_arches freebsd pkg 'amd64 i386 arm64 armv7 armv6 riscv64 powerpc powerpc64 powerpc64le'
  emit_arches macos pkg 'arm64 amd64 universal universal2'
  emit_arches macos dmg 'arm64 amd64 universal universal2'
  emit_arches macos pkg 'macos2006'
  emit_arches macos config 'macos2006'
  emit_arches solaris pkg 'amd64 i386'

  emit_arches freebsd tar.xz 'amd64 i386 arm64 armv7 armv6 riscv64 powerpc powerpc64 powerpc64le'
  emit_arches hardenedbsd tar.xz 'amd64 i386 arm64 armv7 armv6 riscv64 powerpc powerpc64 powerpc64le'
  emit_arches openbsd tar.xz 'amd64 i386 arm64 armv7 riscv64 sparc64 powerpc64'
  emit_arches netbsd tar.xz 'amd64 i386 arm64 armv7 armv6 riscv64 sparc64 powerpc powerpc64 mips mipsel mips64 mips64el'
  emit_arches dragonfly tar.xz 'amd64'
  emit_arches midnightbsd tar.xz 'amd64 i386'
  emit_arches ghostbsd tar.xz 'amd64'
  emit_arches nomadbsd tar.xz 'amd64'
  emit_arches hellosystem tar.xz 'amd64'

  emit_arches windows-xp exe 'i386'
  emit_arches windows-vista exe 'i386'
  emit_arches windows-7 exe 'amd64'
  emit_arches windows-8 exe 'amd64'
  emit_arches windows-8.1 exe 'amd64'
  emit_arches windows-10 exe 'amd64'
  emit_arches windows-11 exe 'amd64 arm64'
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
    raw_arch=${3:?missing architecture}
    arch=$(normalize_arch "$raw_arch")
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
