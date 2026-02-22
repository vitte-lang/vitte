#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "${ROOT_DIR}"

SNAPSHOT="vitteos/test/snapshots/kernel_smoke_runtime.log.snapshot"
REPORT="target/reports/kernel_smoke_runtime.log"
UPDATE=0

if [[ $# -gt 0 ]]; then
  if [[ $# -eq 1 && "$1" == "--update" ]]; then
    UPDATE=1
  else
    echo "usage: $0 [--update]" >&2
    exit 2
  fi
fi

KERNEL_IMAGE="${KERNEL_IMAGE:-}"
if [[ -z "${KERNEL_IMAGE}" ]]; then
  for candidate in \
    "target/kernel-tools/vittec-kernel" \
    "target/kernel.bin" \
    "build/kernel.bin" \
    "vitteos/build/kernel.bin"
  do
    if [[ -f "${candidate}" ]]; then
      KERNEL_IMAGE="${candidate}"
      break
    fi
  done
fi

if command -v qemu-system-x86_64 >/dev/null 2>&1; then
  EMU="qemu"
elif command -v bochs >/dev/null 2>&1; then
  EMU="bochs"
else
  mkdir -p "$(dirname "${REPORT}")"
  printf 'status=skipped\nreason=no-emulator\n' > "${REPORT}"
  echo "kernel smoke runtime: skipped (no qemu/bochs)"
  exit 0
fi

if [[ -z "${KERNEL_IMAGE}" ]]; then
  mkdir -p "$(dirname "${REPORT}")"
  printf 'status=skipped\nreason=no-kernel-image\n' > "${REPORT}"
  echo "kernel smoke runtime: skipped (no kernel image found)"
  exit 0
fi

if [[ "${EMU}" == "qemu" ]]; then
  tmp_raw="$(mktemp)"
  timeout 5s qemu-system-x86_64 \
    -nographic \
    -no-reboot \
    -no-shutdown \
    -serial mon:stdio \
    -kernel "${KERNEL_IMAGE}" > "${tmp_raw}" 2>&1 || true
  mkdir -p "$(dirname "${REPORT}")"
  lines="$(wc -l < "${tmp_raw}" | tr -d ' ')"
  hash="$(sha256sum "${tmp_raw}" | awk '{print $1}')"
  {
    echo "status=ok"
    echo "emulator=qemu"
    echo "kernel_image=${KERNEL_IMAGE}"
    echo "output_lines=${lines}"
    echo "output_sha256=${hash}"
  } > "${REPORT}"
  rm -f "${tmp_raw}"
  if [[ ${UPDATE} -eq 1 ]]; then
    mkdir -p "$(dirname "${SNAPSHOT}")"
    cp "${REPORT}" "${SNAPSHOT}"
    echo "updated snapshot: ${SNAPSHOT}"
  elif [[ -f "${SNAPSHOT}" ]]; then
    if ! diff -u "${SNAPSHOT}" "${REPORT}" >/dev/null; then
      echo "kernel smoke runtime snapshot mismatch: ${SNAPSHOT}" >&2
      echo "run: tools/vitteos_kernel_smoke_runtime.sh --update" >&2
      exit 1
    fi
  else
    echo "kernel smoke runtime snapshot missing: ${SNAPSHOT}" >&2
    echo "run: tools/vitteos_kernel_smoke_runtime.sh --update" >&2
    exit 1
  fi
  echo "kernel smoke runtime: qemu probe attempted with ${KERNEL_IMAGE}"
  exit 0
fi

mkdir -p "$(dirname "${REPORT}")"
printf 'status=skipped\nreason=bochs-not-configured\n' > "${REPORT}"
echo "kernel smoke runtime: bochs detected, runtime probe not configured (skipped)"
