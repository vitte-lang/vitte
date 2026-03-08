#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"
LOG_FILE="${1:-$ROOT_DIR/target/reports/ci-completions/ci-completions.log}"
DIFF_MAX_LINES="${DIFF_MAX_LINES:-200}"

cd "$ROOT_DIR"

[ -f "$LOG_FILE" ] && tail -n 220 "$LOG_FILE" || true
python3 tools/generate_completions.py --print-mode || true
python3 tools/generate_completions.py --check --verbose --mode "${VITTE_COMPLETIONS_MODE:-auto}" || true

diff -u tools/completions/snapshots/bash.vitte.golden completions/bash/vitte | head -n "$DIFF_MAX_LINES" || true
diff -u tools/completions/snapshots/zsh._vitte.golden completions/zsh/_vitte | head -n "$DIFF_MAX_LINES" || true
diff -u tools/completions/snapshots/fish.vitte.fish.golden completions/fish/vitte.fish | head -n "$DIFF_MAX_LINES" || true

FIRST_ERR="$(grep -hoE '\[completions\].*' "$LOG_FILE" 2>/dev/null | head -n1 || true)"
if [ -n "$FIRST_ERR" ]; then
  echo "::error title=ci-completions mismatch::${FIRST_ERR}"
fi
