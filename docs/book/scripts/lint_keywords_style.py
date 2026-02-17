#!/usr/bin/env python3
from pathlib import Path
import re
from collections import Counter

root = Path(__file__).resolve().parents[1]
keywords = sorted((root / 'keywords').glob('*.md'))
skip = {'README.md', 'couverture.md', 'parcours.md', 'packs-apprentissage.md', 'non-utilises.md', 'erreurs-compilateur.md'}

vague_terms = [
    'solide', 'robuste', 'clair', 'simple', 'facile', 'logique', 'important', 'utile',
]

issues = []

for p in keywords:
    if p.name in skip:
        continue
    t = p.read_text(encoding='utf-8')
    lines = t.splitlines()

    # Length metrics
    if len(lines) > 220:
        issues.append((p, 'warning', f'longueur élevée: {len(lines)} lignes'))
    if len(lines) < 40:
        issues.append((p, 'warning', f'fiche trop courte: {len(lines)} lignes'))

    # Repetition metric on non-code lines
    in_code = False
    norm = []
    for l in lines:
        if l.strip().startswith('```'):
            in_code = not in_code
            continue
        if in_code:
            continue
        s = l.strip().lower()
        if not s or s.startswith('#') or len(s) < 30:
            continue
        norm.append(s)
    c = Counter(norm)
    rep = [k for k, v in c.items() if v >= 3]
    if rep:
        issues.append((p, 'warning', f'répétitions détectées ({len(rep)} ligne(s))'))

    # Vague terms metric
    lowered = t.lower()
    count_vague = sum(lowered.count(w) for w in vague_terms)
    if count_vague >= 10:
        issues.append((p, 'warning', f'vocabulaire vague élevé: {count_vague} occurrences'))

if not issues:
    print('LINT OK')
    raise SystemExit(0)

print('LINT REPORT')
for p, lvl, msg in issues:
    print(f'- [{lvl}] {p}: {msg}')

# non-blocking by default
raise SystemExit(0)
