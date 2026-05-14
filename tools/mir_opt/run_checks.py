#!/usr/bin/env python3
from pathlib import Path
import sys
ROOT = Path(__file__).resolve().parents[2]
mod = ROOT/'src/vitte/compiler/optimizations/mir/mod.vit'
smoke = ROOT/'src/vitte/compiler/optimizations/mir/tests/smoke.vit'
if not mod.exists() or not smoke.exists():
    print('[mir-opt][error] missing MIR optimization files', file=sys.stderr); raise SystemExit(1)
text = mod.read_text(encoding='utf-8')
for sym in ['constant_folding','dce_advanced','function_inlining','loop_optimizations','escape_analysis','memory_optimizations','run_all_mir_passes']:
    if sym not in text:
        print(f'[mir-opt][error] missing symbol {sym}', file=sys.stderr); raise SystemExit(1)
print('[mir-opt] checks passed')
