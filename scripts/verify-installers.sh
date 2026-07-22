#!/bin/sh
set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
SCRIPT_NAME=verify-installers
. "$ROOT_DIR/scripts_build/common.sh"
VERSION=${VERSION:-$(tr -d ' \r\n' < "$ROOT_DIR/toolchain/scripts/package/PACKAGE_VERSION")}
OUT_DIR=${OUT_DIR:-$ROOT_DIR/pkgout}
case "$OUT_DIR" in /*) ;; *) OUT_DIR=$ROOT_DIR/$OUT_DIR ;; esac
STRICT_NATIVE=${STRICT_NATIVE:-0}
FREEBSD_MAJOR=${FREEBSD_MAJOR:-14}
VERIFY_METADATA=${VERIFY_METADATA:-0}
RELEASE_INSTALLER_GATE=${RELEASE_INSTALLER_GATE:-0}

die() {
  printf '[verify-installers][error] %s\n' "$*" >&2
  exit 1
}

require_file() {
  [ -s "$1" ] || die "missing artifact: $1"
}

verify_sum() {
  file=$1
  require_file "$file"
  require_file "$file.sha256"
  scripts_build_sha256_check "$file" "$file.sha256"
}

verify_optional_sum() {
  file=$1
  [ ! -e "$file" ] || verify_sum "$file"
}

verify_deb() {
  arch=$1
  file=$OUT_DIR/vitte_${VERSION}_${arch}.deb
  [ -e "$file" ] || return 0
  verify_sum "$file"
  tmp=$(mktemp -d)
  trap 'rm -rf "$tmp"' EXIT HUP INT TERM
  (cd "$tmp" && ar -x "$file")
  for member in debian-binary control.tar.gz data.tar.gz; do
    [ -s "$tmp/$member" ] || die "Debian package missing $member: $file"
  done
  bsdtar -tf "$tmp/control.tar.gz" | grep -Eq '^(\./)?control$' || die "Debian package missing control: $file"
  bsdtar -tf "$tmp/control.tar.gz" | grep -Eq '^(\./)?md5sums$' || die "Debian package missing md5sums: $file"
  bsdtar -tf "$tmp/control.tar.gz" | grep -Eq '^(\./)?postinst$' || die "Debian package missing postinst: $file"
  bsdtar -tf "$tmp/control.tar.gz" | grep -Eq '^(\./)?prerm$' || die "Debian package missing prerm: $file"
  bsdtar -tf "$tmp/data.tar.gz" | grep -Eq '^(\./)?usr/local/share/vitte/INSTALLATION.json$' || die "Debian package missing INSTALLATION.json: $file"
  for command in vitte vittec vittec0; do
    bsdtar -xOf "$tmp/data.tar.gz" "./usr/local/bin/$command" 2>/dev/null | grep -F "/usr/local/libexec/vitte/$command" >/dev/null ||
      die "Debian wrapper does not exec libexec $command: $file"
  done
  bsdtar -tf "$tmp/data.tar.gz" | grep -Eq '^(\./)?usr/local/bin/vitte-installer-doctor$' ||
    die "Debian package missing installer doctor: $file"
  rm -rf "$tmp"
  trap - EXIT HUP INT TERM
}

verify_freebsd() {
  arch=$1
  abi_arch=$2
  file=$OUT_DIR/vitte-${VERSION}-freebsd-${arch}.pkg
  [ -e "$file" ] || return 0
  verify_sum "$file"
  bsdtar -tf "$file" | grep -Fx '+MANIFEST' >/dev/null || die "FreeBSD package missing +MANIFEST: $file"
  bsdtar -tf "$file" | grep -Fx '+COMPACT_MANIFEST' >/dev/null || die "FreeBSD package missing +COMPACT_MANIFEST: $file"
  bsdtar -tf "$file" | grep -Fx '+POST_INSTALL' >/dev/null || die "FreeBSD package missing +POST_INSTALL: $file"
  bsdtar -tf "$file" | grep -Fx '+PRE_DEINSTALL' >/dev/null || die "FreeBSD package missing +PRE_DEINSTALL: $file"
  bsdtar -tf "$file" | grep -Fx '+POST_DEINSTALL' >/dev/null || die "FreeBSD package missing +POST_DEINSTALL: $file"
  bsdtar -tf "$file" | grep -Eq '^(\./)?usr/local/share/vitte/INSTALLATION.json$' || die "FreeBSD package missing INSTALLATION.json: $file"
  bsdtar -xOf "$file" +COMPACT_MANIFEST | python3 -c \
    'import json,sys; value=json.load(sys.stdin); expected=sys.argv[1]; assert value["abi"] == expected' \
    "FreeBSD:$FREEBSD_MAJOR:$abi_arch" || die "wrong FreeBSD ABI: $file"
}

for arch in amd64 arm64 armhf armel i386 riscv64 ppc64el s390x mips64el mipsel powerpc sparc64; do
  verify_deb "$arch"
done

for item in \
  'amd64 amd64' \
  'i386 i386' \
  'arm64 aarch64' \
  'armv7 armv7' \
  'armv6 armv6' \
  'riscv64 riscv64' \
  'powerpc powerpc' \
  'powerpc64 powerpc64' \
  'powerpc64le powerpc64le'
do
  set -- $item
  verify_freebsd "$1" "$2"
done

for bsd_kit in "$OUT_DIR"/vitte-"$VERSION"-*-*-installer.tar.xz; do
  [ -e "$bsd_kit" ] || continue
  verify_sum "$bsd_kit"
  scripts_build_tar_list_xz "$bsd_kit" | grep -Fx 'root/usr/local/share/vitte/assets/logo.png' >/dev/null ||
    die "missing BSD installer logo: $bsd_kit"
  scripts_build_tar_list_xz "$bsd_kit" | grep -Fx 'root/usr/local/share/vitte/INSTALLATION.json' >/dev/null ||
    die "missing BSD installer INSTALLATION.json: $bsd_kit"
  scripts_build_tar_list_xz "$bsd_kit" | grep -Fx 'uninstall.sh' >/dev/null ||
    die "missing BSD uninstall script: $bsd_kit"
  for component in src/vitte/stdlib docs locales completions editors; do
    scripts_build_tar_list_xz "$bsd_kit" | grep -q "^root/usr/local/share/vitte/$component/" ||
      die "BSD installer missing payload component $component: $bsd_kit"
  done
  for command in vitte vittec vittec0; do
    tar -xOf "$bsd_kit" "root/usr/local/bin/$command" 2>/dev/null | grep -F "/usr/local/libexec/vitte/$command" >/dev/null ||
      die "BSD wrapper does not exec libexec $command: $bsd_kit"
  done
  scripts_build_tar_list_xz "$bsd_kit" | grep -Fx 'root/usr/local/bin/vitte-installer-doctor' >/dev/null ||
    die "BSD installer missing installer doctor: $bsd_kit"
done

for portable_kit in "$OUT_DIR"/vitte-"$VERSION"-portable-*-*.tar.gz; do
  [ -e "$portable_kit" ] || continue
  verify_sum "$portable_kit"
  package_dir=$(basename "$portable_kit" .tar.gz)
  for required in \
    "$package_dir/bin/vitte" \
    "$package_dir/bin/vittec" \
    "$package_dir/bin/vittec0" \
    "$package_dir/bin/vitte-installer-doctor" \
    "$package_dir/libexec/vitte/vitte" \
    "$package_dir/share/vitte/INSTALLATION.json" \
    "$package_dir/share/vitte/VERSION" \
    "$package_dir/README.portable"
  do
    tar -tzf "$portable_kit" | grep -Fx "$required" >/dev/null ||
      die "portable archive missing $required: $portable_kit"
  done
  tar -xOzf "$portable_kit" "$package_dir/bin/vitte" | grep -F 'VITTE_ROOT=${VITTE_ROOT:-$root/share/vitte}' >/dev/null ||
    die "portable wrapper does not set relative VITTE_ROOT: $portable_kit"
  tar -xOzf "$portable_kit" "$package_dir/bin/vitte" | grep -F 'exec "$root/libexec/vitte/vitte" "$@"' >/dev/null ||
    die "portable wrapper does not exec relative libexec vitte: $portable_kit"
done

if [ "$(uname -s)" = Darwin ]; then
  for arch in arm64 x86_64 universal universal2 macos2006-i386; do
    verify_optional_sum "$OUT_DIR/vitte-${VERSION}-macos-${arch}.pkg"
    verify_optional_sum "$OUT_DIR/vitte-${VERSION}-macos-${arch}.dmg"
  done
fi

for arch in amd64 i386; do
  solaris_kit=$OUT_DIR/vitte-${VERSION}-solaris-${arch}-spool.tar.gz
  [ ! -e "$solaris_kit" ] || {
    verify_sum "$solaris_kit"
    tar -xOzf "$solaris_kit" pkginfo | grep -Fx "VITTE_PROCESSOR=$arch" >/dev/null ||
      die "invalid Solaris processor"
    for required in pkginfo prototype depend postinstall preremove root/usr/local/share/vitte/INSTALLATION.json; do
      tar -tzf "$solaris_kit" | grep -Fx "$required" >/dev/null ||
        die "Solaris kit missing $required: $solaris_kit"
    done
    for required in install.sh uninstall.sh; do
      tar -tzf "$solaris_kit" | grep -Fx "$required" >/dev/null ||
        die "Solaris kit missing $required: $solaris_kit"
    done
    tar -xOzf "$solaris_kit" prototype | grep -F 'usr/local/bin/vitte' >/dev/null ||
      die "Solaris prototype missing vitte command"
  }
done

if [ -s "$OUT_DIR/CHECKSUMS.txt" ]; then
  while IFS= read -r line; do
    set -- $line
    [ -s "$OUT_DIR/$2" ] || die "CHECKSUMS.txt references missing artifact: $2"
  done < "$OUT_DIR/CHECKSUMS.txt"
fi

if [ "$VERIFY_METADATA" -eq 1 ] || [ "$RELEASE_INSTALLER_GATE" -eq 1 ]; then
  require_file "$OUT_DIR/INSTALLERS.json"
  require_file "$OUT_DIR/CHECKSUMS.txt"
  require_file "$OUT_DIR/SIGNATURES.json"
  python3 - "$OUT_DIR" "$RELEASE_INSTALLER_GATE" <<'PY'
import json
import sys
from pathlib import Path

out = Path(sys.argv[1])
release = sys.argv[2] == "1"
installers = json.loads((out / "INSTALLERS.json").read_text(encoding="utf-8"))
signatures = json.loads((out / "SIGNATURES.json").read_text(encoding="utf-8"))
signature_names = {item["name"] for item in signatures.get("artifacts", [])}
metadata = {"INSTALLERS.json", "CHECKSUMS.txt", "SIGNATURES.json", "SBOM.spdx.json", "SBOM.cyclonedx.json"}
for artifact in installers.get("artifacts", []):
    name = artifact["name"]
    if name in metadata or name.endswith(".sha256") or name.endswith(".MANIFEST.json"):
        continue
    manifest = out / f"{name}.MANIFEST.json"
    if not manifest.is_file():
        raise SystemExit(f"missing artifact manifest: {manifest.name}")
    value = json.loads(manifest.read_text(encoding="utf-8"))
    for key in ("version", "os", "arch", "abi", "libc", "minimum_version", "sha256", "installed_commands", "contents"):
        if key not in value:
            raise SystemExit(f"manifest {manifest.name} missing {key}")
    if name not in signature_names:
        raise SystemExit(f"SIGNATURES.json missing {name}")
if release:
    for sbom in ("SBOM.spdx.json", "SBOM.cyclonedx.json"):
        if not (out / sbom).is_file():
            raise SystemExit(f"release metadata missing {sbom}")
    if not signatures.get("sign_requested"):
        raise SystemExit("release metadata requires sign_requested=true")
PY
fi

  for item in 'amd64 0x8664' 'i386 0x014c' 'arm64 0xaa64' 'armv7 0x01c4'; do
  set -- $item
  windows_kit=$OUT_DIR/vitte-${VERSION}-windows-$1-nsis.tar.gz
  [ ! -e "$windows_kit" ] || {
    verify_sum "$windows_kit"
    tar -xOzf "$windows_kit" BUILD.txt | grep -Fx "Processor: $1 (PE machine $2)" >/dev/null ||
      die "invalid Windows processor manifest"
    tar -tzf "$windows_kit" | grep -Fx 'payload/share/vitte/assets/logo.png' >/dev/null ||
      die "missing Windows installer logo"
    for required in installer.nsi BUILD.txt payload/share/vitte/INSTALLATION.json payload/share/vitte/VERSION; do
      tar -tzf "$windows_kit" | grep -Fx "$required" >/dev/null ||
        die "Windows kit missing $required: $windows_kit"
    done
    tar -xOzf "$windows_kit" installer.nsi | grep -F '!include "LogicLib.nsh"' >/dev/null ||
      die "Windows NSIS script missing LogicLib include: $windows_kit"
    tar -xOzf "$windows_kit" installer.nsi | grep -F '!include "StrFunc.nsh"' >/dev/null ||
      die "Windows NSIS script missing StrFunc include: $windows_kit"
    tar -xOzf "$windows_kit" installer.nsi | grep -F 'Windows XP through Windows 11' >/dev/null ||
      die "Windows NSIS script missing XP through Windows 11 support label: $windows_kit"
    tar -xOzf "$windows_kit" installer.nsi | grep -F 'Function un.RemoveFromPath' >/dev/null ||
      die "Windows NSIS script missing PATH uninstall function: $windows_kit"
    tar -xOzf "$windows_kit" installer.nsi | grep -F 'DeleteRegValue HKLM "SYSTEM\CurrentControlSet\Control\Session Manager\Environment" "VITTE_ROOT"' >/dev/null ||
      die "Windows NSIS script does not remove VITTE_ROOT: $windows_kit"
    for command in vitte vittec vittec0; do
      tar -tzf "$windows_kit" | grep -Fx "payload/bin/$command.cmd" >/dev/null ||
        die "Windows kit missing cmd shim for $command: $windows_kit"
      tar -tzf "$windows_kit" | grep -Fx "payload/bin/$command.ps1" >/dev/null ||
        die "Windows kit missing PowerShell shim for $command: $windows_kit"
      tar -xOzf "$windows_kit" "payload/bin/$command.cmd" | grep -F 'set "VITTE_ROOT=%~dp0..\share\vitte"' >/dev/null ||
        die "Windows cmd shim missing VITTE_ROOT for $command: $windows_kit"
      tar -xOzf "$windows_kit" "payload/bin/$command.ps1" | grep -F '$env:VITTE_ROOT = Join-Path $PSScriptRoot "..\share\vitte"' >/dev/null ||
        die "Windows PowerShell shim missing VITTE_ROOT for $command: $windows_kit"
    done
    tar -tzf "$windows_kit" | grep -Fx 'payload/bin/vitte-installer-doctor.cmd' >/dev/null ||
      die "Windows kit missing cmd installer doctor: $windows_kit"
    tar -xOzf "$windows_kit" payload/bin/vitte-installer-doctor.cmd | grep -F 'Vitte installer doctor' >/dev/null ||
      die "Windows cmd installer doctor missing status output: $windows_kit"
  }
done

if [ "$STRICT_NATIVE" -eq 1 ]; then
  verify_sum "$OUT_DIR/vitte-${VERSION}-solaris-amd64.pkg"
  verify_sum "$OUT_DIR/vitte-${VERSION}-solaris-i386.pkg"
  verify_sum "$OUT_DIR/vitte-${VERSION}-windows-amd64-installer.exe"
  verify_sum "$OUT_DIR/vitte-${VERSION}-windows-i386-installer.exe"
else
  for arch in amd64 i386; do
    verify_optional_sum "$OUT_DIR/vitte-${VERSION}-solaris-${arch}.pkg"
  done
  for arch in amd64 i386 arm64 armv7; do
    verify_optional_sum "$OUT_DIR/vitte-${VERSION}-windows-${arch}-installer.exe"
  done
fi

printf '[verify-installers] OK version=%s strict_native=%s out=%s\n' "$VERSION" "$STRICT_NATIVE" "$OUT_DIR"
