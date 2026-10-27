# Bootstrap Overview

Vitte installs the manifest-pinned `toolchain/seed/vittec0.seed` as
`bin/vittec0` and validates native contracts. The default compiler executable
is implemented under `src/bootstrap/`; native Vitte compiler components are
staged under `src/compiler/`.

Primary checks:

- `make seed-verify`
- `make bootstrap-vitte-hard-gate`
- `make bootstrap-verify`
- See [the native frontend port notes](frontend_native_port.md) for current migration status.

The seed-rooted bootstrap is operational. Official autonomous self-hosting
remains incomplete until successive generated compilers reach byte parity and
the transition bridge is removed.
