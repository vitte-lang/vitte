#!/usr/bin/env sh
set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)

cd "$ROOT_DIR"

python3 tools/check_bootstrap_seed_root.py --artifacts
python3 tools/bootstrap_seed_root_test.py
tools/check_seed_contract.sh

printf '[seed-verify] OK\n'
