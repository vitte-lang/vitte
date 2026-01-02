# bench/detail

Private implementation headers for the Vitte benchmark harness.

## Design goals

- **C17-first**: clean C headers, friendly to C++ TU inclusion (`extern "C"`).
- **Portability**: minimal OS/compiler/arch feature layer.
- **Determinism**: stable behavior in CI; avoid relying on wall-clock time.
- **Low overhead**: keep hot-path helpers in headers (`static inline`) where appropriate.

## Files

### `compat.h`

Portability + attributes layer.

- OS / compiler / arch detection macros.
- Attributes (`FORCEINLINE`, `NOINLINE`, `NORETURN`, `THREADLOCAL`, `ALIGNAS`).
- Helpers: `LIKELY/UNLIKELY`, `DEBUGBREAK`, `STATIC_ASSERT`, path separator.
- Bench APIs (implemented in `.c`):
  - `vitte_bench_time_now_ns()`
  - `vitte_bench_cycles_now()`
  - `vitte_bench_sleep_ns()` / `vitte_bench_yield()`
  - `vitte_bench_pin_to_cpu()`
  - `vitte_bench_thread_set_name()`
  - `vitte_bench_snprintf()` / `vitte_bench_vsnprintf()`
  - `vitte_bench_panic()`

### `format.h`

Small formatting surface used by reporters and loggers.

- `vitte_sv` string-view.
- `vitte_buf` append buffer with NUL termination.
- `appendf` / `vappendf` helpers.
- Numeric append (`u64/i64/hex`).
- JSON-safe helpers (`append_json_string`, key/value helpers).

## API stability

Anything under `bench/detail/` is **not** a stable public API.
Public entrypoints should live in `bench/` includes.

## Contributing

- Keep includes minimal.
- Avoid non-standard language extensions in headers.
- Prefer adding new OS-specific logic to `.c` implementations and exposing only the signature in headers.
