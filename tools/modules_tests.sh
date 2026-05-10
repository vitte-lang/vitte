#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"

# Run module parsing tests
"$ROOT_DIR/tools/parse_modules_tests.sh"

echo "[modules-tests] OK"
