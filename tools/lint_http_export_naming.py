#!/usr/bin/env python3
import re
from pathlib import Path
NAME=re.compile(r'^([a-z][a-z0-9_]*|[A-Z][A-Za-z0-9]*)$')

def check(path: Path, tag: str):
    if not path.exists():
        print(f'[{tag}][error] missing {path}')
        return 1
    errs=[]
    for i,l in enumerate(path.read_text(encoding='utf-8').splitlines(),1):
        t=l.strip()
        if t and not NAME.match(t): errs.append(f'{path}:{i}: invalid {t}')
    if errs:
        for e in errs: print(f'[{tag}][error] {e}')
        return 1
    print(f'[{tag}] OK'); return 0

if __name__=='__main__':
    repo=Path(__file__).resolve().parents[1]
    a=check(repo/'tests/modules/contracts/http/http.exports','http-export-naming')
    b=check(repo/'tests/modules/contracts/http_client/http_client.exports','http-client-export-naming')
    raise SystemExit(1 if (a or b) else 0)
