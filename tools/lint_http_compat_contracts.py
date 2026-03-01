#!/usr/bin/env python3
from pathlib import Path

def read_set(p: Path):
    if not p.exists(): return set()
    return {l.strip() for l in p.read_text(encoding='utf-8').splitlines() if l.strip()}

def check(root: Path, prefix: str, tag: str):
    cur=read_set(root/f'{prefix}.exports')
    exp=read_set(root/f'{prefix}.facade.api')
    pub=read_set(root/f'{prefix}.exports.public')
    errs=[]
    removed=sorted(exp-cur)
    if removed: errs.append('breaking removals detected: '+', '.join(removed))
    if cur!=pub: errs.append(f'{prefix}.exports and .public diverge')
    if errs:
        for e in errs: print(f'[{tag}][error] {e}')
        return 1
    print(f'[{tag}] OK'); return 0

if __name__=='__main__':
    repo=Path(__file__).resolve().parents[1]
    a=check(repo/'tests/modules/contracts/http','http','http-compat-contracts')
    b=check(repo/'tests/modules/contracts/http_client','http_client','http-client-compat-contracts')
    raise SystemExit(1 if (a or b) else 0)
