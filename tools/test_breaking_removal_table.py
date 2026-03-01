#!/usr/bin/env python3
from __future__ import annotations
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CFG = ROOT / 'tools/facade_packages.json'


def read_set(p: Path):
    return {l.strip() for l in p.read_text(encoding='utf-8').splitlines() if l.strip()} if p.exists() else set()


def main() -> int:
    cfg = json.loads(CFG.read_text(encoding='utf-8'))
    errs=[]
    for ent in cfg['packages']:
        pkg=ent['name']
        cdir=ROOT/f'tests/modules/contracts/{pkg}'
        baseline=read_set(cdir/f'{pkg}.facade.api')
        current=read_set(cdir/f'{pkg}.exports')
        removed=sorted(baseline-current)
        if removed:
            errs.append(f'{pkg}: removed exports: {", ".join(removed)}')
        print(f'[breaking-removal-table] {pkg}: baseline={len(baseline)} current={len(current)} removed={len(removed)}')
    if errs:
        for e in errs:
            print(f'[breaking-removal-table][error] {e}')
        return 1
    print('[breaking-removal-table] OK')
    return 0

if __name__=='__main__':
    raise SystemExit(main())
