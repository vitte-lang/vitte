#!/usr/bin/env bash
set -euo pipefail
ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"; BIN="${BIN:-$ROOT_DIR/bin/vitte}"; SRC="${SRC:-$ROOT_DIR/src/vitte/packages/yaml/mod.vit}"; OUT="${OUT:-$ROOT_DIR/target/reports/yaml_analyze.json}"
mkdir -p "$(dirname "$OUT")"
if [ ! -x "$BIN" ]; then echo "[analyze-yaml-json] warning: missing $BIN, skipping"; exit 0; fi
if "$BIN" analyze --help >/dev/null 2>&1 && "$BIN" analyze --json "$SRC" > "$OUT" 2>/tmp/vitte-yaml-analyze.err; then python3 "$ROOT_DIR/tools/enrich_yaml_analyze_json.py" >/dev/null 2>&1 || true; echo "[analyze-yaml-json] wrote $OUT via analyze --json"; exit 0; fi
if "$BIN" check --help 2>/dev/null | grep -q -- '--dump-module-index' && "$BIN" check --lang=en --dump-module-index "$SRC" > "$OUT" 2>/tmp/vitte-yaml-analyze.err; then python3 "$ROOT_DIR/tools/enrich_yaml_analyze_json.py" >/dev/null 2>&1 || true; echo "[analyze-yaml-json] wrote $OUT via check fallback"; exit 0; fi
printf '{"status":"skipped","reason":"analyze/check json unavailable or backend failed for host toolchain"}\n' > "$OUT"; python3 "$ROOT_DIR/tools/enrich_yaml_analyze_json.py" >/dev/null 2>&1 || true; echo "[analyze-yaml-json] warning: produced fallback report at $OUT"
