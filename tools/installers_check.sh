#!/bin/sh
set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
REPORT_DIR=$ROOT_DIR/target/reports/installers
mkdir -p "$REPORT_DIR"

for script in \
  "$ROOT_DIR"/scripts_build/*.sh \
  "$ROOT_DIR"/tools/scripts_build_*_test.sh \
  "$ROOT_DIR"/toolchain/scripts/package/*.sh \
  "$ROOT_DIR"/toolchain/scripts/install/*.sh
do
  sh -n "$script"
done

"$ROOT_DIR/scripts_build/package-matrix.sh" list > "$REPORT_DIR/package-matrix.txt"
"$ROOT_DIR/tools/package_platform_matrix_test.sh"
"$ROOT_DIR/tools/scripts_build_staging_test.sh"
"$ROOT_DIR/tools/scripts_build_checksum_fallback_test.sh"
"$ROOT_DIR/tools/scripts_build_reproducibility_test.sh"
"$ROOT_DIR/tools/scripts_build_arch_matrix_test.sh"

if command -v shellcheck >/dev/null 2>&1; then
  shellcheck "$ROOT_DIR"/scripts_build/*.sh "$ROOT_DIR"/tools/scripts_build_*_test.sh
else
  printf '[installers-check] shellcheck unavailable; skipped locally\n' >&2
fi

python3 - "$ROOT_DIR" "$REPORT_DIR/installers-check.json" <<'PY'
import json
import re
import sys
from pathlib import Path

root = Path(sys.argv[1])
out = Path(sys.argv[2])
package_version = (root / "toolchain/scripts/package/PACKAGE_VERSION").read_text(encoding="utf-8").strip()
violations = []
version_violations = []
allowed = (
    "/usr/local",
    "/usr/bin/env",
    "/bin/sh",
    "/dev/null",
    "/tmp",
    "/var",
    "/Users",
    "/usr/share",
    "/etc",
    "/Library",
    "/opt",
    "/Payload/usr/local",
    "/root/usr/local",
)
for path in sorted((root / "scripts_build").glob("*.sh")):
    text = path.read_text(encoding="utf-8")
    for match in re.finditer(r'(?<![A-Za-z0-9_])/(?:[A-Za-z0-9._ -]+/)+[A-Za-z0-9._-]+', text):
        value = match.group(0)
        if match.start() >= 2 and text[match.start() - 2 : match.start()] == ":/":
            continue
        if value.startswith(("/fd ", "/path/to/")):
            continue
        if not value.startswith(allowed) and "Application Support" not in value:
            violations.append(f"{path.relative_to(root)}:{match.start()}: {value}")

if package_version != "0.1.0":
    version_violations.append(f"toolchain/scripts/package/PACKAGE_VERSION is {package_version!r}, expected '0.1.0'")

version_checked_paths = [
    root / "Makefile",
    root / "man/vitte.1",
    root / "man/vittec.1",
    root / "toolchain/scripts/install/templates/env.sh",
    root / "toolchain/scripts/package/PACKAGE_VERSION",
    root / "toolchain/scripts/package/make-debian-deb.sh",
    root / "toolchain/scripts/package/make-macos-pkg.sh",
    root / "toolchain/scripts/package/make-macos-uninstall-pkg.sh",
    root / "toolchain/scripts/package/windows/vitte-installer.nsi",
]
for path in version_checked_paths:
    text = path.read_text(encoding="utf-8")
    rel = path.relative_to(root)
    if "2.1.1" in text:
        version_violations.append(f"{rel}: contains stale version 2.1.1")

macos_pkg = (root / "toolchain/scripts/package/make-macos-pkg.sh").read_text(encoding="utf-8")
if ".zprofile" not in macos_pkg:
    version_violations.append("toolchain/scripts/package/make-macos-pkg.sh: missing zsh login profile support")
if 'export PATH="$_vitte_pkg_bin${PATH:+:$PATH}"' not in macos_pkg:
    version_violations.append("toolchain/scripts/package/make-macos-pkg.sh: shell support must force the installed bin path")
if "set -gx PATH /usr/local/bin $PATH" not in macos_pkg:
    version_violations.append("toolchain/scripts/package/make-macos-pkg.sh: fish support must force the installed bin path")

data = {
    "schema": "org.vitte.installers-check.v1",
    "status": "PASS" if not violations and not version_violations else "FAIL",
    "package_version": package_version,
    "absolute_path_violations": violations,
    "version_violations": version_violations,
}
out.write_text(json.dumps(data, indent=2, sort_keys=True) + "\n", encoding="utf-8")
if violations:
    raise SystemExit("unexpected absolute installer paths: " + "; ".join(violations[:5]))
if version_violations:
    raise SystemExit("installer version/shell violations: " + "; ".join(version_violations[:5]))
PY

printf '[installers-check] OK\n'
