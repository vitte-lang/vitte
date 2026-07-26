#!/usr/bin/env sh
set -eu

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
FROZEN_GATE="tools/check_seed_frozen.py"

die() {
    printf "[seed-contract][error] %s\n" "$1" >&2
    exit 1
}

if [ ! -f "$ROOT_DIR/$FROZEN_GATE" ]; then
    die "missing frozen seed gate: $FROZEN_GATE"
fi

python3 "$ROOT_DIR/$FROZEN_GATE"

printf "[seed-contract] ok\n"
