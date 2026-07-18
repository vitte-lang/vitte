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

compiler_dirs="src/vitte/compiler src/vitte/packages/compiler/driver"
expected_compiler_root="src/vitte/compiler"
expected_compiler_entry="src/vitte/compiler/main.vit"
seed_manifest="toolchain/seed/manifest.txt"
seed_artifact="toolchain/seed/vittec0.seed"
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
compiler_vitte_files=$(count_files '*.vit' $compiler_dirs)
compiler_vitl_files=$(count_files '*.vitl' $compiler_dirs)

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

printf '\nCompiler source contract:\n'
printf '  trust_root=%s\n' "$seed_artifact"
printf '  compiler_entry=%s\n' "$expected_compiler_entry"
if [ ! -f "$seed_manifest" ]; then
  printf '    [error] missing seed manifest: %s\n' "$seed_manifest"
  audit_errors=1
elif ! grep -F 'seed_file=toolchain/seed/vittec0.seed' "$seed_manifest" >/dev/null; then
  printf '    [error] seed manifest does not point at %s\n' "$seed_artifact"
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

if [ "$audit_errors" -ne 0 ]; then
  exit 1
fi

printf '\nSeed trust root is anchored to %s; compiler entry is %s.\n' "$seed_artifact" "$expected_compiler_entry"
