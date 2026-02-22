#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ISSUES_FILE="${ROOT_DIR}/vitteos/issues.yaml"
ROADMAP_FILE="${ROOT_DIR}/vitteos-roadmap.md"

if [[ ! -f "${ISSUES_FILE}" ]]; then
  echo "missing file: vitteos/issues.yaml" >&2
  exit 1
fi

if [[ ! -f "${ROADMAP_FILE}" ]]; then
  echo "missing file: vitteos-roadmap.md" >&2
  exit 1
fi

python3 - "${ISSUES_FILE}" "${ROADMAP_FILE}" <<'PY'
import re
import sys
from pathlib import Path

issues_path = Path(sys.argv[1])
roadmap_path = Path(sys.argv[2])
text = issues_path.read_text(encoding="utf-8")
roadmap = roadmap_path.read_text(encoding="utf-8")

required = ("M1", "M2", "M3")
errors = []

for m in required:
    block = re.search(rf"^\s{{2}}{m}:\n(?P<body>(?:^\s{{4}}.*\n?)*)", text, flags=re.MULTILINE)
    if not block:
        errors.append(f"missing milestone block in issues.yaml: {m}")
        continue
    body = block.group("body")
    tickets = re.findall(r"^\s{6}-\s+(.+)$", body, flags=re.MULTILINE)
    if not tickets:
        errors.append(f"missing tickets list for milestone {m} in issues.yaml")
    if m not in roadmap:
        errors.append(f"roadmap does not reference milestone id: {m}")

if errors:
    for e in errors:
        print(e, file=sys.stderr)
    sys.exit(1)

print("vitteos issues mapping: OK")
PY
