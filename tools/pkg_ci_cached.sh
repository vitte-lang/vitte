#!/usr/bin/env bash
set -euo pipefail
if [ $# -lt 2 ]; then
  echo "usage: $0 <pkg> <make-target>" >&2
  exit 1
fi
PKG="$1"
TARGET="$2"
ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"
CACHE_DIR="$ROOT_DIR/target/cache/ci/$PKG"
mkdir -p "$CACHE_DIR"
HASH_FILE="$CACHE_DIR/input.sha256"

collect_hash() {
  {
    find "$ROOT_DIR/src/vitte/packages/$PKG" -type f 2>/dev/null | sort | xargs -r cat
    find "$ROOT_DIR/tests/modules/contracts/$PKG" -type f 2>/dev/null | sort | xargs -r cat
    find "$ROOT_DIR/tools" -maxdepth 1 -type f -name "lint_${PKG}_*" 2>/dev/null | sort | xargs -r cat
    [ -f "$ROOT_DIR/tools/facade_packages.json" ] && cat "$ROOT_DIR/tools/facade_packages.json"
  } | sha256sum | awk '{print $1}'
}

H="$(collect_hash)"
if [ -f "$HASH_FILE" ] && [ "$(cat "$HASH_FILE")" = "$H" ]; then
  echo "[pkg-ci-cached] $PKG cache hit -> skip $TARGET"
  exit 0
fi

echo "[pkg-ci-cached] $PKG cache miss -> run $TARGET"
make -C "$ROOT_DIR" -s "$TARGET"
echo "$H" > "$HASH_FILE"
echo "[pkg-ci-cached] $PKG cache updated"
