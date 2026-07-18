#!/bin/sh
set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
MATRIX_FILE=${VITTE_PACKAGE_MATRIX:-$ROOT_DIR/scripts_build/package-matrix.tsv}

die() {
  printf '[package-matrix][error] %s\n' "$*" >&2
  exit 1
}

normalize_os() {
  value=$(printf '%s' "$1" | tr '[:upper:]' '[:lower:]')
  case "$value" in
    linux|gnu/linux) printf 'linux\n' ;;
    darwin|macos|macosx|osx) printf 'macos\n' ;;
    windows|windows_nt|mingw*|msys*|cygwin*) printf 'windows\n' ;;
    freebsd|dragonfly) printf 'freebsd\n' ;;
    sunos|solaris) printf 'solaris\n' ;;
    *) die "unsupported operating system: $1" ;;
  esac
}

normalize_arch() {
  value=$(printf '%s' "$1" | tr '[:upper:]' '[:lower:]')
  case "$value" in
    x86_64|amd64|x64) printf 'amd64\n' ;;
    arm64|aarch64) printf 'arm64\n' ;;
    i386|i486|i586|i686|x86) printf 'i386\n' ;;
    universal|universal2) printf 'universal\n' ;;
    *) die "unsupported processor architecture: $1" ;;
  esac
}

matrix_arch() {
  os=$1
  arch=$2
  if [ "$os" = macos ] && [ "$arch" = amd64 ]; then
    printf 'x86_64\n'
  else
    printf '%s\n' "$arch"
  fi
}

lookup() {
  os=$(normalize_os "$1")
  arch=$(matrix_arch "$os" "$(normalize_arch "$2")")
  format=$3
  awk -v os="$os" -v arch="$arch" -v format="$format" '
    $1 == os && $2 == arch && $3 == format { print; found = 1 }
    END { if (!found) exit 1 }
  ' "$MATRIX_FILE" || die "unsupported package tuple: os=$os arch=$arch format=$format"
}

host() {
  os=$(normalize_os "$(uname -s)")
  arch=$(matrix_arch "$os" "$(normalize_arch "$(uname -m)")")
  printf '%s\t%s\n' "$os" "$arch"
}

list() {
  awk '!/^#/ && NF >= 4 { printf "%-10s %-10s %-5s %s\n", $1, $2, $3, $4 }' "$MATRIX_FILE"
}

doctor() {
  row=$(lookup "$1" "$2" "$3")
  tools=$(printf '%s\n' "$row" | awk '{print $4}')
  missing=0
  old_ifs=$IFS
  IFS=,
  for tool in $tools; do
    if command -v "$tool" >/dev/null 2>&1; then
      printf '[package-matrix] tool %-16s OK\n' "$tool"
    else
      printf '[package-matrix] tool %-16s MISSING\n' "$tool"
      missing=1
    fi
  done
  IFS=$old_ifs
  [ "$missing" -eq 0 ] || return 1
}

usage() {
  cat <<'EOF'
usage: scripts_build/package-matrix.sh COMMAND [ARGS]

commands:
  list
  host
  normalize-os OS
  normalize-arch ARCH
  lookup OS ARCH FORMAT
  doctor OS ARCH FORMAT
EOF
}

command=${1:-}
case "$command" in
  list) [ "$#" -eq 1 ] || die "list takes no arguments"; list ;;
  host) [ "$#" -eq 1 ] || die "host takes no arguments"; host ;;
  normalize-os) [ "$#" -eq 2 ] || die "normalize-os requires OS"; normalize_os "$2" ;;
  normalize-arch) [ "$#" -eq 2 ] || die "normalize-arch requires ARCH"; normalize_arch "$2" ;;
  lookup) [ "$#" -eq 4 ] || die "lookup requires OS ARCH FORMAT"; lookup "$2" "$3" "$4" ;;
  doctor) [ "$#" -eq 4 ] || die "doctor requires OS ARCH FORMAT"; doctor "$2" "$3" "$4" ;;
  -h|--help|help) usage ;;
  *) usage >&2; exit 2 ;;
esac
