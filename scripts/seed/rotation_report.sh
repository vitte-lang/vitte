#!/usr/bin/env sh
set -eu

ROOT_DIR="$(cd "$(dirname "$0")/../.." && pwd)"
MANIFEST="$ROOT_DIR/toolchain/seed/manifest.txt"

die() {
    printf "[seed-rotation-report][error] %s\n" "$1" >&2
    exit 1
}

[ -f "$MANIFEST" ] || die "missing manifest"

source_file="$(awk -F= '/^source_file=/{print $2}' "$MANIFEST")"
seed_file="$(awk -F= '/^seed_file=/{print $2}' "$MANIFEST")"
manifest_sha="$(awk -F= '/^sha256=/{print $2}' "$MANIFEST")"
manifest_version="$(awk -F= '/^version=/{sub(/^version=/, ""); print}' "$MANIFEST")"

[ -n "$source_file" ] && [ -n "$seed_file" ] && [ -n "$manifest_sha" ] && [ -n "$manifest_version" ] ||
    die "invalid manifest"

source_path="$ROOT_DIR/$source_file"
seed_path="$ROOT_DIR/$seed_file"
[ -f "$source_path" ] || die "missing source: $source_file"
[ -f "$seed_path" ] || die "missing seed: $seed_file"

actual_sha="$(LC_ALL=C shasum -a 256 "$seed_path" | awk '{print $1}')"
actual_version="$("$seed_path" --version 2>/dev/null || true)"

printf "[seed-rotation-report] source=%s\n" "$source_file"
printf "[seed-rotation-report] artifact=%s\n" "$seed_file"
printf "[seed-rotation-report] manifest_sha=%s\n" "$manifest_sha"
printf "[seed-rotation-report] actual_sha=%s\n" "$actual_sha"
printf "[seed-rotation-report] manifest_version=%s\n" "$manifest_version"
printf "[seed-rotation-report] actual_version=%s\n" "$actual_version"

if [ "$manifest_sha" = "$actual_sha" ] && [ "$manifest_version" = "$actual_version" ]; then
    printf "[seed-rotation-report] status=clean\n"
else
    printf "[seed-rotation-report] status=rotation-needed\n"
    printf "[seed-rotation-report] update: make seed-manifest-update\n"
fi

printf "[seed-rotation-report] verify: make seed-verify bootstrap-native-snapshots bootstrap-verify seed-contract-check\n"
