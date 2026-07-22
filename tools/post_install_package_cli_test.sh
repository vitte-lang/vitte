#!/usr/bin/env sh
set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
VITTE="$ROOT/bin/vitte"
python3 "$ROOT/tools/package_cli_integration_test.py" --vitte "$VITTE"
printf '[post-install-package-cli] ok: full offline lifecycle\n'
