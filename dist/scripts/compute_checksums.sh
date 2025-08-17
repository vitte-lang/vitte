#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
: > manifests/checksums.txt
for f in archives/*; do [ -f "$f" ] || continue; sha256sum "$f" >> manifests/checksums.txt; done
echo "[checksums] wrote manifests/checksums.txt"
