#!/bin/sh
set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
VERSION=${VERSION:-$(tr -d ' \r\n' < "$ROOT_DIR/toolchain/scripts/package/PACKAGE_VERSION")}
OUT_DIR=${OUT_DIR:-$ROOT_DIR/pkgout}
ARCH=${ARCH:-amd64}
PACKAGE_NAME=${PACKAGE_NAME:-vitte}
WINDOWS_VITTE_EXE=${WINDOWS_VITTE_EXE:-}

die() {
  printf '[build-windows-installer][error] %s\n' "$*" >&2
  exit 1
}

require() {
  command -v "$1" >/dev/null 2>&1 || die "missing required tool: $1"
}

validate_pe_amd64() {
  python3 - "$1" <<'PY'
import struct
import sys
from pathlib import Path

path = Path(sys.argv[1])
data = path.read_bytes()
if len(data) < 64 or data[:2] != b"MZ":
    raise SystemExit(f"not a PE executable: {path}")
offset = struct.unpack_from("<I", data, 0x3C)[0]
if offset + 6 > len(data) or data[offset:offset + 4] != b"PE\0\0":
    raise SystemExit(f"invalid PE signature: {path}")
machine = struct.unpack_from("<H", data, offset + 4)[0]
if machine != 0x8664:
    raise SystemExit(f"wrong PE processor 0x{machine:04x}, expected amd64 (0x8664): {path}")
PY
}

case "$ARCH" in
  x86_64|amd64) ARCH=amd64 ;;
  *) die "unsupported Windows architecture: $ARCH (only amd64 is supported)" ;;
esac

for tool in install python3 shasum tar; do
  require "$tool"
done

stage=$ROOT_DIR/target/installer-windows-$ARCH
payload=$stage/payload
package_file=$OUT_DIR/${PACKAGE_NAME}-${VERSION}-windows-${ARCH}-installer.exe
kit_file=$OUT_DIR/${PACKAGE_NAME}-${VERSION}-windows-${ARCH}-nsis.tar.gz

rm -rf "$stage"
mkdir -p "$payload/bin" "$payload/share/vitte" "$OUT_DIR"
VERSION=$VERSION "$ROOT_DIR/scripts_build/stage-installer-payload.sh" "$payload" windows "$ARCH" windows
[ ! -f "$ROOT_DIR/LICENSE" ] || install -m 0644 "$ROOT_DIR/LICENSE" "$stage/LICENSE"
[ ! -f "$ROOT_DIR/README.md" ] || install -m 0644 "$ROOT_DIR/README.md" "$stage/README.md"

if [ -z "$WINDOWS_VITTE_EXE" ]; then
  for candidate in \
    "$ROOT_DIR/target/windows-amd64/vitte.exe" \
    "$ROOT_DIR/target/windows-x86_64/vitte.exe" \
    "$ROOT_DIR/bin/windows-amd64/vitte.exe"; do
    if [ -f "$candidate" ]; then
      WINDOWS_VITTE_EXE=$candidate
      break
    fi
  done
fi

has_pe=0
if [ -n "$WINDOWS_VITTE_EXE" ]; then
  [ -f "$WINDOWS_VITTE_EXE" ] || die "WINDOWS_VITTE_EXE does not exist: $WINDOWS_VITTE_EXE"
  validate_pe_amd64 "$WINDOWS_VITTE_EXE" || die "invalid Windows amd64 compiler payload"
  install -m 0755 "$WINDOWS_VITTE_EXE" "$payload/bin/vitte.exe"
  has_pe=1
fi

cat > "$stage/installer.nsi" <<'NSI'
Unicode true
!include "MUI2.nsh"
!include "x64.nsh"

!ifndef VERSION
  !define VERSION "0.0.0"
!endif
!ifndef OUT_FILE
  !define OUT_FILE "vitte-windows-amd64-installer.exe"
!endif

Name "Vitte ${VERSION} (amd64)"
OutFile "${OUT_FILE}"
InstallDir "$PROGRAMFILES64\Vitte"
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

Section "Vitte" SEC_VITTE
  SectionIn RO
  SetOutPath "$INSTDIR"
  File /r "payload\*"
  WriteUninstaller "$INSTDIR\uninstall.exe"
  WriteRegStr HKLM "Software\Vitte" "InstallDir" "$INSTDIR"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Vitte" "DisplayName" "Vitte ${VERSION} (amd64)"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Vitte" "DisplayVersion" "${VERSION}"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Vitte" "Publisher" "Vitte Team"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Vitte" "InstallLocation" "$INSTDIR"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Vitte" "UninstallString" '"$INSTDIR\uninstall.exe"'
SectionEnd

Section "Uninstall"
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Vitte"
  DeleteRegKey HKLM "Software\Vitte"
  RMDir /r "$INSTDIR"
SectionEnd

VIProductVersion "${VERSION}.0"
VIAddVersionKey /LANG=${LANG_ENGLISH} "ProductName" "Vitte"
VIAddVersionKey /LANG=${LANG_ENGLISH} "ProductVersion" "${VERSION}"
VIAddVersionKey /LANG=${LANG_ENGLISH} "FileDescription" "Vitte amd64 installer"
NSI

cat > "$stage/BUILD.txt" <<EOF
Vitte Windows amd64 NSIS build kit
Version: $VERSION
Processor: amd64 (PE machine 0x8664)

Place a PE32+ amd64 compiler at payload/bin/vitte.exe, then run:
  makensis -DVERSION=$VERSION -DOUT_FILE=${PACKAGE_NAME}-${VERSION}-windows-amd64-installer.exe installer.nsi

The canonical generator validates the PE header before invoking NSIS:
  WINDOWS_VITTE_EXE=/path/to/vitte.exe scripts_build/build-windows-installer.sh
EOF

COPYFILE_DISABLE=1 tar -czf "$kit_file" -C "$stage" installer.nsi BUILD.txt LICENSE README.md payload
(cd "$OUT_DIR" && shasum -a 256 "$(basename "$kit_file")" > "$(basename "$kit_file.sha256")")
printf '[build-windows-installer] wrote build kit %s (%s bytes)\n' "$kit_file" "$(wc -c < "$kit_file" | tr -d ' ')"

if [ "$has_pe" -eq 1 ] && command -v makensis >/dev/null 2>&1; then
  rm -f "$package_file"
  (cd "$stage" && makensis -DVERSION="$VERSION" -DOUT_FILE="$package_file" installer.nsi)
  validate_pe_amd64 "$package_file" || die "NSIS output is not a Windows amd64 executable"
  (cd "$OUT_DIR" && shasum -a 256 "$(basename "$package_file")" > "$(basename "$package_file.sha256")")
  printf '[build-windows-installer] wrote %s (%s bytes)\n' "$package_file" "$(wc -c < "$package_file" | tr -d ' ')"
elif [ "$has_pe" -eq 0 ]; then
  printf '[build-windows-installer] Windows amd64 PE payload unavailable; NSIS kit generated, native .exe deferred\n' >&2
else
  printf '[build-windows-installer] makensis unavailable; NSIS kit generated, native .exe deferred\n' >&2
fi

printf '[build-windows-installer] complete version=%s arch=%s out=%s\n' "$VERSION" "$ARCH" "$OUT_DIR"
