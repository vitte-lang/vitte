#!/bin/bash
# run_benchmarks.sh - Run benchmarks with various options

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"
BENCHC="${BUILD_DIR}/benchc"

# Ensure benchmarks are built
if [ ! -x "$BENCHC" ]; then
  echo "[vitte-bench] Binary not found, building..."
  "$SCRIPT_DIR/build.sh"
fi

echo "[vitte-bench] Running benchmarks..."
echo ""

# Run all benchmarks
"$BENCHC" --all

echo ""
echo "[vitte-bench] Benchmark results available via:"
echo "  $BENCHC --list           # Show available benchmarks"
echo "  $BENCHC --iters 2000000 micro:add   # Custom iterations"
echo "  $BENCHC --seconds 3 macro:json_parse # Custom duration"
