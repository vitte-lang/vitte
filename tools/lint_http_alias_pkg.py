#!/usr/bin/env python3
import re
from pathlib import Path
USE=re.compile(r'^\s*use\s+([^\s]+)\s+as\s+([A-Za-z_][A-Za-z0-9_]*)')

def check(paths, tag):
    errs=[]
    for root in paths:
        if not root.exists(): continue
        for p in root.rglob('*.vit'):
            for i,l in enumerate(p.read_text(encoding='utf-8').splitlines(),1):
                m=USE.match(l)
                if m and not m.group(2).endswith('_pkg'): errs.append(f'{p}:{i}: alias must end with _pkg')
    if errs:
        for e in errs: print(f'[{tag}][error] {e}')
        return 1
    print(f'[{tag}] OK'); return 0

if __name__=='__main__':
    repo=Path(__file__).resolve().parents[1]
    a=check([repo/'src/vitte/packages/http',repo/'tests/http',repo/'tests/modules/contracts/http'],'http-alias-pkg-lint')
    b=check([repo/'src/vitte/packages/http_client',repo/'tests/http_client',repo/'tests/modules/contracts/http_client'],'http-client-alias-pkg-lint')
    raise SystemExit(1 if (a or b) else 0)
