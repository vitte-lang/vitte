#!/usr/bin/env python3
from __future__ import annotations
import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CFG = ROOT / 'tools/facade_packages.json'
CODE_RE = re.compile(r'VITTE-[A-Z]+[0-9]{4}')


def codes_in_runtime(path: Path, marker: str) -> set[str]:
    if not path.exists():
        return set()
    txt = path.read_text(encoding='utf-8')
    m = re.search(rf'proc\s+{marker}\s*\([^\)]*\)\s*->\s*string\s*\{{(.*?)\n\}}', txt, flags=re.S)
    if not m:
        return set()
    return set(CODE_RE.findall(m.group(1)))


def main() -> int:
    cfg = json.loads(CFG.read_text(encoding='utf-8'))
    errs = []
    for ent in cfg['packages']:
        p = ent['name']
        mod = ROOT / f'src/vitte/packages/{p}/mod.vit'
        rt = ROOT / f'src/vitte/packages/{p}/internal/runtime.vit'
        if not mod.exists() or not rt.exists():
            continue
        mod_txt = mod.read_text(encoding='utf-8')
        if 'proc diagnostics_doc_url(' not in mod_txt:
            errs.append(f'{p}: missing diagnostics_doc_url in facade')
        msg_codes = codes_in_runtime(rt, 'diag_message')
        fix_codes = codes_in_runtime(rt, 'quickfix_for')
        if not msg_codes:
            errs.append(f'{p}: missing or unparsable diag_message mapping in runtime')
            continue
        unknown_fix_codes = sorted([c for c in fix_codes if c not in msg_codes])
        if unknown_fix_codes:
            errs.append(
                f'{p}: quickfix_for has unknown codes not declared in diag_message: '
                + ', '.join(unknown_fix_codes)
            )
    if errs:
        for e in errs:
            print(f'[diag-stability][error] {e}')
        return 1
    print('[diag-stability] OK')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
