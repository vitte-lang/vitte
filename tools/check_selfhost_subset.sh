#!/usr/bin/env sh
set -eu

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"

TARGETS="
toolchain/seed/src/main.vit
src/vitte/compiler/main.vit
"

die() {
  printf "[selfhost-subset][error] %s\n" "$1" >&2
  exit 1
}

require_file() {
  [ -f "$1" ] || die "missing file: $1"
}

require_grep() {
  file="$1"
  pattern="$2"
  message="$3"
  grep -Eq "$pattern" "$file" || die "$message ($file)"
}

forbid_grep() {
  file="$1"
  pattern="$2"
  message="$3"
  if grep -Eq "$pattern" "$file"; then
    die "$message ($file)"
  fi
}

check_top_level_surface() {
  file="$1"
  require_grep "$file" '^space[[:space:]]+[A-Za-z0-9_./-]+' \
    "self-host subset requires a space declaration"
  require_grep "$file" '^proc[[:space:]]+main\(args:[[:space:]]*list\[string\]\)[[:space:]]*->[[:space:]]*int[[:space:]]*[{]' \
    "self-host subset requires proc main(args: list[string]) -> int"
  require_grep "$file" '^export \*$' \
    "self-host subset requires export *"
  forbid_grep "$file" '^[[:space:]]*(trait|impl|class|union|bits|flags|macro|query|compiler|pass|backend|diagnostic|test|bench)\b' \
    "forbidden top-level syntax in self-host subset"
}

check_entry_contract() {
  file="$1"
  require_grep "$file" '^const[[:space:]]+VERSION_TEXT[[:space:]]*:[[:space:]]*string[[:space:]]*=' \
    "self-host subset requires VERSION_TEXT"
  require_grep "$file" '^const[[:space:]]+BANNER_TEXT[[:space:]]*:[[:space:]]*string[[:space:]]*=' \
    "self-host subset requires BANNER_TEXT"
  require_grep "$file" '^proc[[:space:]]+version_text\(\)[[:space:]]*->[[:space:]]*string[[:space:]]*[{]' \
    "self-host subset requires version_text()"
  require_grep "$file" '^proc[[:space:]]+banner_text\(\)[[:space:]]*->[[:space:]]*string[[:space:]]*[{]' \
    "self-host subset requires banner_text()"
}

check_const_surface() {
  file="$1"
  require_grep "$file" '^const[[:space:]]+[A-Z_][A-Z0-9_]*[[:space:]]*:[[:space:]]*string[[:space:]]*=' \
    "self-host subset requires at least one named string constant"
}

for rel in $TARGETS; do
  file="$ROOT_DIR/$rel"
  require_file "$file"
  check_top_level_surface "$file"
  check_entry_contract "$file"
  check_const_surface "$file"
done

printf "[selfhost-subset] OK\n"
