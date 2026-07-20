#!/bin/sh
set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
TMP_ROOT=${TMPDIR:-/tmp}/vitte-scripts-build-checksum-$$

cleanup() {
  rm -rf "$TMP_ROOT"
}
trap cleanup EXIT HUP INT TERM

mkdir -p "$TMP_ROOT"
printf 'vitte checksum fallback\n' > "$TMP_ROOT/artifact.txt"

SCRIPT_NAME=scripts-build-checksum-test . "$ROOT_DIR/scripts_build/common.sh"
SCRIPTS_BUILD_SHA256_BACKEND=python scripts_build_sha256_write "$TMP_ROOT/artifact.txt" "$TMP_ROOT/artifact.txt.sha256"
SCRIPTS_BUILD_SHA256_BACKEND=python scripts_build_sha256_check "$TMP_ROOT/artifact.txt" "$TMP_ROOT/artifact.txt.sha256"

grep -Eq '^[0-9a-f]{64}  artifact\.txt$' "$TMP_ROOT/artifact.txt.sha256"
printf '[scripts-build-checksum-fallback] OK\n'
