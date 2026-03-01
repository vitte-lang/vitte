#!/usr/bin/env python3
import json
import re
from pathlib import Path
USE=re.compile(r'^\s*use\s+([^\s]+)\s+as\s+([A-Za-z_][A-Za-z0-9_]*)')
SENSITIVE={'vitte/ffi','vitte/net','vitte/process'}

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

def main() -> int:
    repo=Path(__file__).resolve().parents[1]
    allow=read_allow(repo/'tests/modules/contracts/yaml/yaml_sensitive_imports.allow') | read_global_allow(repo)
    root=repo/'src/vitte/packages/yaml'
    errs=[]
    for p in root.rglob('*.vit'):
        rel=p.relative_to(repo).as_posix()
        for i,l in enumerate(p.read_text(encoding='utf-8').splitlines(),1):
            m=USE.match(l)
            if not m: continue
            t=m.group(1)
            if t in SENSITIVE and f'{rel}:{t}' not in allow:
                errs.append(f'{p}:{i}: sensitive import denied: {t}')
    if errs:
        for e in errs: print(f'[yaml-sensitive-imports][error] {e}')
        return 1
    print('[yaml-sensitive-imports] OK'); return 0
if __name__=='__main__': raise SystemExit(main())
