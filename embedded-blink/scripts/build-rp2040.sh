#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../boards/rp2040"
cmake -S . -B build
cmake --build build
echo "UF2 prêt dans build/"
