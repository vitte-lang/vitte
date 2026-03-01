#!/usr/bin/env bash
set -euo pipefail
ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"; BIN="${BIN:-$ROOT_DIR/bin/vitte}"; SRC="${SRC:-$ROOT_DIR/src/vitte/packages/json/mod.vit}"; OUT="${OUT:-$ROOT_DIR/target/reports/json_analyze.json}"
mkdir -p "$(dirname "$OUT")"
if [ ! -x "$BIN" ]; then echo "[analyze-json-json] warning: missing $BIN, skipping"; exit 0; fi
if "$BIN" analyze --help >/dev/null 2>&1 && "$BIN" analyze --json "$SRC" > "$OUT" 2>/tmp/vitte-json-analyze.err; then python3 "$ROOT_DIR/tools/enrich_json_analyze_json.py" >/dev/null 2>&1 || true; echo "[analyze-json-json] wrote $OUT via analyze --json"; exit 0; fi
if "$BIN" check --help 2>/dev/null | grep -q -- '--dump-module-index' && "$BIN" check --lang=en --dump-module-index "$SRC" > "$OUT" 2>/tmp/vitte-json-analyze.err; then python3 "$ROOT_DIR/tools/enrich_json_analyze_json.py" >/dev/null 2>&1 || true; echo "[analyze-json-json] wrote $OUT via check fallback"; exit 0; fi
printf '{"status":"skipped","reason":"analyze/check json unavailable or backend failed for host toolchain"}\n' > "$OUT"; python3 "$ROOT_DIR/tools/enrich_json_analyze_json.py" >/dev/null 2>&1 || true; echo "[analyze-json-json] warning: produced fallback report at $OUT"
