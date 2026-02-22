#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
VITTEOS_DIR="${ROOT_DIR}/vitteos"
MANIFEST="${VITTEOS_DIR}/domains.json"

if [[ ! -f "${MANIFEST}" ]]; then
  echo "missing manifest: vitteos/domains.json" >&2
  exit 1
fi

mapfile -t domains < <(python3 -c 'import json,sys; d=json.load(open(sys.argv[1])); [print(x) for x in d["domains"]]' "${MANIFEST}")
mapfile -t required < <(python3 -c 'import json,sys; d=json.load(open(sys.argv[1])); [print(x) for x in d["required_files"]]' "${MANIFEST}")
mapfile -t owner_domains < <(python3 -c 'import json,sys; d=json.load(open(sys.argv[1])); domains=d.get("domains",[]); owners=d.get("owners",{}); [print(x) for x in domains if not (isinstance(owners.get(x), list) and any(isinstance(v,str) and v.strip() for v in owners.get(x,[])))]' "${MANIFEST}")

missing=0

if [[ ${#owner_domains[@]} -ne 0 ]]; then
  for d in "${owner_domains[@]}"; do
    echo "missing owners entry in vitteos/domains.json for domain: ${d}" >&2
  done
  missing=1
fi

for domain in "${domains[@]}"; do
  domain_dir="${VITTEOS_DIR}/${domain}"
  if [[ ! -d "${domain_dir}" ]]; then
    echo "missing domain directory: vitteos/${domain}" >&2
    missing=1
    continue
  fi

  for stem in "${required[@]}"; do
    f="${domain_dir}/${stem}"
    if [[ ! -f "${f}" ]]; then
      echo "missing file: vitteos/${domain}/${stem}" >&2
      missing=1
      continue
    fi
    if ! grep -q '^space ' "${f}"; then
      echo "invalid module header (missing 'space'): vitteos/${domain}/${stem}" >&2
      missing=1
    fi
  done
done

if [[ ${missing} -ne 0 ]]; then
  exit 1
fi

echo "vitteos domain contract: OK"
