# Bench report JSON contract (vitte_rust_api)

This document defines the stable, machine-readable JSON emitted by the Rust FFI entrypoint `vitte_bench_report_write_json` (see `rust/include-gen/vitte_rust_api.h`).

## Encoding and escaping

- Output is **UTF-8**.
- JSON string escaping follows RFC 8259:
  - `"` → `\"`, `\` → `\\`
  - control characters use `\b \f \n \r \t` or `\u00XX`
- Input strings are treated as UTF-8; invalid sequences are replaced (lossy) to keep valid UTF-8 output.

## Field order and stability

- Objects are emitted with a **stable field order** exactly as shown in the schema below.
- New fields may be appended in future versions, but existing field names and their relative order are stable within an ABI version.

## Numbers

- Floating point values are printed with **exactly 6 digits after the decimal point** (same as C `%.6f`).
- `NaN`, `+Inf`, `-Inf` are normalized to `0.000000`.

## Top-level schema

Top-level object:

1. `schema` (string)
2. `suite` (string)
3. `timestamp_ms` (int)
4. `seed` (uint)
5. `threads` (int)
6. `repeat` (int)
7. `warmup` (int)
8. `iters` (int)
9. `calibrate_ms` (int)
10. `cpu` (object)
11. `results` (array)

`cpu` object:

1. `requested` (int)
2. `pinned` (int)

`results[]` object:

1. `name` (string)
2. `status` (string: `"ok" | "failed" | "skipped" | "unknown"`)
3. `iterations` (int)
4. `elapsed_ms` (float, 6 decimals)
5. `ns_per_op` (float, 6 decimals)
6. `bytes_per_sec` (float, 6 decimals)
7. `items_per_sec` (float, 6 decimals)
8. `ns_per_op_stats` (object)
9. `runner` (object)
10. `cpu_telemetry` (object)
11. `samples_ns_per_op` (array of float, 6 decimals; present only if `include_samples=1`)
12. `error` (`null` or string)

`ns_per_op_stats` object:

1. `median` (float, 6 decimals)
2. `p95` (float, 6 decimals)
3. `mad` (float, 6 decimals)
4. `iqr` (float, 6 decimals)
5. `ci95_low` (float, 6 decimals)
6. `ci95_high` (float, 6 decimals)

`runner` object:

1. `iters_per_call` (int)
2. `calls_per_sample` (int)
3. `target_time_ms` (int)

`cpu_telemetry` object:

1. `cycles_per_sec_min` (float, 6 decimals)
2. `cycles_per_sec_max` (float, 6 decimals)
3. `throttling_suspected` (bool)

