#!/bin/sh
set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
SCRIPT_NAME=build-windows-installer
. "$ROOT_DIR/scripts_build/common.sh"
scripts_build_parse_common_flags "$@"
VERSION=${VERSION:-$(tr -d ' \r\n' < "$ROOT_DIR/toolchain/scripts/package/PACKAGE_VERSION")}
OUT_DIR=${OUT_DIR:-$ROOT_DIR/pkgout}
ARCH=${ARCH:-all}
PACKAGE_NAME=${PACKAGE_NAME:-vitte}
WINDOWS_VITTE_EXE=${WINDOWS_VITTE_EXE:-}
WINDOWS_TARGETS=${WINDOWS_TARGETS:-xp vista 7 8 8.1 10 11}
scripts_build_maybe_help "usage: build-windows-installer.sh [--dry-run]"
scripts_build_maybe_dry_run "would build Windows NSIS kits version=$VERSION arch=$ARCH out=$OUT_DIR"

die() {
  printf '[build-windows-installer][error] %s\n' "$*" >&2
  exit 1
}

require() {
  command -v "$1" >/dev/null 2>&1 || die "missing required tool: $1"
}

machine_for_arch() {
  case "$1" in
    amd64) printf '0x8664' ;;
    i386) printf '0x014c' ;;
    arm64) printf '0xaa64' ;;
    armv7) printf '0x01c4' ;;
    *) die "unsupported Windows architecture: $1" ;;
  esac
}

validate_pe() {
  python3 - "$1" "$2" <<'PY'
import struct
import sys
from pathlib import Path

expected = {
    "amd64": 0x8664,
    "i386": 0x014C,
    "arm64": 0xAA64,
    "armv7": 0x01C4,
}[sys.argv[2]]
path = Path(sys.argv[1])
data = path.read_bytes()
if len(data) < 64 or data[:2] != b"MZ":
    raise SystemExit(f"not a PE executable: {path}")
offset = struct.unpack_from("<I", data, 0x3C)[0]
if offset + 6 > len(data) or data[offset:offset + 4] != b"PE\0\0":
    raise SystemExit(f"invalid PE signature: {path}")
machine = struct.unpack_from("<H", data, offset + 4)[0]
if machine != expected:
    raise SystemExit(f"wrong PE processor 0x{machine:04x}, expected 0x{expected:04x}: {path}")
PY
}

find_windows_exe() {
  arch=$1
  [ -z "$WINDOWS_VITTE_EXE" ] || {
    printf '%s\n' "$WINDOWS_VITTE_EXE"
    return 0
  }
  for candidate in \
    "$ROOT_DIR/target/windows-$arch/vitte.exe" \
    "$ROOT_DIR/target/windows/$arch/vitte.exe" \
    "$ROOT_DIR/bin/windows-$arch/vitte.exe" \
    "$ROOT_DIR/bin/$arch/vitte.exe"
  do
    if [ -f "$candidate" ]; then
      printf '%s\n' "$candidate"
      return 0
    fi
  done
  return 1
}

write_cmd_shims() {
  payload=$1
  for command in vitte vittec vittec0; do
    cat > "$payload/bin/$command.cmd" <<EOF
@echo off
set "VITTE_ROOT=%~dp0..\share\vitte"
"%~dp0vitte.exe" %*
EOF
  done
}

write_nsi() {
  stage=$1
  arch=$2
  cat > "$stage/installer.nsi" <<'NSI'
Unicode true
!include "MUI2.nsh"
!include "x64.nsh"
!include "WinVer.nsh"

!ifndef VERSION
  !define VERSION "0.0.0"
!endif
!ifndef OUT_FILE
  !define OUT_FILE "vitte-windows-installer.exe"
!endif
!ifndef ARCH
  !define ARCH "amd64"
!endif
!ifndef WINDOWS_TARGETS
  !define WINDOWS_TARGETS "xp vista 7 8 8.1 10 11"
!endif

Name "Vitte ${VERSION} (${ARCH})"
OutFile "${OUT_FILE}"
InstallDir "$PROGRAMFILES\Vitte"
!if "${ARCH}" == "amd64"
  InstallDir "$PROGRAMFILES64\Vitte"
!endif
InstallDirRegKey HKLM "Software\Vitte" "InstallDir"
RequestExecutionLevel admin
SetCompressor /SOLID lzma

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "LICENSE"
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_LANGUAGE "English"
!insertmacro MUI_LANGUAGE "French"

Function AddToPath
  Push $0
  Push $1
  ReadRegStr $0 HKLM "SYSTEM\CurrentControlSet\Control\Session Manager\Environment" "Path"
  StrCpy $1 "$INSTDIR\bin"
  ${If} $0 == ""
    WriteRegExpandStr HKLM "SYSTEM\CurrentControlSet\Control\Session Manager\Environment" "Path" "$1"
  ${Else}
    StrCmp $0 "$1" done
    WriteRegExpandStr HKLM "SYSTEM\CurrentControlSet\Control\Session Manager\Environment" "Path" "$0;$1"
  ${EndIf}
done:
  WriteRegStr HKLM "SYSTEM\CurrentControlSet\Control\Session Manager\Environment" "VITTE_ROOT" "$INSTDIR\share\vitte"
  Pop $1
  Pop $0
FunctionEnd

Section "Vitte" SEC_VITTE
  SectionIn RO
  ${IfNot} ${AtLeastWinXP}
    MessageBox MB_ICONSTOP "Vitte requires Windows XP or later."
    Abort
  ${EndIf}
  SetOutPath "$INSTDIR"
  File /r "payload\*"
  Call AddToPath
  CreateDirectory "$SMPROGRAMS\Vitte"
  CreateShortCut "$SMPROGRAMS\Vitte\Vitte shell.lnk" "$SYSDIR\cmd.exe" '/K ""$INSTDIR\bin\vitte.cmd" --help"'
  CreateShortCut "$SMPROGRAMS\Vitte\Uninstall Vitte.lnk" "$INSTDIR\uninstall.exe"
  WriteRegStr HKCR ".vit" "" "Vitte.Source"
  WriteRegStr HKCR "Vitte.Source" "" "Vitte source file"
  WriteRegStr HKCR "Vitte.Source\shell\check\command" "" '"$INSTDIR\bin\vitte.cmd" check "%1"'
  WriteUninstaller "$INSTDIR\uninstall.exe"
  WriteRegStr HKLM "Software\Vitte" "InstallDir" "$INSTDIR"
  WriteRegStr HKLM "Software\Vitte" "Architecture" "${ARCH}"
  WriteRegStr HKLM "Software\Vitte" "SupportedWindows" "${WINDOWS_TARGETS}"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Vitte" "DisplayName" "Vitte ${VERSION} (${ARCH})"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Vitte" "DisplayVersion" "${VERSION}"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Vitte" "Publisher" "Vitte Team"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Vitte" "InstallLocation" "$INSTDIR"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Vitte" "UninstallString" '"$INSTDIR\uninstall.exe"'
SectionEnd

Section "Uninstall"
  RMDir /r "$SMPROGRAMS\Vitte"
  DeleteRegKey HKCR "Vitte.Source"
  DeleteRegKey HKCR ".vit"
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Vitte"
  DeleteRegKey HKLM "Software\Vitte"
  RMDir /r "$INSTDIR"
SectionEnd

VIProductVersion "${VERSION}.0"
VIAddVersionKey /LANG=${LANG_ENGLISH} "ProductName" "Vitte"
VIAddVersionKey /LANG=${LANG_ENGLISH} "ProductVersion" "${VERSION}"
VIAddVersionKey /LANG=${LANG_ENGLISH} "FileDescription" "Vitte ${ARCH} installer for Windows XP through Windows 11"
NSI
}

build_one() {
  arch=$1
  stage=$ROOT_DIR/target/installer-windows-$arch
  payload=$stage/payload
  package_file=$OUT_DIR/${PACKAGE_NAME}-${VERSION}-windows-${arch}-installer.exe
  kit_file=$OUT_DIR/${PACKAGE_NAME}-${VERSION}-windows-${arch}-nsis.tar.gz
  machine=$(machine_for_arch "$arch")

  rm -rf "$stage"
  mkdir -p "$payload/bin" "$payload/share/vitte" "$OUT_DIR"

  VERSION=$VERSION "$ROOT_DIR/scripts_build/stage-installer-payload.sh" "$payload" windows "$arch" windows
  [ ! -f "$ROOT_DIR/LICENSE" ] || install -m 0644 "$ROOT_DIR/LICENSE" "$stage/LICENSE"
  [ ! -f "$ROOT_DIR/README.md" ] || install -m 0644 "$ROOT_DIR/README.md" "$stage/README.md"
  [ ! -f "$ROOT_DIR/assets/logo.png" ] || install -m 0644 "$ROOT_DIR/assets/logo.png" "$stage/logo.png"

  has_pe=0
  if exe=$(find_windows_exe "$arch"); then
    validate_pe "$exe" "$arch" || die "invalid Windows $arch compiler payload"
    install -m 0755 "$exe" "$payload/bin/vitte.exe"
    has_pe=1
  fi

  write_cmd_shims "$payload"
  write_nsi "$stage" "$arch"

  cat > "$stage/BUILD.txt" <<EOF
Vitte Windows $arch NSIS build kit
Version: $VERSION
Processor: $arch (PE machine $machine)
Supported Windows: $WINDOWS_TARGETS
Installer runtime target: Windows XP through Windows 11

To create a native EXE installer:
  1. Put a valid $arch PE compiler at payload/bin/vitte.exe.
  2. Run:
       makensis -DVERSION=$VERSION -DARCH=$arch -DWINDOWS_TARGETS="$WINDOWS_TARGETS" -DOUT_FILE=${PACKAGE_NAME}-${VERSION}-windows-${arch}-installer.exe installer.nsi

The canonical generator validates the PE header before invoking NSIS:
  ARCH=$arch WINDOWS_VITTE_EXE=/path/to/vitte.exe scripts_build/build-windows-installer.sh
EOF

  rm -f "$kit_file" "$kit_file.sha256"
  scripts_build_tar_gz "$kit_file" "$stage" installer.nsi BUILD.txt LICENSE README.md logo.png payload
  scripts_build_sha256_write "$kit_file" "$kit_file.sha256"
  printf '[build-windows-installer] wrote build kit %s (%s bytes)\n' "$kit_file" "$(wc -c < "$kit_file" | tr -d ' ')"

  if [ "$has_pe" -eq 1 ] && command -v makensis >/dev/null 2>&1; then
    rm -f "$package_file" "$package_file.sha256"
    (cd "$stage" && makensis -DVERSION="$VERSION" -DARCH="$arch" -DWINDOWS_TARGETS="$WINDOWS_TARGETS" -DOUT_FILE="$package_file" installer.nsi)
    validate_pe "$package_file" "$arch" || die "NSIS output is not a Windows $arch executable"
    scripts_build_sha256_write "$package_file" "$package_file.sha256"
    printf '[build-windows-installer] wrote %s (%s bytes)\n' "$package_file" "$(wc -c < "$package_file" | tr -d ' ')"
  elif [ "$has_pe" -eq 0 ]; then
    printf '[build-windows-installer] Windows %s PE payload unavailable; NSIS kit generated, native .exe deferred\n' "$arch" >&2
  else
    printf '[build-windows-installer] makensis unavailable; NSIS kit generated, native .exe deferred\n' >&2
  fi
}

for tool in install python3 tar wc; do
  require "$tool"
done

[ -x "$ROOT_DIR/scripts_build/stage-installer-payload.sh" ] ||
  die "payload staging script is missing or not executable"

case "$ARCH" in
  all)
    arches='i386 amd64 arm64 armv7'
    ;;
  x86_64 | amd64 | x64)
    arches=amd64
    ;;
  i386 | i486 | i586 | i686 | x86)
    arches=i386
    ;;
  aarch64 | arm64)
    arches=arm64
    ;;
  armv7 | arm)
    arches=armv7
    ;;
  *)
    die "unsupported Windows architecture: $ARCH"
    ;;
esac

for arch in $arches; do
  build_one "$arch"
done

printf '[build-windows-installer] complete version=%s arch=%s out=%s targets=%s\n' \
  "$VERSION" "$ARCH" "$OUT_DIR" "$WINDOWS_TARGETS"
