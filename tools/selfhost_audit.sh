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

compiler_dirs="src/vitte/compiler src/vitte/stdlib/compiler src/vitte/packages/compiler/driver"
expected_compiler_root="src/vitte/compiler"
expected_compiler_entry="src/vitte/compiler/main.vit"
stage_sources="toolchain/stage1/src/main.vit toolchain/stage2/src/main.vit"
audit_errors=0
runtime_bridge_dir="$ROOT_DIR/src/vitte/compiler/backends/runtime_c"
stage2_bridge_sidecar="$ROOT_DIR/target/bootstrap/stage2/vittec.bootstrap-bridge"

legacy_source_files=$(
  find "$ROOT_DIR" \
    -path "$ROOT_DIR/.git" -prune -o \
    -path "$ROOT_DIR/.pkgstage" -prune -o \
    -path "$ROOT_DIR/bin" -prune -o \
    -path "$ROOT_DIR/build" -prune -o \
    -path "$ROOT_DIR/target" -prune -o \
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
for stage_src in $stage_sources; do
  root_value=$(awk -F= '/^[[:space:]]*const[[:space:]]+COMPILER_SOURCE_ROOT[[:space:]]*:/ {gsub(/^[[:space:]]*"|"[[:space:]]*$/, "", $2); gsub(/^[[:space:]]+|[[:space:]]+$/, "", $2); print $2; exit}' "$stage_src")
  entry_value=$(awk -F= '/^[[:space:]]*const[[:space:]]+COMPILER_ENTRY_POINT[[:space:]]*:/ {gsub(/^[[:space:]]*"|"[[:space:]]*$/, "", $2); gsub(/^[[:space:]]+|[[:space:]]+$/, "", $2); print $2; exit}' "$stage_src")
  printf '  %s\n' "$stage_src"
  printf '    COMPILER_SOURCE_ROOT=%s\n' "${root_value:-<missing>}"
  printf '    COMPILER_ENTRY_POINT=%s\n' "${entry_value:-<missing>}"
  if [ "$root_value" != "$expected_compiler_root" ]; then
    printf '    [error] expected COMPILER_SOURCE_ROOT=%s\n' "$expected_compiler_root"
    audit_errors=1
  fi
  if [ "$entry_value" != "$expected_compiler_entry" ]; then
    printf '    [error] expected COMPILER_ENTRY_POINT=%s\n' "$expected_compiler_entry"
    audit_errors=1
  fi
done

if [ -f "$stage2_bridge_sidecar" ]; then
  bridge_src=$(awk -F= '/^src=/ { print $2; exit }' "$stage2_bridge_sidecar")
  printf '\nActive stage2 bridge artifact:\n'
  printf '  %s\n' "${stage2_bridge_sidecar#$ROOT_DIR/}"
  printf '  src=%s\n' "${bridge_src:-<unknown>}"
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

printf '\nStage2 entry is anchored to %s; next step is replacing bootstrap-compatible shell emission with the real compiler backend path.\n' "$expected_compiler_entry"
