#!/usr/bin/env python3
from pathlib import Path
import re

root = Path(__file__).resolve().parents[1]
chapters = sorted((root / 'chapters').glob('*.md'))
keywords_dir = root / 'keywords'

keyword_files = sorted(
    p for p in keywords_dir.glob('*.md')
    if p.name not in {'README.md', 'couverture.md', 'parcours.md', 'packs-apprentissage.md', 'non-utilises.md', 'erreurs-compilateur.md'}
)

code_fence = re.compile(r'```.*?```', re.DOTALL)

rows = []
for kf in keyword_files:
    kw = kf.stem
    kw_text = kf.read_text(encoding='utf-8')
    m_level = re.search(r'^Niveau:\s*(Débutant|Intermédiaire|Avancé)\.?\s*$', kw_text, re.MULTILINE)
    level = m_level.group(1) if m_level else '-'
    word_re = re.compile(rf'(?<![A-Za-z0-9_]){re.escape(kw)}(?![A-Za-z0-9_])')

    principal = '-'
    chapter_hits = 0
    example_hits = 0

    for ch in chapters:
        text = ch.read_text(encoding='utf-8')
        if word_re.search(text):
            chapter_hits += 1
            if principal == '-':
                principal = ch.name

        for block in code_fence.findall(text):
            example_hits += len(word_re.findall(block))

    rows.append((kw, level, principal, chapter_hits, example_hits))

out = [
    '# Couverture des keywords',
    '',
    'Cette page est générée automatiquement par `docs/book/scripts/generate_keyword_coverage.py`.',
    '',
    '| Mot-clé | Niveau | Chapitre principal | Chapitres couverts | Occurrences en exemples |',
    '| --- | --- | --- | --- | --- |',
]

for kw, level, principal, ch_hits, ex_hits in rows:
    chap_ref = f'`docs/book/chapters/{principal}`' if principal != '-' else '-'
    out.append(f'| `{kw}` | {level} | {chap_ref} | {ch_hits} | {ex_hits} |')

(keywords_dir / 'couverture.md').write_text('\n'.join(out) + '\n', encoding='utf-8')

unused = [kw for kw, _lvl, _principal, ch_hits, _ex_hits in rows if ch_hits == 0]
unused_out = [
    '# Keywords non utilisés dans les chapitres',
    '',
    'Cette liste est générée automatiquement par `docs/book/scripts/generate_keyword_coverage.py`.',
    '',
]
for kw in unused:
    unused_out.append(f'- `docs/book/keywords/{kw}.md`.')
(keywords_dir / 'non-utilises.md').write_text('\n'.join(unused_out) + '\n', encoding='utf-8')

print('generated', keywords_dir / 'couverture.md')
print('generated', keywords_dir / 'non-utilises.md')
print('keywords', len(rows))
