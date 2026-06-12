#!/usr/bin/env sh
set -eu

ROOT_DIR="$(cd "$(dirname "$0")/../.." && pwd)"
SOURCE_FILE="toolchain/seed/src/main.vit"
SEED_FILE="toolchain/seed/vittec0.seed"
MANIFEST="$ROOT_DIR/toolchain/seed/manifest.txt"

die() {
    printf "[seed-manifest-update][error] %s\n" "$1" >&2
    exit 1
}

source_path="$ROOT_DIR/$SOURCE_FILE"
seed_path="$ROOT_DIR/$SEED_FILE"

[ -f "$source_path" ] || die "missing seed source: $SOURCE_FILE"
[ -f "$seed_path" ] || die "missing seed artifact: $SEED_FILE"

version="$(
    awk '
        /^[[:space:]]*const[[:space:]]+VERSION_TEXT[[:space:]]*:[[:space:]]*string[[:space:]]*=/ {
            value = $0
            sub(/^.*=[[:space:]]*"/, "", value)
            sub(/"[[:space:]]*$/, "", value)
            print value
            exit
        }
    ' "$source_path"
)"
[ -n "$version" ] || die "missing VERSION_TEXT in $SOURCE_FILE"

sha="$(LC_ALL=C shasum -a 256 "$seed_path" | awk '{print $1}')"
tmp="$MANIFEST.tmp.$$"
trap 'rm -f "$tmp"' EXIT HUP INT TERM

{
    printf 'source_file=%s\n' "$SOURCE_FILE"
    printf 'seed_file=%s\n' "$SEED_FILE"
    printf 'sha256=%s\n' "$sha"
    printf 'version=%s\n' "$version"
} > "$tmp"

mv "$tmp" "$MANIFEST"
trap - EXIT HUP INT TERM

printf "[seed-manifest-update] %s %s\n" "$sha" "$SEED_FILE"
