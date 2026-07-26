#!/usr/bin/env sh
set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
COMPILER_SRC="$ROOT_DIR/src/vitte/compiler/main.vit"

[ -f "$COMPILER_SRC" ] || {
  printf '[bootstrap-source-coverage][error] missing compiler entry: src/vitte/compiler/main.vit\n' >&2
  exit 1
}

grep -Eq '^proc[[:space:]]+main\(args:[[:space:]]*list\[string\]\)[[:space:]]*->[[:space:]]*int[[:space:]]*[{]' "$COMPILER_SRC" || {
  printf '[bootstrap-source-coverage][error] compiler entry lacks main(args: list[string]) -> int\n' >&2
  exit 1
}

printf '[bootstrap-source-coverage] OK compiler_entry=src/vitte/compiler/main.vit\n'
