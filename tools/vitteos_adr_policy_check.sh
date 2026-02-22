#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "${ROOT_DIR}"

if ! git rev-parse --is-inside-work-tree >/dev/null 2>&1; then
  echo "adr policy check: skipped (not a git work tree)"
  exit 0
fi

if ! git rev-parse --verify HEAD~1 >/dev/null 2>&1; then
  echo "adr policy check: skipped (no commit range HEAD~1..HEAD)"
  exit 0
fi

range="${ADR_POLICY_RANGE:-HEAD~1..HEAD}"
changed="$(git diff --name-only "${range}" -- 'vitteos/boot/**' 'vitteos/mm/**' 'vitteos/sched/**' || true)"

if [[ -z "${changed}" ]]; then
  echo "adr policy check: no major boot/mm/sched changes in ${range}"
  exit 0
fi

messages="$(git log --format=%B ${range})"
if echo "${messages}" | grep -Eiq 'ADR[- ]?[0-9]{4}'; then
  echo "adr policy check: OK (ADR reference found)"
  exit 0
fi

echo "adr policy violation: changes in vitteos/boot|mm|sched require ADR reference in commit message (e.g. ADR-0001)" >&2
exit 1
