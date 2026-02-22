#!/usr/bin/env bash
set -euo pipefail

ROOT="VitteOS_voyager49/vitte_os_voyager49_kernel/bin"
STATUS_FILE="${ROOT}/MIGRATION_STATUS.md"
OUT_DIR="target/reports"
OUT_MD="${OUT_DIR}/vitteos_bin_matrix.md"
OUT_JSON="${OUT_DIR}/vitteos_bin_matrix.json"

mkdir -p "$OUT_DIR"

if [[ ! -d "${ROOT}" || ! -f "${STATUS_FILE}" ]]; then
  echo "vitteos-bin-matrix-report: skipped (missing ${STATUS_FILE})"
  exit 0
fi

awk -F'|' '
BEGIN {
  print "# VitteOS Bin Matrix"
  print ""
  print "| command | status | tested | runtime | manpage | entry |"
  print "|---|---|---|---|---|---|"
}
/^\|/ {
  c=$2; st=$3; t=$4; r=$5; e=$6
  gsub(/^[ \t]+|[ \t]+$/, "", c)
  gsub(/^[ \t]+|[ \t]+$/, "", st)
  gsub(/^[ \t]+|[ \t]+$/, "", t)
  gsub(/^[ \t]+|[ \t]+$/, "", r)
  gsub(/^[ \t]+|[ \t]+$/, "", e)
  if (c=="" || c=="command") next
  man = "bin/" c "/" c ".1"
  print "| " c " | " st " | " t " | " r " | " man " | " e " |"
}
' "$STATUS_FILE" > "$OUT_MD"

python3 - <<'PY'
import json
from pathlib import Path
p = Path("VitteOS_voyager49/vitte_os_voyager49_kernel/bin/MIGRATION_STATUS.md")
out = []
for line in p.read_text().splitlines():
    if not line.startswith("|"):
        continue
    cols = [c.strip() for c in line.split("|")[1:-1]]
    if not cols or cols[0] in {"command", "---"}:
        continue
    if len(cols) < 5:
        continue
    cmd, st, tested, runtime, entry = cols[:5]
    out.append({
        "command": cmd,
        "status": st,
        "tested": tested,
        "runtime": runtime,
        "entry": entry,
        "manpage": f"bin/{cmd}/{cmd}.1",
    })
Path("target/reports/vitteos_bin_matrix.json").write_text(json.dumps(out, indent=2) + "\n")
PY

echo "wrote $OUT_MD"
echo "wrote $OUT_JSON"
