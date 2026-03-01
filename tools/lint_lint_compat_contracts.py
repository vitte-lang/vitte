#!/usr/bin/env python3
from pathlib import Path

def read_set(p: Path):
    if not p.exists(): return set()
    return {l.strip() for l in p.read_text(encoding='utf-8').splitlines() if l.strip()}

def main() -> int:
    repo=Path(__file__).resolve().parents[1]
    root=repo/'tests/modules/contracts/lint'
    cur=read_set(root/'lint.exports')
    exp=read_set(root/'lint.facade.api')
    pub=read_set(root/'lint.exports.public')
    errs=[]
    removed=sorted(exp-cur)
    if removed: errs.append('breaking removals detected: '+', '.join(removed))
    if cur!=pub: errs.append('lint.exports and lint.exports.public diverge')
    if errs:
        for e in errs: print(f'[lint-compat-contracts][error] {e}')
        return 1
    print('[lint-compat-contracts] OK'); return 0
if __name__=='__main__': raise SystemExit(main())
