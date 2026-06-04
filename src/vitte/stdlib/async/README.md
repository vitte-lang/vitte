# async

Path: `src/vitte/stdlib/async`

## Purpose

Vitte standard library modules and runtime-facing surfaces.

## Notes

- Keep module APIs stable and documented.
- Maintain ABI expectations for runtime/interop layers.
- Add compatibility notes when changing behavior.
- The current stdlib implementation is portable and synchronous-first:
  `async`, `future_map`, `future_chain`, channels, and executor state are
  coherent and smoke-tested, but they do not yet rely on a preemptive runtime.
