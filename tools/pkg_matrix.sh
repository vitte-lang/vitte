#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
BIN="${BIN:-$ROOT_DIR/bin/vitte}"
REPORT_DIR="$ROOT_DIR/target/reports"
LOG="$REPORT_DIR/pkg_matrix.log"

mkdir -p "$REPORT_DIR"
: >"$LOG"

run_ok() {
  local label="$1"
  shift
  echo "[pkg-matrix] ok: $label" | tee -a "$LOG"
  "$@" | tee -a "$LOG"
}

run_expect_diag() {
  local label="$1"
  local code="$2"
  local src="$3"
  local out

  echo "[pkg-matrix] expect-diag: $label ($code)" | tee -a "$LOG"
  set +e
  out="$("$BIN" check "$src" 2>&1)"
  rc=$?
  set -e
  printf '%s\n' "$out" >>"$LOG"

  if [ "$rc" -eq 0 ]; then
    echo "[pkg-matrix][error] expected failure for $src" | tee -a "$LOG" >&2
    exit 1
  fi
  if ! printf '%s\n' "$out" | grep -Fq "$code"; then
    echo "[pkg-matrix][error] missing diagnostic $code for $src" | tee -a "$LOG" >&2
    exit 1
  fi
}

run_ok "packages-check-all" "$ROOT_DIR/tools/package_check_all.sh"
run_ok "package-manager-contract" python3 "$ROOT_DIR/tools/package_manager/run_checks.py"
run_ok "package-meta-surface" python3 - "$ROOT_DIR" <<'PY'
from pathlib import Path
import sys

root = Path(sys.argv[1])
mods = sorted(root.glob("src/vitte/packages/**/mod.vit"))
if not mods:
    print("[pkg-matrix][error] no package mod.vit files found", file=sys.stderr)
    raise SystemExit(1)

for mod in mods:
    text = mod.read_text(encoding="utf-8", errors="replace")
    rel = mod.relative_to(root)
    if "proc ready() -> bool" not in text:
        print(f"[pkg-matrix][error] missing ready() in {rel}", file=sys.stderr)
        raise SystemExit(1)
    if "proc package_meta() -> string" not in text:
        print(f"[pkg-matrix][error] missing package_meta() in {rel}", file=sys.stderr)
        raise SystemExit(1)
    print(f"[pkg-matrix] package contract ok: {rel}")
PY
run_ok "internal-dependencies" "$BIN" check "$ROOT_DIR/tests/pkg/ok_internal_dependencies.vit"
run_ok "cross-package-imports" "$BIN" check "$ROOT_DIR/tests/pkg/ok_cross_package_imports.vit"
run_ok "ast-interning-smoke" "$BIN" check "$ROOT_DIR/tests/pkg/ast_interning_smoke.vit"
run_ok "compiler-ast-interning-flow" "$BIN" check "$ROOT_DIR/tests/pkg/compiler_ast_interning_flow.vit"
run_expect_diag "missing-package" "MOD_E_PACKAGE_MISSING" "$ROOT_DIR/tests/pkg/bad_missing_package.vit"
run_expect_diag "import-cycle" "MOD_E_IMPORT_CYCLE" "$ROOT_DIR/tests/pkg/bad_import_cycle.vit"
run_expect_diag "symbol-not-exported" "MOD_E_SYMBOL_NOT_EXPORTED" "$ROOT_DIR/tests/pkg/bad_symbol_not_exported.vit"

echo "[pkg-matrix] PASS" | tee -a "$LOG"
