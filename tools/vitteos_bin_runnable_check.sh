#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "${ROOT_DIR}"

BIN="bin/vitte"

if [[ ! -x "${BIN}" ]]; then
  echo "bin runnable check: FAIL (${BIN} not found or not executable)" >&2
  exit 1
fi

if "${BIN}" --help >/dev/null 2>&1; then
  echo "bin runnable check: OK"
  exit 0
fi

bin_kind="unknown"
if command -v file >/dev/null 2>&1; then
  bin_kind="$(file -b "${BIN}" 2>/dev/null || echo unknown)"
fi

echo "bin runnable check: FAIL (${BIN} is not runnable on this host; kind: ${bin_kind})" >&2
exit 1
