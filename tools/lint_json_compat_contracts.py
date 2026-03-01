#!/usr/bin/env python3
from pathlib import Path

def read_set(p: Path):
    if not p.exists(): return set()
    return {l.strip() for l in p.read_text(encoding='utf-8').splitlines() if l.strip()}

def main() -> int:
    repo=Path(__file__).resolve().parents[1]
    root=repo/'tests/modules/contracts/json'
    cur=read_set(root/'json.exports')
    exp=read_set(root/'json.facade.api')
    pub=read_set(root/'json.exports.public')
    errs=[]
    removed=sorted(exp-cur)
    if removed: errs.append('breaking removals detected: '+', '.join(removed))
    if cur!=pub: errs.append('json.exports and json.exports.public diverge')
    if errs:
        for e in errs: print(f'[json-compat-contracts][error] {e}')
        return 1
    print('[json-compat-contracts] OK'); return 0
if __name__=='__main__': raise SystemExit(main())
