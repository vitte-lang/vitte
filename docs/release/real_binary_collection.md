# Real Binary Collection

Vitte release binaries are accepted only from real machines or VMs that match the target OS and architecture.

Required targets:

- `linux-x86_64`
- `linux-i386`
- `linux-arm64`
- `linux-armv7`
- `linux-armv6`
- `linux-riscv64`
- `debian-oldstable-amd64`
- `debian-stable-amd64`
- `raspberry-pi-os-armv6`
- `raspberry-pi-os-armv7`
- `raspberry-pi-os-aarch64`
- `macos-x86_64`
- `macos-arm64`
- `solaris-amd64`
- `solaris-i386`
- `windows-xp-i386`
- `windows-7-i386`
- `windows-10-amd64`
- `windows-11-amd64`

On each runner, build the native `vitte` executable, then stage it:

```sh
make stage-real-binary OS=linux ARCH=x86_64 BIN=/path/to/vitte
```

Windows runners use the same contract with the target OS key and `.exe` binary:

```bat
python tools\stage_real_binary.py --os windows-11 --arch amd64 --binary C:\path\to\vitte.exe
```

The staging tool rejects scripts and placeholders. Expected binary formats:

- ELF for Linux, Debian, Raspberry Pi OS, and Solaris
- Mach-O for macOS
- PE for Windows

Each staged target must contain:

- `vitte` or `vitte.exe`
- `ATTESTATION.json`

The attestation records target OS, architecture, binary format, SHA-256, runner identity, and smoke command results.

Release check:

```sh
STRICT_REAL_INSTALLERS=1 RELEASE_INSTALLER_GATE=1 make release-installer-gate
```

This command must fail until every target has a real binary and passing smoke evidence.
