#!/usr/bin/env python3
from pathlib import Path
import argparse
import re
from collections import Counter

root = Path(__file__).resolve().parents[1]
repo = root.parents[1]
chapters = sorted((root / 'chapters').glob('*.html'))
keywords = sorted((root / 'chapters' / 'keywords').glob('*.html'))

required_global_files = [
    root / 'STYLE.html',
    root / 'summary.html',
    root / 'technical-index.html',
    root / 'checklist.html',
    root / 'classic-mistakes.html',
    root / 'errors.html',
    root / 'glossary.html',
    root / 'stdlib.html',
    root / 'chapters' / 'keywords' / 'couverture.html',
    root / 'chapters' / 'keywords' / 'parcours.html',
    root / 'chapters' / 'keywords' / 'packs-apprentissage.html',
    root / 'chapters' / 'keywords' / 'non-utilises.html',
    root / 'chapters' / 'keywords' / 'erreurs-compilateur.html',
]
ebnf_source = repo / 'src/vitte/grammar/vitte.ebnf'
ebnf_doc_root = root / 'grammar-surface.ebnf'
ebnf_doc_surface = root / 'grammar' / 'grammar-surface.ebnf'
ebnf_doc_legacy = root / 'grammar' / 'vitte.ebnf'

link_re = re.compile(r"""(?i)\b(?:href|src)=["']([^"']+)["']""")
level_re = re.compile(r'^Level:\s*(Beginner|Intermediate|Advanced)\.?\s*$', re.MULTILINE)
num_re = re.compile(r'^(\d+)')
kw_path_re = re.compile(r'`(docs/book/chapters/keywords/[a-z0-9\-]+\.html)`')
ch_path_re = re.compile(r'`(docs/book/chapters/[0-9a-z\-]+\.html)`')

parser = argparse.ArgumentParser(description='QA éditoriale book')
parser.add_argument('--strict', action='store_true', help='activer les contrôles stricts (répétitions et formulations génériques)')
args = parser.parse_args()

errors = []
warnings = []


TAG_RE = re.compile(r'<[^>]+>')


def strip_html(text: str) -> str:
    return TAG_RE.sub(' ', text)


def is_redirect_page(text: str) -> bool:
    low = text.lower()
    return 'http-equiv="refresh"' in low and 'rel="canonical"' in low


def add_issue(msg: str, strict_only: bool = False):
    if strict_only:
        if args.strict:
            errors.append(msg)
        else:
            warnings.append(msg)
    else:
        errors.append(msg)


def check_links(p: Path, lines: list[str]):
    for i, l in enumerate(lines, start=1):
        for m in link_re.finditer(l):
            target = m.group(1).strip()
            if not target or target.startswith(('http://', 'https://', '#', 'mailto:')):
                continue

            candidates = [
                (p.parent / target).resolve(),
                (root / target).resolve(),
                (repo / target).resolve(),
            ]
            if not any(c.exists() for c in candidates):
                add_issue(f"{p}:{i}: lien cassé -> {target}")


def repetition_check(p: Path, lines: list[str], threshold: int = 6):
    normalized = []
    for l in lines:
        s = strip_html(l).strip().lower()
        if not s:
            continue
        if s.startswith('<'):
            continue
        if s.startswith('lecture ligne par ligne'):
            continue
        if s.startswith('mini tableau entrée -> sortie'):
            continue
        if s.startswith('test mental standard'):
            continue
        if s.startswith('mental test:'):
            continue
        if s.startswith('réponse attendue: le bloc doit activer une garde explicite'):
            continue
        if 'brace closes the logical block' in s:
            continue
        if 'participates in the processing process' in s:
            continue
        if len(s) < 35:
            continue
        normalized.append(s)
    c = Counter(normalized)
    repeated = [(line, cnt) for line, cnt in c.items() if cnt >= threshold]
    if repeated:
        sample = repeated[0][0][:80]
        add_issue(f"{p}: répétitions excessives détectées (ex: '{sample}...')", strict_only=True)


def generic_phrase_check(p: Path, text: str):
    phrases = [
        'rend l’intention plus explicite et vérifiable',
        'sortie observable',
        'cas volontairement hors contrat',
    ]
    low = text.lower()
    for ph in phrases:
        if low.count(ph.lower()) >= 3:
            add_issue(f"{p}: formulation générique répétée: '{ph}'", strict_only=True)


for gf in required_global_files:
    if not gf.exists():
        add_issue(f"{gf}: fichier global manquant")

# EBNF source/doc artifacts must stay strictly aligned via sync script.
if not ebnf_source.exists():
    add_issue(f"{ebnf_source}: fichier EBNF source manquant")
else:
    src_txt = ebnf_source.read_text(encoding='utf-8')
    expected_marker = '# source: src/vitte/grammar/vitte.ebnf'
    for artifact in (ebnf_doc_root, ebnf_doc_surface, ebnf_doc_legacy):
        if not artifact.exists():
            add_issue(f"{artifact}: artefact EBNF généré manquant")
            continue
        doc_txt = artifact.read_text(encoding='utf-8')
        if expected_marker not in doc_txt or src_txt not in doc_txt:
            add_issue(
                f"{artifact}: diverge de {ebnf_source} "
                "(artefact non aligné, exécuter docs/book/grammar/scripts/sync_grammar.py)"
            )

# Chapters checks.
for p in chapters:
    t = p.read_text(encoding='utf-8')
    if is_redirect_page(t):
        continue
    lines = t.splitlines()
    mnum = num_re.match(p.name)
    n = int(mnum.group(1)) if mnum else None
    is_core_chapter = n is not None and n <= 31
    is_special_chapter = p.name.startswith('checklist-')

    if is_core_chapter and not is_special_chapter:
        m = re.search(r'<h2[^>]*>\s*Keywords to review\s*</h2>(.*?)(?=<h2[^>]*>)', t, flags=re.DOTALL | re.I)
        if m:
            sec_txt = m.group(1)
            links = kw_path_re.findall(sec_txt)
            if links and (len(links) < 3 or len(links) > 6):
                add_issue(f"{p}: Keywords to review should contain 3 to 6 links (current={len(links)})", strict_only=True)
            for link in links:
                lp = repo / link
                if not lp.exists():
                    add_issue(f"{p}: missing keyword link target: {link}")
                else:
                    # Bidirectional chapter -> keyword -> chapter.
                    kw_text = lp.read_text(encoding='utf-8')
                    chap_ref = f'docs/book/chapters/{p.name}'
                    if chap_ref not in kw_text:
                        add_issue(f"{p}: lien non bidirectionnel avec {link} (chapitre absent de 'Utilisé dans les chapitres')", strict_only=True)

    check_links(p, lines)
    repetition_check(p, lines)

# Keyword checks.
skip_kw = {'README.html', 'all.html', 'couverture.html', 'parcours.html', 'packs-apprentissage.html', 'non-utilises.html', 'erreurs-compilateur.html'}
for p in keywords:
    if p.name in skip_kw:
        continue

    t = p.read_text(encoding='utf-8')
    if is_redirect_page(t):
        continue
    lines = t.splitlines()

    # Bidirectional keyword -> chapters and lexical presence.
    m_used = re.search(r'<h2[^>]*>\s*Used in chapters\s*</h2>(.*?)(?=<h2[^>]*>)', t, flags=re.DOTALL | re.I)
    if m_used:
        sec_txt = m_used.group(1)
        ch_links = ch_path_re.findall(sec_txt)
        kw = p.stem
        word_re = re.compile(rf'(?<![A-Za-z0-9_]){re.escape(kw)}(?![A-Za-z0-9_])')
        for link in ch_links:
            cp = repo / link
            if not cp.exists():
                add_issue(f"{p}: chapitre référencé introuvable: {link}")
                continue
            ch_text = cp.read_text(encoding='utf-8')
            kw_ref = f'docs/book/chapters/keywords/{kw}.html'
            if not (word_re.search(strip_html(ch_text)) or kw_ref in ch_text):
                add_issue(f"{p}: lien non bidirectionnel vers {link} (mot-clé absent du chapitre)", strict_only=True)

    check_links(p, lines)
    repetition_check(p, lines)
    generic_phrase_check(p, t)

if errors:
    print('QA FAILED')
    for e in errors:
        print('-', e)
    if warnings:
        print('QA WARNINGS')
        for w in warnings:
            print('-', w)
    raise SystemExit(1)

print('QA OK')
print(f'chapters={len(chapters)}')
print(f'keywords={len([k for k in keywords if k.name not in skip_kw])}')
if warnings:
    print('QA WARNINGS')
    for w in warnings:
        print('-', w)
