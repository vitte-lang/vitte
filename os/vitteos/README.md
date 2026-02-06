# VitteOS (x86_64, GRUB, serial + VGA)

This is a minimal, text-only kernel experiment in Vitte.

## Build

```sh
vitte build --target kernel-x86_64-grub -o build/vitteos.elf /Users/vincent/Documents/Github/vitte/os/vitteos/kernel/main.vit
```

## Make ISO

```sh
/Users/vincent/Documents/Github/vitte/tools/kernel/mkiso.sh build/vitteos.elf build/vitteos.iso
```

## Run (QEMU)

```sh
qemu-system-x86_64 -cdrom build/vitteos.iso -serial stdio -display none
```

You should see the prompt on COM1 and in VGA text mode.

## Notes

- Serial output uses COM1 (0x3F8), mirrored to VGA text (0xB8000).
- A user-mode demo runs in ring3 via `int 0x80` syscalls.
- Memory allocator is a small bump allocator in the runtime.
