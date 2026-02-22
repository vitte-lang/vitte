#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "${ROOT_DIR}"

MANIFEST="vitteos/domains.json"
if [[ ! -f "${MANIFEST}" ]]; then
  echo "missing manifest: ${MANIFEST}" >&2
  exit 1
fi

mapfile -t domains < <(python3 -c 'import json,sys; d=json.load(open(sys.argv[1])); [print(x) for x in d["domains"]]' "${MANIFEST}")
mapfile -t required < <(python3 -c 'import json,sys; d=json.load(open(sys.argv[1])); [print(x) for x in d["required_files"]]' "${MANIFEST}")

files=()
for domain in "${domains[@]}"; do
  for f in "${required[@]}"; do
    files+=("vitteos/${domain}/${f}")
  done
done
files+=("vitteos/scripts/vitteos_tooling.vit")
files+=("vitteos/test/kernel_smoke.vit")

failed=0
for f in "${files[@]}"; do
  if [[ ! -f "${f}" ]]; then
    echo "missing file for header lint: ${f}" >&2
    failed=1
    continue
  fi
  if ! grep -q '<<<' "${f}" || ! grep -q '>>>' "${f}"; then
    echo "missing Vitte header block (<<< >>>): ${f}" >&2
    failed=1
  fi
done

if [[ ${failed} -ne 0 ]]; then
  exit 1
fi

echo "vitteos .vit header lint: OK"
