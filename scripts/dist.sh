#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DIST_DIR="$ROOT/dist"

mkdir -p "$DIST_DIR"
VERSION="0.1.0"
TAR_NAME="vittec-${VERSION}-darwin-arm64.tar.gz"

tar -C "$ROOT/target/release" -czf "$DIST_DIR/$TAR_NAME" vittec
shasum -a 256 "$DIST_DIR/$TAR_NAME" > "$DIST_DIR/$TAR_NAME.sha256"

echo "Dist created: $DIST_DIR/$TAR_NAME"
