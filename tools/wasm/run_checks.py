#!/usr/bin/env python3
from pathlib import Path
import sys
ROOT = Path(__file__).resolve().parents[2]
req = [ROOT/'src/vitte/compiler/backends/wasm/mod.vit', ROOT/'src/vitte/compiler/backends/wasm/tests/smoke.vit', ROOT/'src/vitte/compiler/backends/vitte_emit/mod.vit']
for p in req:
    if not p.exists():
        print(f'[wasm][error] missing {p}', file=sys.stderr); raise SystemExit(1)
src=(ROOT/'src/vitte/compiler/backends/wasm/mod.vit').read_text(encoding='utf-8')
for sym in ['emit_wat','wasi_enabled','web_api_surface','size_optimization_mode']:
    if sym not in src:
        print(f'[wasm][error] missing symbol {sym}', file=sys.stderr); raise SystemExit(1)
print('[wasm] checks passed')
