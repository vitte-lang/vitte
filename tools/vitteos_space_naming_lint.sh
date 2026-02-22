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

failed=0

for domain in "${domains[@]}"; do
  for req in "${required[@]}"; do
    f="vitteos/${domain}/${req}"
    if [[ ! -f "${f}" ]]; then
      continue
    fi
    rel="${f#vitteos/}"
    expected="space ${rel%.vit}"
    actual="$(grep -m1 '^space ' "${f}" || true)"
    if [[ -z "${actual}" ]]; then
      echo "missing space declaration: ${f}" >&2
      failed=1
      continue
    fi
    if [[ "${actual}" != "${expected}" ]]; then
      echo "invalid space naming in ${f}" >&2
      echo "  expected: ${expected}" >&2
      echo "  actual:   ${actual}" >&2
      failed=1
    fi
  done
done

INDEX_FILE="vitteos/modules.index"
if [[ -f "${INDEX_FILE}" ]]; then
  while IFS= read -r module; do
    [[ -z "${module}" ]] && continue
    [[ "${module}" =~ ^# ]] && continue
    for req in "${required[@]}"; do
      f="${module}/${req}"
      if [[ ! -f "${f}" ]]; then
        continue
      fi
      rel="${f#vitteos/}"
      expected="space ${rel%.vit}"
      actual="$(grep -m1 '^space ' "${f}" || true)"
      if [[ -z "${actual}" ]]; then
        echo "missing space declaration: ${f}" >&2
        failed=1
        continue
      fi
      if [[ "${actual}" != "${expected}" ]]; then
        echo "invalid space naming in ${f}" >&2
        echo "  expected: ${expected}" >&2
        echo "  actual:   ${actual}" >&2
        failed=1
      fi
    done
  done < "${INDEX_FILE}"
fi

if [[ ${failed} -ne 0 ]]; then
  exit 1
fi

echo "vitteos space naming lint: OK"
