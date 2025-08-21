#!/usr/bin/env bash
set -euo pipefail
TARGETS=(tokenizer parser pretty loader disasm vm asm wasm_emitter ini csv)
for t in "${TARGETS[@]}"; do
  echo "==> Fuzzing $t"
  cargo +nightly fuzz run "$t" -- -dict=dicts/vitte.dict -timeout=5 -max_len=4096
done
