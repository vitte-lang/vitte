#!/usr/bin/env python3
from pathlib import Path

def main()->int:
    repo=Path(__file__).resolve().parents[1]
    txt=(repo/'src/vitte/packages/test/mod.vit').read_text(encoding='utf-8')
    itxt=(repo/'src/vitte/packages/test/internal/runner.vit').read_text(encoding='utf-8')
    errs=[]
    for need in ['TestMode','SnapshotConfig','FuzzConfig','BenchConfig','run_fuzz','run_bench']:
        if need not in txt: errs.append(f'missing {need} in facade')
    for need in ['VITTE-T0014','VITTE-T0015','diag_message','quickfix_for']:
        if need not in itxt: errs.append(f'missing {need} in runtime')
    if errs:
        for e in errs: print(f'[test-security-lint][error] {e}')
        return 1
    print('[test-security-lint] OK'); return 0
if __name__=='__main__': raise SystemExit(main())
