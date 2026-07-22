#!/bin/sh
set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
SCRIPT_NAME=stage-installer-payload
. "$ROOT_DIR/scripts_build/common.sh"
case "${1:-}" in
  --dry-run)
    shift
    DRY_RUN=1
    ;;
  --help | -h)
    printf '%s\n' "usage: stage-installer-payload.sh [--dry-run] DEST PLATFORM ARCH [unix|windows]"
    exit 0
    ;;
esac
DEST=${1:?usage: stage-installer-payload.sh DEST PLATFORM ARCH [unix|windows]}
PLATFORM=${2:?missing platform}
ARCH=${3:?missing architecture}
LAYOUT=${4:-unix}
VERSION=${VERSION:-$(tr -d ' \r\n' < "$ROOT_DIR/toolchain/scripts/package/PACKAGE_VERSION")}
STRICT_PROCESSOR=${STRICT_PROCESSOR:-0}
VITTE_ABSOLUTE=$(install_vitte_if_missing)
verify_vitte "$VITTE_ABSOLUTE" >/dev/null
scripts_build_maybe_dry_run "would stage installer payload platform=$PLATFORM arch=$ARCH layout=$LAYOUT dest=$DEST"

die() {
  printf '[stage-installer-payload][error] %s\n' "$*" >&2
  exit 1
}

copy_tree() {
  source=$1
  destination=$2
  scripts_build_copy_tree "$source" "$destination"
}

normalize_key() {
  printf '%s' "$1" | tr '[:lower:]-' '[:upper:]_'
}

host_processor_label() {
  case "$(uname -m 2>/dev/null || printf unknown)" in
    x86_64 | amd64) printf 'amd64' ;;
    arm64 | aarch64) printf 'arm64' ;;
    i386 | i486 | i586 | i686) printf 'i386' ;;
    armv6*) printf 'armv6' ;;
    armv7*) printf 'armv7' ;;
    riscv64*) printf 'riscv64' ;;
    ppc64le | powerpc64le) printf 'powerpc64le' ;;
    ppc64 | powerpc64) printf 'powerpc64' ;;
    ppc | powerpc) printf 'powerpc' ;;
    sparc64*) printf 'sparc64' ;;
    *) printf 'unknown' ;;
  esac
}

find_command_payload() {
  command=$1
  platform_key=$(normalize_key "$PLATFORM")
  arch_key=$(normalize_key "$ARCH")
  command_key=$(normalize_key "$command")

  eval "override=\${VITTE_${command_key}_${platform_key}_${arch_key}:-}"
  [ -z "${override:-}" ] || {
    printf '%s\n' "$override"
    return 0
  }

  eval "override=\${VITTE_${command_key}_${arch_key}:-}"
  [ -z "${override:-}" ] || {
    printf '%s\n' "$override"
    return 0
  }

  eval "override=\${VITTE_BIN_${platform_key}_${arch_key}:-}"
  [ -z "${override:-}" ] || {
    printf '%s\n' "$override"
    return 0
  }

  eval "override=\${VITTE_BIN_${arch_key}:-}"
  [ -z "${override:-}" ] || {
    printf '%s\n' "$override"
    return 0
  }

  for candidate in \
    "$ROOT_DIR/target/$PLATFORM-$ARCH/$command" \
    "$ROOT_DIR/target/$PLATFORM-$ARCH/vitte" \
    "$ROOT_DIR/target/$ARCH/$command" \
    "$ROOT_DIR/bin/$PLATFORM-$ARCH/$command" \
    "$ROOT_DIR/bin/$ARCH/$command"
  do
    if [ -x "$candidate" ]; then
      printf '%s\n' "$candidate"
      return 0
    fi
  done

  host_arch=$(host_processor_label)
  if [ "$ARCH" = "$host_arch" ] && [ -x "$ROOT_DIR/bin/$command" ]; then
    printf '%s\n' "$ROOT_DIR/bin/$command"
    return 0
  fi

  if [ -x "$ROOT_DIR/bin/$command" ] && [ "$STRICT_PROCESSOR" -eq 0 ]; then
    printf '%s\n' "$ROOT_DIR/bin/$command"
    return 0
  fi

  return 1
}

install_unix_command() {
  command=$1
  libexec_dir=$2
  bin_dir=$3
  payload=$(find_command_payload "$command") ||
    die "missing executable payload for $PLATFORM/$ARCH command $command; set VITTE_$(normalize_key "$command")_$(normalize_key "$PLATFORM")_$(normalize_key "$ARCH") or STRICT_PROCESSOR=0"

  install -m 0755 "$payload" "$libexec_dir/$command"
  cat > "$bin_dir/$command" <<EOF
#!/bin/sh
set -eu
export VITTE_ROOT=\${VITTE_ROOT:-/usr/local/share/vitte}
exec /usr/local/libexec/vitte/$command "\$@"
EOF
  chmod 0755 "$bin_dir/$command"
}

install_windows_shims() {
  bin_dir=$1

  for command in vitte vittec vittec0; do
    cat > "$bin_dir/$command.cmd" <<EOF
@echo off
setlocal
set "VITTE_ROOT=%~dp0..\share\vitte"
if exist "%~dp0$command.exe" (
  "%~dp0$command.exe" %*
) else (
  "%~dp0vitte.exe" %*
)
exit /b %ERRORLEVEL%
EOF
    cat > "$bin_dir/$command.ps1" <<EOF
\$ErrorActionPreference = "Stop"
\$env:VITTE_ROOT = Join-Path \$PSScriptRoot "..\\share\\vitte"
\$command = Join-Path \$PSScriptRoot "$command.exe"
if (-not (Test-Path \$command)) {
  \$command = Join-Path \$PSScriptRoot "vitte.exe"
}
& \$command @args
exit \$LASTEXITCODE
EOF
  done

  cat > "$bin_dir/vitte-installer-doctor.cmd" <<'EOF'
@echo off
setlocal
set "PREFIX=%~dp0.."
set "VITTE_ROOT=%~dp0..\share\vitte"
set "STATUS=0"
echo Vitte installer doctor
echo prefix: %PREFIX%
echo VITTE_ROOT: %VITTE_ROOT%
for %%C in (vitte vittec vittec0) do (
  if exist "%~dp0%%C.cmd" (
    echo OK   wrapper exists: %~dp0%%C.cmd
  ) else (
    echo FAIL missing wrapper: %~dp0%%C.cmd
    set "STATUS=1"
  )
)
if exist "%VITTE_ROOT%\VERSION" (
  echo OK   version file exists: %VITTE_ROOT%\VERSION
) else (
  echo FAIL missing version file: %VITTE_ROOT%\VERSION
  set "STATUS=1"
)
if exist "%VITTE_ROOT%\INSTALLATION.json" (
  echo OK   installation manifest exists: %VITTE_ROOT%\INSTALLATION.json
) else (
  echo FAIL missing installation manifest: %VITTE_ROOT%\INSTALLATION.json
  set "STATUS=1"
)
if exist "%~dp0vitte.cmd" (
  "%~dp0vitte.cmd" --version >nul 2>nul
  if errorlevel 1 (
    echo FAIL vitte --version failed
    set "STATUS=1"
  ) else (
    echo OK   vitte --version runs
  )
  "%~dp0vitte.cmd" --help >nul 2>nul
  if errorlevel 1 (
    echo FAIL vitte --help failed
    set "STATUS=1"
  ) else (
    echo OK   vitte --help runs
  )
  set "SMOKE=%TEMP%\vitte-installer-doctor-%RANDOM%"
  mkdir "%SMOKE%" >nul 2>nul
  > "%SMOKE%\smoke.vit" echo proc main() -^> int {
  >> "%SMOKE%\smoke.vit" echo   give 0
  >> "%SMOKE%\smoke.vit" echo }
  pushd "%SMOKE%" >nul
  "%~dp0vitte.cmd" check smoke.vit >nul 2>nul
  if errorlevel 1 (
    echo FAIL vitte check smoke.vit failed
    set "STATUS=1"
  ) else (
    echo OK   vitte check smoke.vit runs
  )
  "%~dp0vitte.cmd" build smoke.vit -o smoke >nul 2>nul
  if errorlevel 1 (
    echo FAIL vitte build smoke.vit -o smoke failed
    set "STATUS=1"
  ) else (
    echo OK   vitte build smoke.vit -o smoke runs
    if exist smoke.exe smoke.exe >nul 2>nul
    if errorlevel 1 (
      echo FAIL built smoke executable failed
      set "STATUS=1"
    ) else (
      echo OK   built smoke executable runs or is not present
    )
  )
  popd >nul
  rmdir /s /q "%SMOKE%" >nul 2>nul
)
if "%STATUS%"=="0" (
  echo installer status: OK
) else (
  echo installer status: FAIL
)
exit /b %STATUS%
EOF
}

case "$LAYOUT" in
  unix)
    prefix=$DEST/usr/local
    bin_dir=$prefix/bin
    libexec_dir=$prefix/libexec/vitte
    share_dir=$prefix/share/vitte
    mkdir -p "$bin_dir" "$libexec_dir" "$share_dir"
    for command in vitte vittec vittec0; do
      install_unix_command "$command" "$libexec_dir" "$bin_dir"
    done
    install -m 0755 "$ROOT_DIR/scripts/installer-doctor.sh" "$bin_dir/vitte-installer-doctor"
    copy_tree "$ROOT_DIR/man" "$prefix/share/man/man1"
    mkdir -p \
      "$prefix/etc/bash_completion.d" \
      "$prefix/share/zsh/site-functions" \
      "$prefix/share/fish/vendor_completions.d"
    [ ! -f "$ROOT_DIR/completions/bash/vitte" ] ||
      install -m 0644 "$ROOT_DIR/completions/bash/vitte" "$prefix/etc/bash_completion.d/vitte"
    [ ! -f "$ROOT_DIR/completions/zsh/_vitte" ] ||
      install -m 0644 "$ROOT_DIR/completions/zsh/_vitte" "$prefix/share/zsh/site-functions/_vitte"
    [ ! -f "$ROOT_DIR/completions/fish/vitte.fish" ] ||
      install -m 0644 "$ROOT_DIR/completions/fish/vitte.fish" "$prefix/share/fish/vendor_completions.d/vitte.fish"
    ;;
  windows)
    share_dir=$DEST/share/vitte
    mkdir -p "$DEST/bin" "$share_dir"
    install_windows_shims "$DEST/bin"
    ;;
  *) die "unsupported layout: $LAYOUT" ;;
esac

copy_tree "$ROOT_DIR/src/vitte" "$share_dir/src/vitte"
copy_tree "$ROOT_DIR/toolchain/seed" "$share_dir/toolchain/seed"
for directory in assets completions docs editors examples locales; do
  copy_tree "$ROOT_DIR/$directory" "$share_dir/$directory"
done
for file in LICENSE README.md CHANGELOG.md; do
  [ ! -f "$ROOT_DIR/$file" ] || install -m 0644 "$ROOT_DIR/$file" "$share_dir/$file"
done
printf '%s\n' "$VERSION" > "$share_dir/VERSION"

copy_tree "$ROOT_DIR/src/vitte/packages/registry" "$share_dir/packages/registry"
mkdir -p "$share_dir/packages/compiled" "$share_dir/stdlib/compiled"

python3 - "$ROOT_DIR" "$share_dir" "$PLATFORM" "$ARCH" "$VERSION" "$LAYOUT" <<'PY'
import hashlib
import json
import sys
from pathlib import Path

root = Path(sys.argv[1])
share = Path(sys.argv[2])
platform = sys.argv[3]
arch = sys.argv[4]
version = sys.argv[5]
layout = sys.argv[6]

registry_path = root / "src/vitte/packages/registry/registry.json"
registry = json.loads(registry_path.read_text(encoding="utf-8"))
compiled_packages = share / "packages/compiled"
compiled_packages.mkdir(parents=True, exist_ok=True)
package_rows = []
for package in sorted(registry.get("packages", []), key=lambda item: (item["name"], item["version"])):
    checksum = package.get("checksum", {})
    row = {
        "abi": package.get("abi", ""),
        "arch": arch,
        "checksum": checksum,
        "exports": sorted(package.get("exports", [])),
        "exports_sha256": package.get("exports_sha256", ""),
        "language": package.get("language", ""),
        "name": package["name"],
        "os": platform,
        "schema": "org.vitte.compiled-package-cache-entry.v1",
        "source": package.get("source", ""),
        "version": package["version"],
    }
    package_rows.append(row)
    (compiled_packages / f"{package['name']}-{package['version']}.vittepkg.json").write_text(
        json.dumps(row, indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
    )
(compiled_packages / "packages-manifest.json").write_text(
    json.dumps(
        {
            "arch": arch,
            "package_count": len(package_rows),
            "packages": package_rows,
            "schema": "org.vitte.compiled-package-cache.v1",
            "version": version,
            "os": platform,
        },
        indent=2,
        sort_keys=True,
    )
    + "\n",
    encoding="utf-8",
)

stdlib_compiled = share / "stdlib/compiled"
stdlib_files = []
stdlib_root = root / "src/vitte/stdlib"
if stdlib_root.is_dir():
    for path in sorted(stdlib_root.rglob("*")):
        if path.is_file() and path.suffix in {".vit", ".vitl", ".json"}:
            stdlib_files.append(
                {
                    "path": path.relative_to(stdlib_root).as_posix(),
                    "sha256": hashlib.sha256(path.read_bytes()).hexdigest(),
                    "size": path.stat().st_size,
                }
            )
(stdlib_compiled / "stdlib-manifest.json").write_text(
    json.dumps(
        {
            "arch": arch,
            "file_count": len(stdlib_files),
            "files": stdlib_files,
            "os": platform,
            "schema": "org.vitte.stdlib-compiled-cache.v1",
            "version": version,
        },
        indent=2,
        sort_keys=True,
    )
    + "\n",
    encoding="utf-8",
)

sbom_files = []
for path in sorted(share.rglob("*")):
    if path.is_file() and path.name not in {"CHECKSUMS.sha256", "SBOM.spdx.json", "SBOM.cyclonedx.json"}:
        digest = hashlib.sha256(path.read_bytes()).hexdigest()
        relative = path.relative_to(share).as_posix()
        sbom_files.append({"path": relative, "sha256": digest, "size": path.stat().st_size})

spdx = {
    "SPDXID": "SPDXRef-DOCUMENT",
    "creationInfo": {"creators": ["Tool: vitte scripts_build"], "created": "1970-01-01T00:00:00Z"},
    "dataLicense": "CC0-1.0",
    "name": f"vitte-{version}-{platform}-{arch}",
    "packages": [
        {
            "SPDXID": f"SPDXRef-{index}",
            "checksums": [{"algorithm": "SHA256", "checksumValue": row["sha256"]}],
            "downloadLocation": "NOASSERTION",
            "filesAnalyzed": False,
            "name": row["path"],
        }
        for index, row in enumerate(sbom_files, 1)
    ],
    "spdxVersion": "SPDX-2.3",
}
cyclonedx = {
    "bomFormat": "CycloneDX",
    "components": [
        {
            "hashes": [{"alg": "SHA-256", "content": row["sha256"]}],
            "name": row["path"],
            "type": "file",
            "version": version,
        }
        for row in sbom_files
    ],
    "metadata": {"component": {"name": "vitte", "type": "application", "version": version}},
    "specVersion": "1.5",
    "version": 1,
}
(share / "SBOM.spdx.json").write_text(json.dumps(spdx, indent=2, sort_keys=True) + "\n", encoding="utf-8")
(share / "SBOM.cyclonedx.json").write_text(json.dumps(cyclonedx, indent=2, sort_keys=True) + "\n", encoding="utf-8")

checksum_rows = []
for path in sorted(share.rglob("*")):
    if path.is_file() and path.name != "CHECKSUMS.sha256":
        checksum_rows.append(
            f"{hashlib.sha256(path.read_bytes()).hexdigest()}  {path.relative_to(share).as_posix()}\n"
        )
(share / "CHECKSUMS.sha256").write_text("".join(checksum_rows), encoding="utf-8")
PY

python3 - "$DEST" "$share_dir/INSTALLATION.json" "$PLATFORM" "$ARCH" "$VERSION" "$LAYOUT" <<'PY'
import hashlib
import json
import sys
from pathlib import Path

root = Path(sys.argv[1])
manifest_path = Path(sys.argv[2])
files = []
for path in sorted(root.rglob("*")):
    if path.is_file() and path != manifest_path:
        files.append({
            "path": path.relative_to(root).as_posix(),
            "sha256": hashlib.sha256(path.read_bytes()).hexdigest(),
            "size": path.stat().st_size,
        })
manifest = {
    "schema": "org.vitte.installer-payload.v1",
    "platform": sys.argv[3],
    "architecture": sys.argv[4],
    "version": sys.argv[5],
    "layout": sys.argv[6],
    "components": [
        "compiler", "runtime", "stdlib", "sources", "documentation",
        "examples", "editors", "system-completions", "locales", "assets",
        "compiled-stdlib", "compiled-packages", "local-package-registry",
        "checksums", "sbom", "installer-doctor",
    ],
    "files": files,
}
manifest_path.write_text(json.dumps(manifest, indent=2, sort_keys=True) + "\n", encoding="utf-8")
PY

printf '[stage-installer-payload] complete platform=%s arch=%s layout=%s files=%s\n' \
  "$PLATFORM" "$ARCH" "$LAYOUT" "$(find "$DEST" -type f | wc -l | tr -d ' ')"
