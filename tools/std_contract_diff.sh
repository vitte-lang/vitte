#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"
python3 "$ROOT_DIR/tools/lint_std_compat_contracts.py"
