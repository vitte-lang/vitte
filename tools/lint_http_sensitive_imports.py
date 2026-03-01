#!/usr/bin/env python3
import json
import re
from pathlib import Path
USE=re.compile(r'^\s*use\s+([^\s]+)\s+as\s+([A-Za-z_][A-Za-z0-9_]*)')
SENSITIVE={'vitte/ffi','vitte/process','vitte/net'}

def read_allow(path: Path):
    if not path.exists(): return set()
    out=set()
    for l in path.read_text(encoding='utf-8').splitlines():
        t=l.strip()
        if t and not t.startswith('#'): out.add(t)
    return out

def read_global_allow(repo: Path):
    p = repo / 'tools/allowlist_policy.json'
    if not p.exists():
        return set()
    try:
        data = json.loads(p.read_text(encoding='utf-8'))
    except Exception:
        return set()
    return set(data.get('sensitive_import_allowlist', []))

def check(root: Path, allow: set[str], tag:str):
    errs=[]
    for p in root.rglob('*.vit'):
        rel=p.relative_to(Path(__file__).resolve().parents[1]).as_posix()
        for i,l in enumerate(p.read_text(encoding='utf-8').splitlines(),1):
            m=USE.match(l)
            if not m: continue
            t=m.group(1)
            if t in SENSITIVE and f'{rel}:{t}' not in allow:
                errs.append(f'{p}:{i}: sensitive import denied: {t}')
    if errs:
        for e in errs: print(f'[{tag}][error] {e}')
        return 1
    print(f'[{tag}] OK'); return 0

if __name__=='__main__':
    repo=Path(__file__).resolve().parents[1]
    g = read_global_allow(repo)
    a=check(repo/'src/vitte/packages/http',read_allow(repo/'tests/modules/contracts/http/http_sensitive_imports.allow') | g,'http-sensitive-imports')
    b=check(repo/'src/vitte/packages/http_client',read_allow(repo/'tests/modules/contracts/http_client/http_client_sensitive_imports.allow') | g,'http-client-sensitive-imports')
    raise SystemExit(1 if (a or b) else 0)
