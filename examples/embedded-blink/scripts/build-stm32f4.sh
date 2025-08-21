#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../boards/stm32f4"
make
echo "blink.bin prêt (flash avec OpenOCD selon ta carte)"
