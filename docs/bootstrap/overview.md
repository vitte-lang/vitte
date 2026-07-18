# Bootstrap Overview

Vitte installs the manifest-pinned `toolchain/seed/vittec0.seed` as
`bin/vittec0`, validates native contracts, and checks the canonical compiler
entry at `src/vitte/compiler/main.vit`.

Primary checks:

- `make seed-verify`
- `make bootstrap-vitte-hard-gate`
- `make bootstrap-verify`
- `python3 tools/selfhost_completion_audit.py`

The seed-rooted bootstrap is operational. Official autonomous self-hosting
remains incomplete until successive generated compilers reach byte parity and
the transition bridge is removed.
