# Vitte 2025 — Steel toolchain (native-first, single-stage)

This is a **complete scaffold** for a portable Vitte toolchain in C:

- `steel`  : build driver (parses `muffin.muf`, plans builds, invokes toolchain)
- `steelc` : compiler (Vitte -> C11 backend) — minimal working subset
- `runtime` + `pal` : portability layer (POSIX/Win32) — skeleton contracts

Goals:
- portable targets: Linux/macOS/Windows/*BSD/Solaris-style POSIX
- AOT native output via generated C (then clang/zig cc)
- MUF as the only build manifest (required)

Notes:
- This repo contains a minimal but **functional** `steelc` that can compile a tiny Vitte subset:
  `module`, `fn main() -> i32`, `return <int>;`, `.end` blocks.
- `steel` currently plans builds and can execute toolchain commands if enabled (see TODOs).

Layout:
- `include/steel/**` public headers
- `src/**` implementations
- `std/` Vitte stdlib placeholder
- `examples/` example projects
