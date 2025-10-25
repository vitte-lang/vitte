#!/usr/bin/env bash
set -euo pipefail

endpoint="${VITTE_LSP_ENDPOINT:-ws://127.0.0.1:9337/repl}"
iters=50
warmup=10
out="metrics.json"
budgets="benchmarks/repl-lsp/budgets.json"

while [[ $# -gt 0 ]]; do
  case "$1" in
    --endpoint) endpoint="$2"; shift 2;;
    --iterations) iters="$2"; shift 2;;
    --warmup) warmup="$2"; shift 2;;
    --out) out="$2"; shift 2;;
    --budgets) budgets="$2"; shift 2;;
    *) shift;;
  esac
done

echo "Running bench against $endpoint (iterations=$iters, warmup=$warmup)"
VITTE_LSP_ENDPOINT="$endpoint" cargo run -q -p vitte-repl --bin vitte-repl-bench -- --iterations "$iters" --warmup "$warmup" --out "$out"
echo "Metrics written to $out"

echo "Comparing against budgets: $budgets"
cargo run -q -p vitte-repl --bin vitte-repl-compare-budgets -- --metrics "$out" --budgets "$budgets"
echo "Bench OK"
