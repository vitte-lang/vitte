#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../boards/esp32"
idf.py build
