#!/usr/bin/env python3
from pathlib import Path
ROOT = Path(__file__).resolve().parents[1]
EN = ROOT / 'locales' / 'en' / 'diagnostics.ftl'
LOCALES = sorted([p for p in (ROOT / 'locales').iterdir() if p.is_dir() and p.name != 'en'])


def parse_ftl(path: Path):
    out = {}
    lines = []
    for raw in path.read_text(encoding='utf-8').splitlines():
        lines.append(raw)
        line = raw.strip()
        if not line or line.startswith('#') or '=' not in line:
            continue
        key, value = line.split('=', 1)
        out[key.strip()] = value.strip()
    return out, lines


en_msgs, _ = parse_ftl(EN)
added = 0
for loc_path in LOCALES:
    target = loc_path / 'diagnostics.ftl'
    if not target.exists():
        print(f'skipping {loc_path.name}: diagnostics.ftl missing')
        continue
    msgs, lines = parse_ftl(target)
    missing = [k for k in en_msgs.keys() if k not in msgs]
    if not missing:
        print(f'{loc_path.name}: no missing keys')
        continue
    print(f'{loc_path.name}: adding {len(missing)} keys')
    with target.open('a', encoding='utf-8') as fh:
        fh.write('\n# AUTOFILL: copied from en for missing keys\n')
        for k in missing:
            fh.write(f'{k} = {en_msgs[k]}\n')
            added += 1

print(f'done, total added: {added}')
