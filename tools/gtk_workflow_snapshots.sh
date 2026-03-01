#!/usr/bin/env bash
set -euo pipefail
ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"
OUT="$ROOT_DIR/tests/tui_snapshots/gtk_workflows_critical.must"
mkdir -p "$(dirname "$OUT")"

{
  echo "GTK Workflow Snapshots"
  echo "- problems_quickfix: tests/tui_snapshots/gtk_core_quickfix.must"
  echo "- doctors: target/reports/doctors_dashboard.txt"
  echo "- contracts_panel: target/reports/contracts_dashboard.md"
  echo "- release_doctor: target/reports/release_doctor.md"
  date -u +"generated_at=%Y-%m-%dT%H:%M:%SZ"
} > "$OUT"

echo "[gtk-workflow-snapshots] wrote $OUT"
