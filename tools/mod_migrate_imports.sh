#!/usr/bin/env bash
set -euo pipefail
ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"
exec "$ROOT_DIR/tools/modules_fix_all.sh" "$@"
