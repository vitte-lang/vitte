#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
MANIFEST="${ROOT_DIR}/vitteos/domains.json"
INDEX_FILE="${ROOT_DIR}/vitteos/modules.index"

if [[ ! -f "${MANIFEST}" ]]; then
  echo "missing manifest: vitteos/domains.json" >&2
  exit 1
fi

if [[ ! -f "${INDEX_FILE}" ]]; then
  echo "missing module index: vitteos/modules.index" >&2
  exit 1
fi

mapfile -t domains < <(python3 -c 'import json,sys; d=json.load(open(sys.argv[1])); [print(x) for x in d["domains"]]' "${MANIFEST}")
mapfile -t allowed_roots < <(python3 -c 'import json,sys; d=json.load(open(sys.argv[1])); [print(x) for x in d.get("non_domain_roots", [])]' "${MANIFEST}")

allowed_csv="$(printf '%s,' "${domains[@]}" "${allowed_roots[@]}")"

failed=0

while IFS= read -r module; do
  [[ -z "${module}" ]] && continue
  [[ "${module}" =~ ^# ]] && continue
  if [[ ! "${module}" =~ ^vitteos/ ]]; then
    echo "invalid modules.index entry (must start with vitteos/): ${module}" >&2
    failed=1
    continue
  fi
  root="${module#vitteos/}"
  root="${root%%/*}"
  if [[ ",${allowed_csv}," != *",${root},"* ]]; then
    echo "modules.index entry is outside allowed roots/domains: ${module}" >&2
    failed=1
  fi
done < "${INDEX_FILE}"

while IFS= read -r root; do
  if [[ ",${allowed_csv}," != *",${root},"* ]]; then
    echo "orphan .vit root outside declared domains: vitteos/${root}" >&2
    failed=1
  fi
done < <(
  find "${ROOT_DIR}/vitteos" -mindepth 1 -maxdepth 1 -type d -print0 |
  while IFS= read -r -d '' d; do
    if [[ -n "$(find "${d}" -maxdepth 1 -type f -name '*.vit' -print -quit)" ]]; then
      basename "${d}"
    fi
  done | sort -u
)

if [[ ${failed} -ne 0 ]]; then
  exit 1
fi

echo "vitteos orphan module/root check: OK"
