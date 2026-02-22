#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "${ROOT_DIR}"

MANIFEST="vitteos/domains.json"
SNAPSHOT="vitteos/test/snapshots/vitteos_vit_targeted_check.snapshot"
UPDATE=0

if [[ $# -gt 0 ]]; then
  if [[ $# -eq 1 && "$1" == "--update" ]]; then
    UPDATE=1
  else
    echo "usage: $0 [--update]" >&2
    exit 2
  fi
fi

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
mapfile -t files < <(printf '%s\n' "${files[@]}" | sort -u)

failed=0
report=()

for f in "${files[@]}"; do
  if [[ ! -s "${f}" ]]; then
    echo "missing or empty: ${f}" >&2
    failed=1
    continue
  fi
  report+=("OK file ${f}")
done

for domain in "${domains[@]}"; do
  for stem in "${required[@]}"; do
    f="vitteos/${domain}/${stem}"
    if ! grep -q '^space ' "${f}"; then
      echo "missing 'space' declaration: ${f}" >&2
      failed=1
    else
      report+=("OK space ${f}")
    fi
  done
done

if ! grep -q '^entry main at vitteos/scripts' vitteos/scripts/vitteos_tooling.vit; then
  echo "missing vitteos tooling orchestrator entry" >&2
  failed=1
fi

if ! grep -q '^entry main at vitteos/test/kernel_smoke' vitteos/test/kernel_smoke.vit; then
  echo "missing kernel smoke entry" >&2
  failed=1
fi

if ! grep -q 'smoke_boot' vitteos/test/kernel_smoke.vit; then
  echo "missing smoke_boot step in kernel smoke" >&2
  failed=1
fi

if ! grep -q 'smoke_serial_log' vitteos/test/kernel_smoke.vit; then
  echo "missing smoke_serial_log step in kernel smoke" >&2
  failed=1
fi

if ! grep -q 'smoke_halt' vitteos/test/kernel_smoke.vit; then
  echo "missing smoke_halt step in kernel smoke" >&2
  failed=1
fi

if [[ ${failed} -eq 0 ]]; then
  report+=("OK entry vitteos/scripts/vitteos_tooling.vit")
  report+=("OK entry vitteos/test/kernel_smoke.vit")
  report+=("OK smoke smoke_boot")
  report+=("OK smoke smoke_serial_log")
  report+=("OK smoke smoke_halt")
fi

if [[ ${failed} -ne 0 ]]; then
  exit 1
fi

tmp_report="$(mktemp)"
printf '%s\n' "${report[@]}" > "${tmp_report}"

if [[ ${UPDATE} -eq 1 ]]; then
  mkdir -p "$(dirname "${SNAPSHOT}")"
  cp "${tmp_report}" "${SNAPSHOT}"
  echo "updated snapshot: ${SNAPSHOT}"
else
  if [[ ! -f "${SNAPSHOT}" ]]; then
    echo "missing snapshot: ${SNAPSHOT} (run: tools/vitteos_vit_targeted_check.sh --update)" >&2
    rm -f "${tmp_report}"
    exit 1
  fi
  if ! diff -u "${SNAPSHOT}" "${tmp_report}" >/dev/null; then
    echo "targeted check snapshot mismatch: ${SNAPSHOT}" >&2
    echo "run: tools/vitteos_vit_targeted_check.sh --update" >&2
    rm -f "${tmp_report}"
    exit 1
  fi
fi

rm -f "${tmp_report}"
echo "vitteos targeted .vit checks: OK"
