#!/usr/bin/env sh
set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$ROOT_DIR"

count_files() {
  pattern=$1
  shift
  if [ "$#" -eq 0 ]; then
    printf '0'
    return
  fi
  rg --files "$@" -g "$pattern" | wc -l | tr -d ' '
}

count_lines() {
  pattern=$1
  shift
  if [ "$#" -eq 0 ]; then
    printf '0'
    return
  fi
  files=$(rg --files "$@" -g "$pattern" || true)
  if [ -z "$files" ]; then
    printf '0'
    return
  fi
  printf '%s\n' "$files" | xargs wc -l | tail -n 1 | awk '{print $1}'
}

existing_dirs() {
  for dir in "$@"; do
    if [ -d "$dir" ]; then
      printf '%s\n' "$dir"
    fi
  done
}

compiler_dirs="src/vitte/compiler src/vitte/packages/compiler/driver"
compiler_existing_dirs=$(existing_dirs src/vitte/compiler src/vitte/packages/compiler/driver | tr '\n' ' ')
expected_compiler_root="src/vitte/compiler"
expected_compiler_entry="src/vitte/compiler/main.vit"
seed_manifest="toolchain/seed/manifest.txt"
release_artifact="target/release/vitte"
stage1_artifact="target/stage1/vitte"
stage2_artifact="target/stage2/vitte"
audit_errors=0
runtime_bridge_dir="$ROOT_DIR/src/vitte/compiler/backends/runtime_c"

legacy_source_files=$(
  find "$ROOT_DIR" \
    -path "$ROOT_DIR/.git" -prune -o \
    -path "$ROOT_DIR/.pkgstage" -prune -o \
    -path "$ROOT_DIR/bin" -prune -o \
    -path "$ROOT_DIR/build" -prune -o \
    -path "$ROOT_DIR/target" -prune -o \
    -path "$ROOT_DIR/editors/tree-sitter" -prune -o \
    -path "$ROOT_DIR/vscode/VitteLangVsCode-main/.vscode-test" -prune -o \
    -path "$ROOT_DIR/vscode/VitteLangVsCode-main/node_modules" -prune -o \
    -path "$ROOT_DIR/vscode/VitteLangVsCode-main/vitte_out.cpp" -prune -o \
    -type f \( -name '*.'c -o -name '*.'cc -o -name '*.'c'pp' -o -name '*.'cxx -o -name '*.'h -o -name '*.'h'pp' -o -name '*.'hxx \) -print \
  | sort
)

runtime_bridge_files=$(printf '%s\n' "$legacy_source_files" | awk -v dir="$runtime_bridge_dir/" 'index($0, dir) == 1')
legacy_non_runtime_files=$(printf '%s\n' "$legacy_source_files" | awk -v dir="$runtime_bridge_dir/" 'index($0, dir) != 1')

runtime_bridge_count=$(printf '%s\n' "$runtime_bridge_files" | sed '/^$/d' | wc -l | tr -d ' ')
legacy_non_runtime_count=$(printf '%s\n' "$legacy_non_runtime_files" | sed '/^$/d' | wc -l | tr -d ' ')
legacy_source_count=$(printf '%s\n' "$legacy_source_files" | sed '/^$/d' | wc -l | tr -d ' ')
compiler_vitte_files=$(count_files '*.vit' $compiler_existing_dirs)
compiler_vitl_files=$(count_files '*.vitl' $compiler_existing_dirs)
compiler_vitl_list=$(rg --files $compiler_existing_dirs -g '*.vitl' | sort || true)

printf 'Self-hosting audit\n'
printf '==================\n'
printf 'Status: '
if [ "$legacy_source_count" -eq 0 ]; then
  printf 'workspace source is Vitte-only\n'
elif [ "$legacy_non_runtime_count" -eq 0 ]; then
  printf 'runtime bridge sources still remain\n'
else
  printf 'legacy host sources still remain\n'
fi

printf '\nLegacy source files: %s\n' "$legacy_source_count"
if [ "$legacy_source_count" -ne 0 ]; then
  printf '%s\n' "$legacy_source_files"
fi

if [ "$runtime_bridge_count" -ne 0 ]; then
  printf '\nRuntime bridge sources: %s\n' "$runtime_bridge_count"
  printf '%s\n' "$runtime_bridge_files"
fi

if [ "$legacy_non_runtime_count" -ne 0 ]; then
  printf '\nNon-runtime legacy host sources: %s\n' "$legacy_non_runtime_count"
  printf '%s\n' "$legacy_non_runtime_files"
fi

printf 'Vitte compiler surface: %s .vit / %s .vitl\n' \
  "$compiler_vitte_files" "$compiler_vitl_files"
if [ "$compiler_vitl_files" -ne 0 ]; then
  printf '%s\n' "$compiler_vitl_list" | sed 's/^/  library: /'
fi

printf '\nCompiler source contract:\n'
printf '  release_artifact=%s\n' "$release_artifact"
printf '  stage1_artifact=%s\n' "$stage1_artifact"
printf '  stage2_artifact=%s\n' "$stage2_artifact"
printf '  compiler_entry=%s\n' "$expected_compiler_entry"

if [ -e "$seed_manifest" ]; then
  printf '    [error] retired seed manifest still exists: %s\n' "$seed_manifest"
  audit_errors=1
fi

if [ -e toolchain/seed ]; then
  printf '    [error] retired seed root still exists: toolchain/seed\n'
  audit_errors=1
fi

if [ -e scripts/seed ]; then
  printf '    [error] retired seed scripts still exist: scripts/seed\n'
  audit_errors=1
fi

if [ -e bin/vittec0 ]; then
  printf '    [error] retired stage0 seed artifact still exists: bin/vittec0\n'
  audit_errors=1
fi

if [ ! -d "$expected_compiler_root" ]; then
  printf '\n[error] missing compiler source root: %s\n' "$expected_compiler_root"
  audit_errors=1
fi

if [ ! -f "$expected_compiler_entry" ]; then
  printf '\n[error] missing compiler entry point: %s\n' "$expected_compiler_entry"
  audit_errors=1
fi

if [ "$compiler_vitl_files" -eq 0 ]; then
  printf '\n[error] compiler library surface has no .vitl files under: %s\n' "$compiler_dirs"
  audit_errors=1
fi

if ! grep -F '"library_file": "src/<space>.vitl"' src/vitte/compiler/modules.vitte.json >/dev/null; then
  printf '\n[error] compiler module manifest no longer records the .vitl library-file contract\n'
  audit_errors=1
fi

if ! python3 tools/runtime_driver_provenance_gate.py; then
  printf '\n[error] runtime compiler does not execute the current Vitte driver entry\n'
  audit_errors=1
fi

if [ "$audit_errors" -ne 0 ]; then
  exit 1
fi

printf '\nSeed roots are retired and runtime provenance confirms compiler entry %s.\n' "$expected_compiler_entry"
