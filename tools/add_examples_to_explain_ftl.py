#!/usr/bin/env python3
from pathlib import Path
ROOT = Path(__file__).resolve().parents[1]
LOCALES_DIR = ROOT / 'locales'
EN_EXPLAIN = LOCALES_DIR / 'en' / 'diagnostics_explain.ftl'


def parse_ftl(path: Path):
    out = {}
    for raw in path.read_text(encoding='utf-8').splitlines():
        line = raw.strip()
        if not line or line.startswith('#') or '=' not in line:
            continue
        key, value = line.split('=', 1)
        out[key.strip()] = value.strip()
    return out


en = parse_ftl(EN_EXPLAIN)
# determine base codes from en explain by taking keys before suffix
codes = set(k.split('.',1)[0] for k in en.keys())
added = 0
for loc in sorted([d for d in LOCALES_DIR.iterdir() if d.is_dir()]):
    explain_file = loc / 'diagnostics_explain.ftl'
    if not explain_file.exists():
        continue
    explain = parse_ftl(explain_file)
    to_add = []
    for code in codes:
        ex_key = f'{code}.example'
        if ex_key not in explain:
            # construct example from summary if available
            summary_key = f'{code}.summary'
            example_text = explain.get(summary_key) or en.get(summary_key) or 'Example omitted; please add.'
            to_add.append((ex_key, f'Example: {example_text}'))
    if to_add:
        with explain_file.open('a', encoding='utf-8') as fh:
            fh.write('\n# AUTOGEN: added example placeholders\n')
            for k,v in to_add:
                fh.write(f"{k} = {v}\n")
                added += 1
print('added examples:', added)
