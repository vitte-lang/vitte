#!/usr/bin/env sh
set -eu
ROOT_DIR="$(cd "$(dirname "$0")/../.." && pwd)"
MANIFEST="$ROOT_DIR/toolchain/seed/manifest.txt"

[ -f "$MANIFEST" ] || { echo "[seed-verify][error] missing manifest" >&2; exit 1; }
source_file="$(awk -F= '/^source_file=/{print $2}' "$MANIFEST")"
seed_file="$(awk -F= '/^seed_file=/{print $2}' "$MANIFEST")"
expected_sha="$(awk -F= '/^sha256=/{print $2}' "$MANIFEST")"
expected_version="$(awk -F= '/^version=/{sub(/^version=/, ""); print}' "$MANIFEST")"

[ -n "$source_file" ] && [ -n "$seed_file" ] && [ -n "$expected_sha" ] && [ -n "$expected_version" ] || { echo "[seed-verify][error] invalid manifest" >&2; exit 1; }

source_path="$ROOT_DIR/$source_file"
[ -f "$source_path" ] || { echo "[seed-verify][error] missing seed source file: $source_path" >&2; exit 1; }
seed_path="$ROOT_DIR/$seed_file"
[ -f "$seed_path" ] || { echo "[seed-verify][error] missing seed file: $seed_path" >&2; exit 1; }

source_version="$(awk '
    /^[[:space:]]*const[[:space:]]+VERSION_TEXT[[:space:]]*:[[:space:]]*string[[:space:]]*=/ {
        value = $0
        sub(/^.*=[[:space:]]*"/, "", value)
        sub(/"[[:space:]]*$/, "", value)
        print value
        exit
    }
' "$source_path")"
[ -n "$source_version" ] || { echo "[seed-verify][error] missing VERSION_TEXT in seed source" >&2; exit 1; }
[ "$source_version" = "$expected_version" ] || { echo "[seed-verify][error] source VERSION_TEXT mismatch" >&2; echo "manifest=$expected_version" >&2; echo "source=$source_version" >&2; exit 1; }

actual_sha="$(shasum -a 256 "$seed_path" | awk '{print $1}')"
[ "$actual_sha" = "$expected_sha" ] || { echo "[seed-verify][error] checksum mismatch" >&2; echo "expected=$expected_sha" >&2; echo "actual=$actual_sha" >&2; exit 1; }

chmod +x "$seed_path"
actual_version="$($seed_path --version 2>/dev/null || true)"
[ "$actual_version" = "$expected_version" ] || { echo "[seed-verify][error] version mismatch" >&2; echo "expected=$expected_version" >&2; echo "actual=$actual_version" >&2; exit 1; }

echo "[seed-verify] ok: seed verified"
