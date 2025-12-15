# Vitte 2025 — Toolchain "Steel" (portable native, single-stage)

Components
- `steel`  : build driver (reads `muffin.muf`, plans/executes builds)
- `steelc` : compiler (Vitte -> C17 backend) — scaffolded core
- `runtime`: minimal ABI (slices/handles/panic) — TODO
- `pal`    : Platform Abstraction Layer (POSIX/Win32) — TODO

Build model
- `muffin.muf` is mandatory. Without it, nothing builds.
- The output model mirrors Rust/Cargo intent (packages, profiles, lockfile),
  but MUF syntax is Vitte-native (`.end` blocks).

Status
- This repository is a **complete project skeleton**: directories, headers, docs, tests, fuzz, bench.
- A minimal `steelc` subset can be wired as the first milestone (parser_core + backend_c).
