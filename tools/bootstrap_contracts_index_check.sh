#!/usr/bin/env sh
set -eu

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
DOC="$ROOT_DIR/docs/bootstrap_contracts.md"
MAKEFILE="$ROOT_DIR/Makefile"

die() {
    printf "[bootstrap-contracts-index-check][error] %s\n" "$1" >&2
    exit 1
}

[ -f "$DOC" ] || die "missing docs/bootstrap_contracts.md"
[ -f "$MAKEFILE" ] || die "missing Makefile"

tokens="$(
    awk '
        {
            line = $0
            while (match(line, /`[^`]+`/)) {
                print substr(line, RSTART + 1, RLENGTH - 2)
                line = substr(line, RSTART + RLENGTH)
            }
        }
    ' "$DOC"
)"

printf "%s\n" "$tokens" | while IFS= read -r token; do
    case "$token" in
        docs/*|tests/*|tools/*|toolchain/*|scripts/*|README.md|Makefile)
            [ -e "$ROOT_DIR/$token" ] || die "missing indexed path: $token"
            ;;
        "make "*)
            target=${token#make }
            awk -F: -v target="$target" '$1 == target { found = 1 } END { exit(found ? 0 : 1) }' "$MAKEFILE" \
                || die "missing indexed make target: $target"
            ;;
    esac
done

printf "[bootstrap-contracts-index-check] OK\n"
