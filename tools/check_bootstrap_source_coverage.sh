#!/usr/bin/env sh
set -eu

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
SEED_SRC="$ROOT_DIR/toolchain/seed/src/main.vit"
STAGE2_SRC="$ROOT_DIR/toolchain/stage2/src/main.vit"

die() {
    printf "[bootstrap-source-coverage][error] %s\n" "$1" >&2
    exit 1
}

require_grep() {
    file="$1"
    pattern="$2"
    label="$3"
    grep -Eq "$pattern" "$file" || die "$label"
}

[ -f "$SEED_SRC" ] || die "missing seed source: toolchain/seed/src/main.vit"
[ -f "$STAGE2_SRC" ] || die "missing stage2 source: toolchain/stage2/src/main.vit"

require_grep "$SEED_SRC" '^const[[:space:]]+[A-Z_][A-Z0-9_]*[[:space:]]*:[[:space:]]*string[[:space:]]*=' \
    "seed source must exercise named string constants"
require_grep "$SEED_SRC" '^const[[:space:]]+[A-Z_][A-Z0-9_]*[[:space:]]*:[[:space:]]*int[[:space:]]*=' \
    "seed source must exercise named int constants"
require_grep "$SEED_SRC" '^proc[[:space:]]+main\(args:[[:space:]]*list\[string\]\)[[:space:]]*->[[:space:]]*int[[:space:]]*[{]' \
    "seed source must exercise proc main(args: list[string]) -> int"
require_grep "$SEED_SRC" '^[[:space:]]*give[[:space:]]+[A-Z_][A-Z0-9_]*$' \
    "seed source main should exercise giving a named constant"

require_grep "$STAGE2_SRC" '^const[[:space:]]+VERSION_TEXT[[:space:]]*:[[:space:]]*string[[:space:]]*=' \
    "stage2 source must exercise named VERSION_TEXT string constant"
require_grep "$STAGE2_SRC" '^const[[:space:]]+BANNER_TEXT[[:space:]]*:[[:space:]]*string[[:space:]]*=' \
    "stage2 source must exercise named BANNER_TEXT string constant"
require_grep "$STAGE2_SRC" '^[[:space:]]*give[[:space:]]+VERSION_TEXT$' \
    "stage2 version_text should exercise named string constant give"
require_grep "$STAGE2_SRC" '^proc[[:space:]]+main\(args:[[:space:]]*list\[string\]\)[[:space:]]*->[[:space:]]*int[[:space:]]*[{]' \
    "stage2 source must exercise proc main(args: list[string]) -> int"

printf "[bootstrap-source-coverage] OK\n"
