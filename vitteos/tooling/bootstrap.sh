#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"

echo "[bootstrap] project root: ${ROOT_DIR}"
echo "[bootstrap] build vitte compiler"
make -C "${ROOT_DIR}" build

echo "[bootstrap] check vitteos tooling script"
"${ROOT_DIR}/bin/vitte" check "${ROOT_DIR}/vitteos/scripts/vitteos_tooling.vit"

echo "[bootstrap] done"
