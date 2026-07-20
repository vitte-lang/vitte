#!/bin/sh
set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
SCRIPT_NAME=build-all-installers
. "$ROOT_DIR/scripts_build/common.sh"
scripts_build_parse_common_flags "$@"
VERSION=${VERSION:-$(tr -d ' \r\n' < "$ROOT_DIR/toolchain/scripts/package/PACKAGE_VERSION")}
OUT_DIR=${OUT_DIR:-$ROOT_DIR/pkgout}
FAMILY=${FAMILY:-all}
SOURCE_DATE_EPOCH=${SOURCE_DATE_EPOCH:-$(git -C "$ROOT_DIR" log -1 --format=%ct 2>/dev/null || date +%s)}
scripts_build_maybe_help "usage: build-all-installers.sh [--dry-run]"
scripts_build_maybe_dry_run "would build installers family=$FAMILY version=$VERSION out=$OUT_DIR"

run() {
  family=$1
  description=$2
  arch=$3
  shift 3
  before=$(find "$OUT_DIR" -maxdepth 1 -type f 2>/dev/null | wc -l | tr -d ' ')
  printf '[build-all-installers] %s\n' "$description"
  VERSION=$VERSION OUT_DIR=$OUT_DIR ARCH=$arch SOURCE_DATE_EPOCH=$SOURCE_DATE_EPOCH "$@"
  after=$(find "$OUT_DIR" -maxdepth 1 -type f 2>/dev/null | wc -l | tr -d ' ')
  count=$((after - before))
  printf '[build-all-installers] summary family=%s new_artifacts=%s total_artifacts=%s\n' "$family" "$count" "$after"
}

case "$FAMILY" in
  all | linux)
    run linux 'Linux deb: amd64, arm64, armhf, armel, i386, riscv64, ppc64el, s390x, mips64el, mipsel, powerpc, sparc64' all "$ROOT_DIR/scripts_build/build-linux-debs.sh"
    ;;
esac

case "$FAMILY" in
  all | freebsd)
    run freebsd 'FreeBSD pkg: amd64, i386, arm64, armv7, armv6, riscv64, powerpc, powerpc64, powerpc64le' all "$ROOT_DIR/scripts_build/build-freebsd-packages.sh"
    ;;
esac

case "$FAMILY" in
  all | bsd)
    run bsd 'BSD portable installers: FreeBSD, OpenBSD, NetBSD, DragonFly, MidnightBSD, GhostBSD, HardenedBSD, NomadBSD, helloSystem releases and arches' all "$ROOT_DIR/scripts_build/build-bsd-installers.sh"
    ;;
esac

case "$FAMILY" in
  all | macos)
    if [ "$(uname -s)" = Darwin ]; then
      run macos 'macOS pkg+dmg: arm64, x86_64, universal, universal2, MacOS2006 config and optional legacy i386' all "$ROOT_DIR/scripts_build/build-macos-installers.sh"
    else
      printf '[build-all-installers] macOS pkg+dmg deferred: requires a Darwin host\n' >&2
    fi
    ;;
esac

case "$FAMILY" in
  all | solaris)
    run solaris 'Solaris SVR4: amd64, i386' all "$ROOT_DIR/scripts_build/build-solaris-package.sh"
    ;;
esac

case "$FAMILY" in
  all | windows)
    run windows 'Windows NSIS kits and optional EXE: XP, Vista, 7, 8, 8.1, 10, 11 for i386, amd64, arm64, armv7' all "$ROOT_DIR/scripts_build/build-windows-installer.sh"
    ;;
esac

case "$FAMILY" in
  all | linux | freebsd | bsd | macos | solaris | windows) ;;
  *) scripts_build_die "unsupported FAMILY=$FAMILY" ;;
esac

VERSION=$VERSION OUT_DIR=$OUT_DIR "$ROOT_DIR/scripts_build/verify-installers.sh"
python3 - "$OUT_DIR" "$VERSION" "$SOURCE_DATE_EPOCH" <<'PY'
import hashlib
import json
import sys
from pathlib import Path

out = Path(sys.argv[1])
artifacts = []
for path in sorted(out.iterdir() if out.exists() else []):
    if not path.is_file() or path.name == "INSTALLERS.json":
        continue
    artifacts.append({
        "name": path.name,
        "size": path.stat().st_size,
        "sha256": hashlib.sha256(path.read_bytes()).hexdigest(),
    })
report = {
    "schema": "org.vitte.installers.v1",
    "version": sys.argv[2],
    "source_date_epoch": int(sys.argv[3]),
    "artifact_count": len(artifacts),
    "artifacts": artifacts,
}
(out / "INSTALLERS.json").write_text(json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8")
PY

printf '[build-all-installers] complete version=%s out=%s\n' "$VERSION" "$OUT_DIR"
