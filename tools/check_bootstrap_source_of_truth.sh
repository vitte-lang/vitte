#!/usr/bin/env sh
set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
cd "$ROOT_DIR"

require_text() {
  file="$1"
  needle="$2"
  label="$3"
  if ! grep -F "$needle" "$file" >/dev/null 2>&1; then
    printf '[bootstrap-source-of-truth][error] %s missing %s\n' "$file" "$label" >&2
    exit 1
  fi
}

[ -f src/vitte/compiler/main.vit ] || {
  printf '[bootstrap-source-of-truth][error] missing canonical compiler entry: src/vitte/compiler/main.vit\n' >&2
  exit 1
}

[ -f src/vitte/compiler/entrypoint.vitte.json ] || {
  printf '[bootstrap-source-of-truth][error] missing compiler entrypoint manifest: src/vitte/compiler/entrypoint.vitte.json\n' >&2
  exit 1
}

tools/check_compiler_entry_lock.sh
python3 tools/check_compiler_entrypoint.py
tools/check_bootstrap_source_coverage.sh
python3 tools/bootstrap_src_alignment_gate.py

require_text toolchain/README.md 'The compiler source of truth is `src/vitte/compiler`.' 'compiler source-of-truth statement'
require_text toolchain/INDEX.md 'Canonical compiler source: `src/vitte/compiler/main.vit`' 'canonical compiler entry statement'

printf '[bootstrap-source-of-truth] ok compiler_entry=src/vitte/compiler/main.vit\n'
