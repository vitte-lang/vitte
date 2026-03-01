#!/usr/bin/env python3
from __future__ import annotations
import json, hashlib
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CFG = ROOT / 'tools/facade_packages.json'


def sha(s: str) -> str:
    return hashlib.sha256(s.encode('utf-8')).hexdigest()


def read_lines(p: Path):
    return [l.strip() for l in p.read_text(encoding='utf-8').splitlines() if l.strip()] if p.exists() else []


def main() -> int:
    cfg = json.loads(CFG.read_text(encoding='utf-8'))
    errs=[]
    for ent in cfg['packages']:
        pkg=ent['name']
        cdir=ROOT/f'tests/modules/contracts/{pkg}'
        lock=cdir/f'{pkg}.contract.lock.json'
        if not lock.exists():
            errs.append(f'{pkg}: missing lockfile')
            continue
        data=json.loads(lock.read_text(encoding='utf-8'))
        exp=read_lines(cdir/f'{pkg}.exports')
        exp_sha=sha('\n'.join(exp)+'\n')
        if data.get('exports_sha256') != exp_sha:
            errs.append(f'{pkg}: lockfile exports_sha256 mismatch')
    if errs:
        for e in errs:
            print(f'[contract-lockfiles][error] {e}')
        return 1
    print('[contract-lockfiles] OK')
    return 0

if __name__=='__main__':
    raise SystemExit(main())
