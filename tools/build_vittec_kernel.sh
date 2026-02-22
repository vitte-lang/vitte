#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUT_BIN="${ROOT_DIR}/target/kernel-tools/vittec-kernel"
CXX_BIN="${CXX:-g++}"

if [[ "${1:-}" == "--clean" ]]; then
  rm -f "${OUT_BIN}"
fi

make -C "${ROOT_DIR}" CXX="${CXX_BIN}" vittec-kernel

if [[ ! -x "${OUT_BIN}" ]]; then
  echo "error: missing output binary ${OUT_BIN}" >&2
  exit 1
fi

echo "built ${OUT_BIN}"
