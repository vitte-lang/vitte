#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"
BIN="${BIN:-$ROOT_DIR/bin/vitte}"
SRC="${SRC:-$ROOT_DIR/src/vitte/packages/core/mod.vit}"
OUT="${OUT:-$ROOT_DIR/target/reports/core_analyze.json}"

mkdir -p "$(dirname "$OUT")"

if [ ! -x "$BIN" ]; then
  printf '[analyze-core-json] warning: missing %s, skipping\n' "$BIN"
  exit 0
fi

if "$BIN" analyze --help >/dev/null 2>&1; then
  if "$BIN" analyze --json "$SRC" > "$OUT" 2>/tmp/vitte-core-analyze.err; then
    python3 "$ROOT_DIR/tools/enrich_core_analyze_json.py" >/dev/null 2>&1 || true
    printf '[analyze-core-json] wrote %s via analyze --json\n' "$OUT"
    exit 0
  fi
  printf '[analyze-core-json] warning: analyze --json failed, trying fallback\n'
fi

if "$BIN" check --help 2>/dev/null | grep -q -- '--dump-module-index'; then
  if "$BIN" check --lang=en --dump-module-index "$SRC" > "$OUT" 2>/tmp/vitte-core-analyze.err; then
    python3 "$ROOT_DIR/tools/enrich_core_analyze_json.py" >/dev/null 2>&1 || true
    printf '[analyze-core-json] wrote %s via check --dump-module-index fallback\n' "$OUT"
    exit 0
  fi
fi

printf '{"status":"skipped","reason":"analyze/check json unavailable or backend failed for host toolchain"}\n' > "$OUT"
python3 "$ROOT_DIR/tools/enrich_core_analyze_json.py" >/dev/null 2>&1 || true
printf '[analyze-core-json] warning: produced fallback report at %s\n' "$OUT"
exit 0
