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

legacy_source_files=$(
  find "$ROOT_DIR" \
    -path "$ROOT_DIR/.git" -prune -o \
    -path "$ROOT_DIR/bin" -prune -o \
    -path "$ROOT_DIR/build" -prune -o \
    -path "$ROOT_DIR/target" -prune -o \
    -type f \( -name '*.'c -o -name '*.'cc -o -name '*.'c'pp' -o -name '*.'cxx -o -name '*.'h -o -name '*.'h'pp' -o -name '*.'hxx \) -print \
  | sort
)

legacy_source_count=$(printf '%s\n' "$legacy_source_files" | sed '/^$/d' | wc -l | tr -d ' ')
compiler_vitte_files=$(count_files '*.vit' $compiler_dirs)
compiler_vitl_files=$(count_files '*.vitl' $compiler_dirs)

printf 'Self-hosting audit\n'
printf '==================\n'
printf 'Status: '
if [ "$legacy_source_count" -eq 0 ]; then
  printf 'workspace source is Vitte-only\n'
else
  printf 'legacy host sources still remain\n'
fi

printf '\nLegacy source files: %s\n' "$legacy_source_count"
if [ "$legacy_source_count" -ne 0 ]; then
  printf '%s\n' "$legacy_source_files"
fi

printf 'Vitte compiler surface: %s .vit / %s .vitl\n' \
  "$compiler_vitte_files" "$compiler_vitl_files"

printf '\nNext step: keep bootstrap checks green while expanding the Vitte compiler implementation.\n'
