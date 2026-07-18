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

validate_pe() {
  python3 - "$1" "$2" <<'PY'
import struct
import sys
from pathlib import Path

path = Path(sys.argv[1])
expected = {"amd64": 0x8664, "i386": 0x014c}[sys.argv[2]]
data = path.read_bytes()
if len(data) < 64 or data[:2] != b"MZ":
    raise SystemExit(f"not a PE executable: {path}")
offset = struct.unpack_from("<I", data, 0x3C)[0]
if offset + 6 > len(data) or data[offset:offset + 4] != b"PE\0\0":
    raise SystemExit(f"invalid PE signature: {path}")
machine = struct.unpack_from("<H", data, offset + 4)[0]
if machine != expected:
    raise SystemExit(f"wrong PE processor 0x{machine:04x}, expected {sys.argv[2]} (0x{expected:04x}): {path}")
PY
}

case "$ARCH" in
  x86_64|amd64) ARCH=amd64 ;;
  i386|i486|i586|i686|x86) ARCH=i386 ;;
  *) die "unsupported Windows architecture: $ARCH" ;;
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
  for candidate in "$ROOT_DIR/target/windows-$ARCH/vitte.exe" "$ROOT_DIR/bin/windows-$ARCH/vitte.exe"; do
    if [ -f "$candidate" ]; then
      WINDOWS_VITTE_EXE=$candidate
      break
    fi
  done
fi

has_pe=0
if [ -n "$WINDOWS_VITTE_EXE" ]; then
  [ -f "$WINDOWS_VITTE_EXE" ] || die "WINDOWS_VITTE_EXE does not exist: $WINDOWS_VITTE_EXE"
  validate_pe "$WINDOWS_VITTE_EXE" "$ARCH" || die "invalid Windows $ARCH compiler payload"
  install -m 0755 "$WINDOWS_VITTE_EXE" "$payload/bin/vitte.exe"
  has_pe=1
fi

for command in vitte vittec vittec0; do
  cat > "$payload/bin/$command.cmd" <<EOF
@echo off
set "VITTE_ROOT=%~dp0..\share\vitte"
"%~dp0vitte.exe" %*
EOF
done
cat > "$payload/Install-VittePath.ps1" <<'PS1'
$ErrorActionPreference = 'Stop'
$bin = Join-Path $PSScriptRoot 'bin'
$path = [Environment]::GetEnvironmentVariable('Path', 'Machine')
if (($path -split ';') -notcontains $bin) {
  [Environment]::SetEnvironmentVariable('Path', ($path.TrimEnd(';') + ';' + $bin), 'Machine')
}
[Environment]::SetEnvironmentVariable('VITTE_ROOT', (Join-Path $PSScriptRoot 'share\vitte'), 'Machine')
PS1
cat > "$payload/Remove-VittePath.ps1" <<'PS1'
$ErrorActionPreference = 'Stop'
$bin = Join-Path $PSScriptRoot 'bin'
$path = [Environment]::GetEnvironmentVariable('Path', 'Machine')
$clean = (($path -split ';') | Where-Object { $_ -and $_ -ne $bin }) -join ';'
[Environment]::SetEnvironmentVariable('Path', $clean, 'Machine')
[Environment]::SetEnvironmentVariable('VITTE_ROOT', $null, 'Machine')
PS1

cat > "$stage/installer.nsi" <<'NSI'
Unicode true
!include "MUI2.nsh"
!include "x64.nsh"

!ifndef VERSION
  !define VERSION "0.0.0"
!endif
!ifndef OUT_FILE
  !define OUT_FILE "vitte-windows-installer.exe"
!endif
!ifndef ARCH
  !define ARCH "amd64"
!endif

Name "Vitte ${VERSION} (${ARCH})"
OutFile "${OUT_FILE}"
!if "${ARCH}" == "amd64"
InstallDir "$PROGRAMFILES64\Vitte"
!else
InstallDir "$PROGRAMFILES\Vitte"
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

Section "Vitte" SEC_VITTE
  SectionIn RO
  SetOutPath "$INSTDIR"
  File /r "payload\*"
  ExecWait 'powershell.exe -NoProfile -ExecutionPolicy Bypass -File "$INSTDIR\Install-VittePath.ps1"'
  CreateDirectory "$SMPROGRAMS\Vitte"
  CreateShortCut "$SMPROGRAMS\Vitte\Vitte shell.lnk" "$SYSDIR\cmd.exe" '/K ""$INSTDIR\bin\vitte.cmd" --help"'
  CreateShortCut "$SMPROGRAMS\Vitte\Uninstall Vitte.lnk" "$INSTDIR\uninstall.exe"
  WriteRegStr HKCR ".vit" "" "Vitte.Source"
  WriteRegStr HKCR "Vitte.Source" "" "Vitte source file"
  WriteRegStr HKCR "Vitte.Source\shell\check\command" "" '"$INSTDIR\bin\vitte.cmd" check "%1"'
  WriteUninstaller "$INSTDIR\uninstall.exe"
  WriteRegStr HKLM "Software\Vitte" "InstallDir" "$INSTDIR"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Vitte" "DisplayName" "Vitte ${VERSION} (${ARCH})"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Vitte" "DisplayVersion" "${VERSION}"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Vitte" "Publisher" "Vitte Team"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Vitte" "InstallLocation" "$INSTDIR"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Vitte" "UninstallString" '"$INSTDIR\uninstall.exe"'
SectionEnd

Section "Uninstall"
  ExecWait 'powershell.exe -NoProfile -ExecutionPolicy Bypass -File "$INSTDIR\Remove-VittePath.ps1"'
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
VIAddVersionKey /LANG=${LANG_ENGLISH} "FileDescription" "Vitte ${ARCH} installer"
NSI

cat > "$stage/BUILD.txt" <<EOF
Vitte Windows $ARCH NSIS build kit
Version: $VERSION
Processor: $ARCH (PE machine $(if [ "$ARCH" = amd64 ]; then printf 0x8664; else printf 0x014c; fi))

Place a PE compiler for $ARCH at payload/bin/vitte.exe, then run:
  makensis -DVERSION=$VERSION -DARCH=$ARCH -DOUT_FILE=${PACKAGE_NAME}-${VERSION}-windows-${ARCH}-installer.exe installer.nsi

The canonical generator validates the PE header before invoking NSIS:
  WINDOWS_VITTE_EXE=/path/to/vitte.exe scripts_build/build-windows-installer.sh
EOF

COPYFILE_DISABLE=1 tar -czf "$kit_file" -C "$stage" installer.nsi BUILD.txt LICENSE README.md payload
(cd "$OUT_DIR" && shasum -a 256 "$(basename "$kit_file")" > "$(basename "$kit_file.sha256")")
printf '[build-windows-installer] wrote build kit %s (%s bytes)\n' "$kit_file" "$(wc -c < "$kit_file" | tr -d ' ')"

if [ "$has_pe" -eq 1 ] && command -v makensis >/dev/null 2>&1; then
  rm -f "$package_file"
  (cd "$stage" && makensis -DVERSION="$VERSION" -DARCH="$ARCH" -DOUT_FILE="$package_file" installer.nsi)
  validate_pe "$package_file" "$ARCH" || die "NSIS output is not a Windows $ARCH executable"
  (cd "$OUT_DIR" && shasum -a 256 "$(basename "$package_file")" > "$(basename "$package_file.sha256")")
  printf '[build-windows-installer] wrote %s (%s bytes)\n' "$package_file" "$(wc -c < "$package_file" | tr -d ' ')"
elif [ "$has_pe" -eq 0 ]; then
  printf '[build-windows-installer] Windows %s PE payload unavailable; NSIS kit generated, native .exe deferred\n' "$ARCH" >&2
else
  printf '[build-windows-installer] makensis unavailable; NSIS kit generated, native .exe deferred\n' >&2
fi

printf '[build-windows-installer] complete version=%s arch=%s out=%s\n' "$VERSION" "$ARCH" "$OUT_DIR"
