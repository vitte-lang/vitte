#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "${ROOT_DIR}"

required_paths=(
  "vitteos/boot/mod.vit"
  "vitteos/debug/serial"
  "vitteos/core/shutdown"
  "vitteos/test/kernel_smoke.vit"
)

for p in "${required_paths[@]}"; do
  if [[ ! -e "${p}" ]]; then
    echo "kernel smoke prerequisite missing: ${p}" >&2
    exit 1
  fi
done

if ! grep -q '^entry main at vitteos/test/kernel_smoke' vitteos/test/kernel_smoke.vit; then
  echo "kernel smoke entry is missing" >&2
  exit 1
fi

if [[ -x "bin/vitte" ]] && bin/vitte --help >/dev/null 2>&1; then
  bin/vitte check vitteos/test/kernel_smoke.vit
else
  echo "bin/vitte unavailable on this host: syntax probe skipped"
fi

echo "kernel smoke contract: OK"
