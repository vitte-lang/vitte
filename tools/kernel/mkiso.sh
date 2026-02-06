#!/usr/bin/env bash
set -euo pipefail

if [[ $# -lt 2 ]]; then
  echo "usage: $0 <kernel.elf> <out.iso>"
  exit 1
fi

KERNEL_ELF="$1"
OUT_ISO="$2"

if ! command -v grub-mkrescue >/dev/null 2>&1; then
  echo "grub-mkrescue not found. On macOS: brew install grub"
  exit 1
fi

WORKDIR=$(mktemp -d)
mkdir -p "$WORKDIR/boot/grub"

cat > "$WORKDIR/boot/grub/grub.cfg" <<'CFG'
set timeout=0
set default=0

menuentry "Vitte Kernel" {
  multiboot2 /boot/kernel.elf
  boot
}
CFG

cp "$KERNEL_ELF" "$WORKDIR/boot/kernel.elf"

if [[ "$(uname)" == "Darwin" ]]; then
  grub-mkrescue -o "$OUT_ISO" "$WORKDIR" -- -iso-level 3
else
  grub-mkrescue -o "$OUT_ISO" "$WORKDIR"
fi

rm -rf "$WORKDIR"
