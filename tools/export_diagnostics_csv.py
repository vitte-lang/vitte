#!/usr/bin/env python3
from pathlib import Path
import csv
ROOT = Path(__file__).resolve().parents[1]
EN = ROOT / 'locales' / 'en' / 'diagnostics.ftl'
OUT = ROOT / 'pkgout' / 'diagnostics_for_translation.csv'


def parse_ftl(path: Path):
    out = {}
    for raw in path.read_text(encoding='utf-8').splitlines():
        line = raw.strip()
        if not line or line.startswith('#') or '=' not in line:
            continue
        key, value = line.split('=', 1)
        out[key.strip()] = value.strip()
    return out


en = parse_ftl(EN)
OUT.parent.mkdir(parents=True, exist_ok=True)
with OUT.open('w', encoding='utf-8', newline='') as fh:
    writer = csv.writer(fh)
    writer.writerow(['key','english_text'])
    for k in sorted(en.keys()):
        writer.writerow([k,en[k]])
print('Wrote', OUT)
