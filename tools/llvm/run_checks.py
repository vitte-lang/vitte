#!/usr/bin/env python3
from pathlib import Path
import sys
ROOT = Path(__file__).resolve().parents[2]
req = [
 ROOT/'src/vitte/compiler/backends/llvm_bindings/mod.vit',
 ROOT/'src/vitte/compiler/backends/llvm_bindings/tests/smoke.vit',
 ROOT/'src/vitte/compiler/backends/vitte_emit/mod.vit',
 ROOT/'tools/llvm/generate_artifacts.py',
]
for p in req:
    if not p.exists():
        print(f'[llvm][error] missing {p}', file=sys.stderr)
        raise SystemExit(1)
mod = (ROOT/'src/vitte/compiler/backends/llvm_bindings/mod.vit').read_text(encoding='utf-8')
for needle in ['supported_opt_levels', 'emit_llvm_ir_from_mir', 'debug_info_format', 'pgo_enabled', 'emit_debug_info', 'emit_pgo_instrumentation']:
    if needle not in mod:
        print(f'[llvm][error] missing symbol {needle}', file=sys.stderr)
        raise SystemExit(1)
print('[llvm] checks passed')
