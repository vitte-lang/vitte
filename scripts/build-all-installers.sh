#!/bin/sh
set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
SCRIPT_NAME=build-all-installers

. "$ROOT_DIR/scripts/common.sh"

scripts_build_parse_common_flags "$@"

VERSION=${VERSION:-$(scripts_build_package_version)}
OUT_DIR=${OUT_DIR:-$ROOT_DIR/pkgout}

case "$OUT_DIR" in
  /*) ;;
  *) OUT_DIR=$ROOT_DIR/$OUT_DIR ;;
esac

FAMILY=${FAMILY:-all}
SOURCE_DATE_EPOCH=${SOURCE_DATE_EPOCH:-$(git -C "$ROOT_DIR" log -1 --format=%ct 2>/dev/null || date +%s)}
SBOM=${SBOM:-0}
SIGN=${SIGN:-0}
NOTARIZE=${NOTARIZE:-0}

case "$SBOM" in
  0 | 1) ;;
  *) scripts_build_die "SBOM must be 0 or 1" ;;
esac

case "$SIGN" in
  0 | 1) ;;
  *) scripts_build_die "SIGN must be 0 or 1" ;;
esac

case "$NOTARIZE" in
  0 | 1) ;;
  *) scripts_build_die "NOTARIZE must be 0 or 1" ;;
esac

if [ "$NOTARIZE" -eq 1 ] && [ "$SIGN" -ne 1 ]; then
  scripts_build_die "NOTARIZE=1 requires SIGN=1"
fi

scripts_build_maybe_help \
  "usage: build-all-installers.sh [--dry-run] [--list-targets] [--print-env] [--verify-only OUT_DIR] [--clean]"

if [ "$LIST_TARGETS" -eq 1 ]; then
  "$ROOT_DIR/scripts/package-matrix.sh" list
  exit 0
fi

if [ "$PRINT_ENV" -eq 1 ]; then
  printf 'VERSION=%s\n' "$VERSION"
  printf 'OUT_DIR=%s\n' "$OUT_DIR"
  printf 'FAMILY=%s\n' "$FAMILY"
  printf 'SOURCE_DATE_EPOCH=%s\n' "$SOURCE_DATE_EPOCH"
  printf 'SBOM=%s\n' "$SBOM"
  printf 'SIGN=%s\n' "$SIGN"
  printf 'NOTARIZE=%s\n' "$NOTARIZE"
  exit 0
fi

if [ -n "$VERIFY_ONLY" ]; then
  verify_dir=$VERIFY_ONLY

  case "$verify_dir" in
    /*) ;;
    *) verify_dir=$ROOT_DIR/$verify_dir ;;
  esac

  VERSION=$VERSION \
  OUT_DIR=$verify_dir \
  VERIFY_METADATA=1 \
    "$ROOT_DIR/scripts/verify-installers.sh"

  exit 0
fi

if [ "$CLEAN" -eq 1 ]; then
  case "${CONFIRM_CLEAN:-}" in
    YES) ;;
    *)
      scripts_build_die \
        "--clean requires CONFIRM_CLEAN=YES"
      ;;
  esac

  if [ -d "$ROOT_DIR/target" ]; then
    for installer_stage in "$ROOT_DIR/target"/installer-*; do
      [ -d "$installer_stage" ] || continue
      rm -rf "$installer_stage"
    done
  fi

  printf '%s\n' \
    '[build-all-installers] cleaned target/installer-*'

  exit 0
fi

scripts_build_maybe_dry_run \
  "would build installers family=$FAMILY version=$VERSION out=$OUT_DIR"

mkdir -p "$OUT_DIR"
mkdir -p "$OUT_DIR/logs"

artifact_count() {
  if [ ! -d "$OUT_DIR" ]; then
    printf '%s\n' 0
    return 0
  fi

  count=0
  for artifact in "$OUT_DIR"/*; do
    [ -f "$artifact" ] || continue
    case "$(basename "$artifact")" in
      INSTALLERS.json|CHECKSUMS.txt|SIGNATURES.json|SBOM.spdx.json|SBOM.cyclonedx.json)
        continue
        ;;
    esac
    count=$((count + 1))
  done
  printf '%s\n' "$count"
}

run() {
  family=$1
  description=$2
  arch=$3
  shift 3

  before=$(artifact_count)

  printf '[build-all-installers] %s\n' \
    "$description"

  VERSION=$VERSION \
  OUT_DIR=$OUT_DIR \
  ARCH=$arch \
  SOURCE_DATE_EPOCH=$SOURCE_DATE_EPOCH \
  SBOM=$SBOM \
  SIGN=$SIGN \
  NOTARIZE=$NOTARIZE \
    "$@"

  after=$(artifact_count)

  if [ "$after" -ge "$before" ]; then
    count=$((after - before))
  else
    count=0
  fi

  printf \
    '[build-all-installers] summary family=%s new_artifacts=%s total_artifacts=%s\n' \
    "$family" \
    "$count" \
    "$after"

  printf \
    '{"schema":"org.vitte.installer-builder-log.v1","family":"%s","new_artifacts":%s,"total_artifacts":%s}\n' \
    "$family" \
    "$count" \
    "$after" \
    > "$OUT_DIR/logs/$family.json"
}

case "$FAMILY" in
  all | linux)
    run \
      linux \
      'Linux deb: amd64, arm64, armhf, armel, i386, riscv64, ppc64el, s390x, mips64el, mipsel, powerpc, sparc64' \
      all \
      "$ROOT_DIR/scripts/build-linux-debs.sh"
    ;;
esac

case "$FAMILY" in
  all | portable)
    run \
      portable \
      'Portable tar.gz archives: amd64, i386, arm64, armv7, armv6 (Raspberry Pi 1), riscv64' \
      all \
      "$ROOT_DIR/scripts/build-portable-tarball.sh"
    ;;
esac

case "$FAMILY" in
  all | freebsd)
    run \
      freebsd \
      'FreeBSD pkg: amd64, i386, arm64, armv7, armv6, riscv64, powerpc, powerpc64, powerpc64le' \
      all \
      "$ROOT_DIR/scripts/build-freebsd-packages.sh"
    ;;
esac

case "$FAMILY" in
  all | bsd)
    run \
      bsd \
      'BSD portable installers: FreeBSD, OpenBSD, NetBSD, DragonFly, MidnightBSD, GhostBSD, HardenedBSD, NomadBSD, helloSystem releases and arches' \
      all \
      "$ROOT_DIR/scripts/build-bsd-installers.sh"
    ;;
esac

case "$FAMILY" in
  all | macos)
    if [ "$(uname -s)" = Darwin ]; then
      run \
        macos \
        'macOS pkg+dmg: arm64, x86_64, universal, universal2, MacOS2006 config and optional legacy i386' \
        all \
        "$ROOT_DIR/scripts/build-macos-installers.sh"
    else
      printf '%s\n' \
        '[build-all-installers] macOS pkg+dmg deferred: requires a Darwin host' \
        >&2
    fi
    ;;
esac

case "$FAMILY" in
  all | solaris)
    run \
      solaris \
      'Solaris SVR4: amd64, i386' \
      all \
      "$ROOT_DIR/scripts/build-solaris-package.sh"
    ;;
esac

case "$FAMILY" in
  all | windows)
    run \
      windows \
      'Windows NSIS kits and optional EXE: XP, Vista, 7, 8, 8.1, 10, 11 for i386, amd64, arm64, armv7' \
      all \
      "$ROOT_DIR/scripts/build-windows-installer.sh"
    ;;
esac

case "$FAMILY" in
  all | linux | portable | freebsd | bsd | macos | solaris | windows | metadata)
    ;;
  *)
    scripts_build_die \
      "unsupported FAMILY=$FAMILY"
    ;;
esac

#
# Metadata generation
#
# release_installer_metadata.py is the canonical metadata generator.
# Do not create a second parallel INSTALLERS.json / CHECKSUMS.txt /
# SIGNATURES.json implementation here.
#

scripts_build_require python3

set -- \
  --out "$OUT_DIR" \
  --version "$VERSION" \
  --source-date-epoch "$SOURCE_DATE_EPOCH"

if [ "$SBOM" -eq 1 ]; then
  set -- "$@" --sbom
fi

if [ "$SIGN" -eq 1 ]; then
  set -- "$@" --require-signatures
fi

if [ -n "${VITTE_RELEASE_PUBLIC_KEY:-}" ]; then
  set -- \
    "$@" \
    --public-key "$VITTE_RELEASE_PUBLIC_KEY"
fi

python3 \
  "$ROOT_DIR/tools/release_installer_metadata.py" \
  "$@"

#
# Final verification
#

VERSION=$VERSION \
OUT_DIR=$OUT_DIR \
VERIFY_METADATA=1 \
  "$ROOT_DIR/scripts/verify-installers.sh"

printf \
  '[build-all-installers] complete version=%s out=%s\n' \
  "$VERSION" \
  "$OUT_DIR"
