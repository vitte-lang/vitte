

# Vitte Bench Scripts

This directory contains the operational scripts used to run and validate the Vitte benchmark harness across CI and developer machines.

Design goals:

- **Deterministic artifacts**: scripts write outputs to stable paths under `compiler/compat/api/reports/<triple>/`.
- **CI-first**: scripts behave well in non-interactive environments and emit metadata files.
- **Overridable**: nearly all decisions can be overridden via environment variables.
- **Conservative**: no script silently changes system configuration (e.g., `kernel.perf_event_paranoid`).

> Naming convention: a **platform triple** is stored as `arch-unknown-os` (e.g., `x86_64-unknown-linux`, `aarch64-unknown-darwin`).

---

## Scripts

### `baseline_ci.sh`

Generates the canonical **baseline benchmark reports** for CI.

Outputs (default):

- `compiler/compat/api/reports/<triple>/bench_baseline.json`
- `compiler/compat/api/reports/<triple>/bench_baseline.txt` (optional)
- `compiler/compat/api/reports/<triple>/bench_stamp.txt`

The stamp file is a small, deterministic key-value file used to correlate artifacts.

Common usage:

```sh
# Baseline all suites (default bench behavior)
./compiler/bench/src/scripts/baseline_ci.sh

# Baseline a suite with 80 histogram bins
./compiler/bench/src/scripts/baseline_ci.sh --suite all --bins 80

# Force output directory
./compiler/bench/src/scripts/baseline_ci.sh --out-dir ./out/reports
```

Environment overrides:

- `VITTE_BENCH_EXE` — path to bench executable (repo-relative or absolute)
- `VITTE_BENCH_CMD` — full bench command string (takes precedence over `VITTE_BENCH_EXE`)
- `VITTE_REPO_ROOT` — repository root override
- `VITTE_REPORT_DIR` — report base dir override
- `VITTE_MODE` — build mode (default: `release`)
- `VITTE_TIMEOUT_S` — timeout seconds (best-effort; requires `timeout`)

GitHub Actions outputs:

When `GITHUB_OUTPUT` is present, the script appends:

- `bench_triple`
- `bench_report_dir`
- `bench_report_json`
- `bench_report_text` (if enabled)
- `bench_stamp`

---

### `perf_linux.sh`

Runs the bench harness under **Linux `perf`**.

Outputs (default):

- `compiler/compat/api/reports/<triple>/perf/perf_stat.txt`
- `compiler/compat/api/reports/<triple>/perf/perf_stat.csv`
- `compiler/compat/api/reports/<triple>/perf/perf_cmd.txt`
- `compiler/compat/api/reports/<triple>/perf/perf_stamp.txt`

Optional (when recording enabled):

- `compiler/compat/api/reports/<triple>/perf/perf.data`
- `compiler/compat/api/reports/<triple>/perf/perf_report.txt`

Common usage:

```sh
# perf stat for a suite
./compiler/bench/src/scripts/perf_linux.sh --suite alloc --iters 1000

# perf record + report
./compiler/bench/src/scripts/perf_linux.sh --suite alloc --record

# custom events and repeat
./compiler/bench/src/scripts/perf_linux.sh --events cycles,instructions --repeat 10
```

Environment overrides:

- `VITTE_BENCH_EXE`, `VITTE_BENCH_CMD`, `VITTE_REPO_ROOT`, `VITTE_MODE`, `VITTE_TRIPLE`, `VITTE_REPORT_DIR`
- `VITTE_PERF_BIN` — `perf` binary (default: `perf`)
- `VITTE_PERF_EVENTS` — perf events list
- `VITTE_PERF_REPEAT` — repeat count (default: `5`)
- `VITTE_PERF_RECORD` — `1` enables `perf record`
- `VITTE_PERF_FREQ` — record frequency (default: `99`)
- `VITTE_PERF_CALLGRAPH` — callgraph mode (default: `dwarf`)
- `VITTE_TIMEOUT_S` — timeout seconds

Notes:

- Many CI environments restrict perf usage via `kernel.perf_event_paranoid`. This script will **not** modify sysctls.
- For `perf record` call graphs, you may need additional packages (e.g., `linux-tools`, `dwarves`, debug symbols).

---

## Reproducibility and artifact layout

All scripts aim to produce predictable outputs:

- A **report directory** that is stable across runs.
- A **stamp file** that records key environment characteristics.
- A **command file** (for `perf`) recording the executed bench command.

Recommended CI artifact collection:

- Collect `compiler/compat/api/reports/**`.
- When using perf recording, also collect `compiler/compat/api/reports/**/perf/perf.data`.

---

## Extending

If you add new scripts:

- Prefer **POSIX sh** unless there is a hard requirement for bash.
- Always include a `--help` usage.
- Always support `VITTE_REPO_ROOT` and `VITTE_REPORT_DIR`.
- Write deterministic artifacts and a small stamp file.