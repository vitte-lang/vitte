#!/usr/bin/env sh
# smoke_standalone.sh - run each standalone fuzz harness against its curated seeds.
#
# Assumes binaries have been built under fuzz/out via fuzz/scripts/build_targets.sh

set -eu

OUT_DIR="${OUT_DIR:-fuzz/out}"
CORPUS_DIR="${CORPUS_DIR:-fuzz/targets/corpus}"

run_dir() {
  target="$1"
  bin="$OUT_DIR/$target"
  dir="$CORPUS_DIR/$target"
  if [ ! -x "$bin" ]; then
    echo "[skip] missing $bin"
    return 0
  fi
  if [ ! -d "$dir" ]; then
    echo "[skip] missing $dir"
    return 0
  fi
  for f in "$dir"/*; do
    [ -f "$f" ] || continue
    "$bin" "$f" >/dev/null 2>&1 || {
      echo "[fail] $target: $f"
      return 1
    }
  done
  echo "[ok] $target"
}

run_dir fuzz_lexer
run_dir fuzz_parser
run_dir fuzz_parser_recovery
run_dir fuzz_ast_invariants
run_dir fuzz_lowering
run_dir fuzz_ast_printer
run_dir fuzz_vitte_parser
run_dir fuzz_typecheck
run_dir fuzz_lockfile
run_dir fuzz_muf_parser
run_dir fuzz_vm_exec
run_dir fuzir_verify
run_dir fuzz_diag_json
run_dir fuze_vm_decode

