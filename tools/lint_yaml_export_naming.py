#!/usr/bin/env python3
import re
from pathlib import Path
NAME=re.compile(r'^([a-z][a-z0-9_]*|[A-Z][A-Za-z0-9]*)$')

def main() -> int:
    repo=Path(__file__).resolve().parents[1]
    path=repo/'tests/modules/contracts/yaml/yaml.exports'
    if not path.exists():
        print(f'[yaml-export-naming][error] missing {path}')
        return 1
    errs=[]
    for i,l in enumerate(path.read_text(encoding='utf-8').splitlines(),1):
        t=l.strip()
        if t and not NAME.match(t): errs.append(f'{path}:{i}: invalid {t}')
    if errs:
        for e in errs: print(f'[yaml-export-naming][error] {e}')
        return 1
    print('[yaml-export-naming] OK'); return 0
if __name__=='__main__': raise SystemExit(main())
