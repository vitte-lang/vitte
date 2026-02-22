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

failed=0
warn=0

# 1) mod.vit must remain wiring-only (no business declarations).
for d in "${domains[@]}"; do
  f="vitteos/${d}/mod.vit"
  [[ -f "${f}" ]] || continue
  if rg -n '^(proc|form|pick)\b' "${f}" >/dev/null 2>&1; then
    echo "mod.vit must be wiring-only (no proc/form/pick): ${f}" >&2
    failed=1
  fi
done

# 2) boot -> arch must go through arch/api only.
if compgen -G "vitteos/boot/*.vit" >/dev/null; then
  if rg -n 'arch/(impl|types|consts|tests|doc|mod)\b' vitteos/boot/*.vit >/dev/null 2>&1; then
    echo "boot must not depend on arch internals; use arch/api only" >&2
    rg -n 'arch/(impl|types|consts|tests|doc|mod)\b' vitteos/boot/*.vit >&2 || true
    failed=1
  fi
fi

# 3) mm and sched must not depend directly on drivers.
if compgen -G "vitteos/mm/*.vit" >/dev/null; then
  if rg -n 'drivers/' vitteos/mm/*.vit >/dev/null 2>&1; then
    echo "mm must not depend directly on drivers/*; use abstract interfaces" >&2
    rg -n 'drivers/' vitteos/mm/*.vit >&2 || true
    failed=1
  fi
fi
if compgen -G "vitteos/sched/*.vit" >/dev/null; then
  if rg -n 'drivers/' vitteos/sched/*.vit >/dev/null 2>&1; then
    echo "sched must not depend directly on drivers/*; use abstract interfaces" >&2
    rg -n 'drivers/' vitteos/sched/*.vit >&2 || true
    failed=1
  fi
fi

# 4) API should avoid naked primitive public signatures.
primitive='(int|bool|string|float|double|i32|i64|u32|u64)'
for d in "${domains[@]}"; do
  f="vitteos/${d}/api.vit"
  [[ -f "${f}" ]] || continue
  if rg -n "^proc .*\\(([^)]*:[[:space:]]*${primitive}([^A-Za-z0-9_]|$))" "${f}" >/dev/null 2>&1; then
    echo "api uses naked primitive parameter type; prefer explicit domain types: ${f}" >&2
    rg -n "^proc .*\\(([^)]*:[[:space:]]*${primitive}([^A-Za-z0-9_]|$))" "${f}" >&2 || true
    failed=1
  fi
  if rg -n "^proc .*->[[:space:]]*${primitive}([^A-Za-z0-9_]|$)" "${f}" >/dev/null 2>&1; then
    echo "api uses naked primitive return type; prefer explicit domain types: ${f}" >&2
    rg -n "^proc .*->[[:space:]]*${primitive}([^A-Za-z0-9_]|$)" "${f}" >&2 || true
    failed=1
  fi
done

# 5) doc.vit contract section guidance (soft while legacy placeholders remain).
for d in "${domains[@]}"; do
  f="vitteos/${d}/doc.vit"
  [[ -f "${f}" ]] || continue
  if rg -q '<<< auto generated >>>' "${f}"; then
    warn=$((warn + 1))
    continue
  fi
  if ! rg -qi 'contract' "${f}"; then
    echo "doc should include a contract section (role/deps/invariants/errors): ${f}" >&2
    failed=1
  fi
done

if [[ ${warn} -gt 0 ]]; then
  echo "vitteos arch contract lint: skipped ${warn} legacy doc placeholders (doc contract section)"
fi

if [[ ${failed} -ne 0 ]]; then
  exit 1
fi

echo "vitteos arch contract lint: OK"
