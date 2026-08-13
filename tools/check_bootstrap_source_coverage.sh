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

grep -Eq '^const[[:space:]]+VERSION_TEXT:[[:space:]]*string[[:space:]]*=' "$COMPILER_SRC" || {
  printf '[bootstrap-source-coverage][error] compiler entry lacks named VERSION_TEXT string constant\n' >&2
  exit 1
}

grep -Eq '^const[[:space:]]+BANNER_TEXT:[[:space:]]*string[[:space:]]*=' "$COMPILER_SRC" || {
  printf '[bootstrap-source-coverage][error] compiler entry lacks named BANNER_TEXT string constant\n' >&2
  exit 1
}

grep -Eq '^proc[[:space:]]+version_text\(\)[[:space:]]*->[[:space:]]*string[[:space:]]*[{]' "$COMPILER_SRC" || {
  printf '[bootstrap-source-coverage][error] compiler entry lacks version_text() -> string\n' >&2
  exit 1
}

grep -Eq '^[[:space:]]*give[[:space:]]+VERSION_TEXT[[:space:]]*;?[[:space:]]*$' "$COMPILER_SRC" || {
  printf '[bootstrap-source-coverage][error] version_text() does not return named VERSION_TEXT constant\n' >&2
  exit 1
}

printf '[bootstrap-source-coverage] OK compiler_entry=src/vitte/compiler/main.vit\n'
