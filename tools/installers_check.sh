#!/bin/sh
set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
REPORT_DIR=$ROOT_DIR/target/reports/installers
mkdir -p "$REPORT_DIR"

for script in "$ROOT_DIR"/scripts_build/*.sh "$ROOT_DIR"/tools/scripts_build_*_test.sh; do
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
violations = []
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

data = {
    "schema": "org.vitte.installers-check.v1",
    "status": "PASS" if not violations else "FAIL",
    "absolute_path_violations": violations,
}
out.write_text(json.dumps(data, indent=2, sort_keys=True) + "\n", encoding="utf-8")
if violations:
    raise SystemExit("unexpected absolute installer paths: " + "; ".join(violations[:5]))
PY

printf '[installers-check] OK\n'
