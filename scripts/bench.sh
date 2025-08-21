#!/usr/bin/env bash
# bench.sh — Vitte workspace micro/meso benchmarks
# Usage:
#   ./scripts/bench.sh [--quick] [--filter <regex>] [--iterations N]
#                      [--criterion] [--hyperfine] [--wasm] [--riscv]
#                      [--out DIR] [--sample PATH] [--cmd "custom command"]
#
# Philosophy:
#  - No flaky assumptions. If Criterion benches exist -> use `cargo bench`.
#  - Else, use `hyperfine` to time common commands (build/run/disasm).
#  - WASM: run vitte-wasi-runner with wasmtime if available.
#  - RISC-V: build via cross; run with qemu-riscv64 if available (optional).
#
# Requirements:
#  - cargo (stable)
#  - Optional: hyperfine, wasmtime, cross, qemu-riscv64
#
set -euo pipefail

# --------------------------- args parsing ------------------------------------
QUICK=0
FILTER=""
ITER=10
DO_CRITERION=0
DO_HYPERFINE=0
DO_WASM=0
DO_RISCV=0
OUT="bench_out/$(date +%Y%m%d_%H%M%S)"
SAMPLE="examples/bench/hello.vitbc"
CUSTOM_CMD=""

while [[ $# -gt 0 ]]; do
  case "$1" in
    --quick) QUICK=1; ITER=5; shift ;;
    --filter) FILTER="$2"; shift 2 ;;
    --iterations) ITER="$2"; shift 2 ;;
    --criterion) DO_CRITERION=1; shift ;;
    --hyperfine) DO_HYPERFINE=1; shift ;;
    --wasm) DO_WASM=1; shift ;;
    --riscv) DO_RISCV=1; shift ;;
    --out) OUT="$2"; shift 2 ;;
    --sample) SAMPLE="$2"; shift 2 ;;
    --cmd) CUSTOM_CMD="$2"; shift 2 ;;
    -h|--help)
      sed -n '1,80p' "$0"; exit 0 ;;
    *) echo "Unknown arg: $1" >&2; exit 2 ;;
  esac
done

mkdir -p "$OUT"

# --------------------------- helpers -----------------------------------------
have() { command -v "$1" >/dev/null 2>&1; }
log() { printf "\033[1;36m>>> %s\033[0m\n" "$*"; }
warn(){ printf "\033[1;33m!! %s\033[0m\n" "$*"; }
die() { printf "\033[1;31mXX %s\033[0m\n" "$*"; exit 1; }

detect_cpu() {
  if [[ "$(uname -s)" == "Darwin" ]]; then
    sysctl -n machdep.cpu.brand_string 2>/dev/null || echo "Apple/Intel"
  elif have lscpu; then
    lscpu | sed -n '1,4p'
  else
    uname -a
  fi
}

# --------------------------- preflight ---------------------------------------
log "Workspace root: $(pwd)"
log "CPU:"; detect_cpu | sed 's/^/    /'
log "Output dir: $OUT"

if [[ ! -f Cargo.toml ]]; then
  warn "No Cargo.toml at repo root; continuing, but cargo may fail."
fi

if [[ ! -f "$SAMPLE" ]]; then
  warn "Sample bytecode not found at $SAMPLE — trying to build an example."
  # Best effort: create a tiny sample if a compiler exists later; else, fallback to /dev/zero timing.
fi

# Default modes if none selected
if [[ $DO_CRITERION -eq 0 && $DO_HYPERFINE -eq 0 && $DO_WASM -eq 0 && $DO_RISCV -eq 0 ]]; then
  DO_HYPERFINE=1
  DO_WASM=1
fi

# --------------------------- cargo build (host) -------------------------------
log "Building workspace (release)…"
cargo build --workspace --release

# Try to locate vitte-cli binary (host)
HOST_BIN="target/release/vitte"
if [[ ! -x "$HOST_BIN" ]]; then
  # Try common bin names
  HOST_BIN=$(fd -a -t f -E target/debug -E target/release 'vitte(|-cli)$' 2>/dev/null | head -n1 || true)
fi
if [[ -z "${HOST_BIN:-}" || ! -e "$HOST_BIN" ]]; then
  warn "vitte CLI not found; hyperfine will use a placeholder echo if needed."
fi

# --------------------------- Criterion ---------------------------------------
if [[ $DO_CRITERION -eq 1 ]]; then
  log "Running Criterion benches (cargo bench)…"
  set +e
  if [[ -n "$FILTER" ]]; then
    cargo bench -- --filter "$FILTER"
  else
    cargo bench
  fi
  RC=$?
  set -e
  if [[ $RC -ne 0 ]]; then
    warn "cargo bench failed (maybe no benches configured?)."
  else
    log "Criterion completed."
  fi
fi

# --------------------------- Hyperfine ---------------------------------------
if [[ $DO_HYPERFINE -eq 1 ]]; then
  if ! have hyperfine; then
    warn "hyperfine not installed; skipping hyperfine benchmarks."
  else
    log "Hyperfine micro-benchmarks…"
    JSON="$OUT/hyperfine.json"
    MD="$OUT/hyperfine.md"
    CSV="$OUT/hyperfine.csv"

    CMDS=()
    if [[ -n "$CUSTOM_CMD" ]]; then
      CMDS+=("$CUSTOM_CMD")
    fi
    if [[ -n "$HOST_BIN" && -e "$HOST_BIN" ]]; then
      if [[ -f "$SAMPLE" ]]; then
        CMDS+=("$HOST_BIN run $SAMPLE")
        CMDS+=("$HOST_BIN disasm $SAMPLE")
      else
        CMDS+=("$HOST_BIN --help")
      fi
    else
      CMDS+=("echo 'host binary not found'")
    fi

    log "Commands:"
    for c in "${CMDS[@]}"; do echo "  - $c"; done

    hyperfine --warmup 3 --runs "$ITER" \
      --export-json "$JSON" \
      --export-markdown "$MD" \
      --export-csv "$CSV" \
      "${CMDS[@]}"
    log "Hyperfine results -> $JSON | $MD | $CSV"
  fi
fi

# --------------------------- WASM (WASI) -------------------------------------
if [[ $DO_WASM -eq 1 ]]; then
  if ! have wasmtime; then
    warn "wasmtime not installed; skipping WASI run."
  else
    log "Building WASI runner…"
    cargo build -p vitte-wasi-runner --target wasm32-wasi --release || warn "WASI build failed"
    WASM_BIN="target/wasm32-wasi/release/vitte-wasi-runner.wasm"
    if [[ -f "$WASM_BIN" ]]; then
      log "Running WASI sample via wasmtime…"
      if [[ -f "$SAMPLE" ]]; then
        wasmtime run --dir=. -- "$WASM_BIN" "$SAMPLE" | tee "$OUT/wasi_run.txt"
      else
        wasmtime run --dir=. -- "$WASM_BIN" || true
      fi
      if have hyperfine; then
        log "Hyperfine (WASI)…"
        hyperfine --warmup 3 --runs "$ITER" \
          --export-json "$OUT/hf_wasi.json" \
          "wasmtime run --dir=. -- $WASM_BIN $SAMPLE"
      fi
    else
      warn "WASM binary not found at $WASM_BIN"
    fi
  fi
fi

# --------------------------- RISC-V ------------------------------------------
if [[ $DO_RISCV -eq 1 ]]; then
  if ! have cross; then
    warn "cross not installed; skipping RISC-V build."
  else
    log "Building RISC-V (cross)…"
    cross build --target riscv64gc-unknown-linux-gnu --workspace --release || warn "RISC-V build failed"
    # Try to run with qemu (optional)
    if have qemu-riscv64; then
      # Attempt to run vitte CLI if available
      RV_BIN="target/riscv64gc-unknown-linux-gnu/release/vitte"
      if [[ -f "$RV_BIN" && -f "$SAMPLE" ]]; then
        log "qemu-riscv64 run (best-effort)…"
        qemu-riscv64 -L /usr/riscv64-linux-gnu "$RV_BIN" run "$SAMPLE" | tee "$OUT/riscv_run.txt" || true
      fi
    fi
  fi
fi

# --------------------------- summary -----------------------------------------
log "Done. Artifacts in: $OUT"
if [[ -d "$OUT" ]]; then
  echo "  - $(ls -1 "$OUT" | wc -l) files written"
  ls -1 "$OUT" | sed 's/^/    /'
fi
