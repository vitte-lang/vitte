#!/usr/bin/env sh
set -eu

artifact=${1:-}
mode=${2:-executable}

die() {
  printf '[native-artifact][error] %s\n' "$1" >&2
  exit 1
}

[ -n "$artifact" ] || die "usage: tools/require_native_artifact.sh ARTIFACT [compiler]"
[ -f "$artifact" ] || die "missing artifact: $artifact"
[ -s "$artifact" ] || die "empty artifact: $artifact"
[ -x "$artifact" ] || die "artifact is not executable: $artifact"

first_line=$(LC_ALL=C sed -n '1p' "$artifact" 2>/dev/null || true)
case "$first_line" in
  '#!'*) die "script artifact is forbidden: $artifact" ;;
esac

kind=$(LC_ALL=C file -b "$artifact" 2>/dev/null || true)
host_os=$(uname -s)
host_arch=$(uname -m)
case "$kind" in
  *Mach-O*)
    [ "$host_os" = Darwin ] || die "Mach-O artifact cannot run on $host_os: $artifact"
    case "$host_arch:$kind" in
      arm64:*arm64*|x86_64:*x86_64*|x86_64:*universal*) ;;
      *) die "Mach-O processor mismatch for $host_arch: $kind" ;;
    esac
    ;;
  *ELF*)
    [ "$host_os" = Linux ] || die "ELF artifact cannot run on $host_os: $artifact"
    case "$host_arch:$kind" in
      x86_64:*x86-64*|aarch64:*ARM\ aarch64*|arm64:*ARM\ aarch64*) ;;
      *) die "ELF processor mismatch for $host_arch: $kind" ;;
    esac
    ;;
  *PE32*|*PE32+*)
    [ "${OS:-}" = Windows_NT ] || die "PE artifact cannot run on $host_os: $artifact"
    ;;
  *) die "artifact is not a recognized native executable: $kind" ;;
esac

if [ "$mode" = compiler ]; then
  for marker in \
    'bootstrap native backend could not lower this program' \
    'compiler entry still embeds bootstrap payload bridge marker' \
    'E_BOOTSTRAP_COMPILER_TEST_BRIDGE'; do
    if LC_ALL=C grep -a -F "$marker" "$artifact" >/dev/null 2>&1; then
      die "compiler artifact contains forbidden bridge marker: $marker"
    fi
  done
elif [ "$mode" != executable ]; then
  die "unsupported validation mode: $mode"
fi

printf '[native-artifact] OK mode=%s kind=%s file=%s\n' "$mode" "$kind" "$artifact"
