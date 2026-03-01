#!/usr/bin/env bash
set -euo pipefail
python3 "$(cd "$(dirname "$0")/.." && pwd)/tools/lint_lint_compat_contracts.py"
