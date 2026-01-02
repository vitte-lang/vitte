# Architecture

- `driver.py` orchestrates generation based on a preset.
- `refresh_all.py` regenerates the `generated/` dataset root.
- `refresh_fuzz.py` optionally refreshes heavyweight fuzz fixtures.
- `gen_*` scripts generate individual dataset types.
- `vitte_benchgen/` contains shared utilities (write, scan, RNG).
- `tools/` contains operational helpers (diff, normalization, schema checks).

Outputs are designed for:
- deterministic compiler perf benchmarks
- CI baseline comparison
- reproducible fixture updates (manifest + sha256)
