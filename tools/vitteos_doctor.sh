#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "${ROOT_DIR}"

check_cmd() {
  local label="$1"
  local cmd="$2"
  if command -v "${cmd}" >/dev/null 2>&1; then
    echo "tool:${label}=present (${cmd})"
  else
    echo "tool:${label}=missing (${cmd})"
  fi
}

echo "vitteos-doctor"
echo "root=${ROOT_DIR}"

if [[ -x "bin/vitte" ]] && bin/vitte --help >/dev/null 2>&1; then
  echo "bin/vitte=runnable"
else
  echo "bin/vitte=not-runnable-or-missing"
fi

check_cmd "python3" "python3"
check_cmd "node" "node"
check_cmd "qemu" "qemu-system-x86_64"
check_cmd "bochs" "bochs"

if [[ -f "vitteos/test/snapshots/vitteos_vit_targeted_check.snapshot" ]]; then
  echo "snapshot:vitteos_vit_targeted_check=present"
else
  echo "snapshot:vitteos_vit_targeted_check=missing"
fi

if [[ -f "vitteos/test/snapshots/kernel_smoke_runtime.log.snapshot" ]]; then
  echo "snapshot:kernel_smoke_runtime=present"
else
  echo "snapshot:kernel_smoke_runtime=missing"
fi

if [[ -f "vitteos/domains.json" ]]; then
  count="$(python3 -c 'import json; d=json.load(open("vitteos/domains.json")); print(len(d.get("domains", [])))')"
  echo "domains.count=${count}"
fi
