# Installer Real Platform Matrix

This file defines the release blocker for broad installation support.

Required binary architectures:

- `x86_64`
- `i386`
- `arm64`
- `armv7`
- `armv6`
- `riscv64`

Required Raspberry Pi targets:

- Raspberry Pi OS `armv6`
- Raspberry Pi OS `armv7`
- Raspberry Pi OS `aarch64`

Required real or VM smoke targets:

- Windows XP `i386`
- Windows 7 `i386`
- Windows 10 `amd64`
- Windows 11 `amd64`
- Debian oldstable `amd64`
- Debian stable `amd64`
- macOS Intel `x86_64`
- macOS Apple Silicon `arm64`
- Solaris `amd64`
- Solaris `i386`

Every target must install Vitte and run:

```sh
vitte --help
vitte check smoke.vit
vitte build smoke.vit -o smoke
```

Normal CI validates the matrix and scripts. Release CI must run
`STRICT_REAL_INSTALLERS=1 make installer-real-platforms-check` with real
artifacts and real runners.
