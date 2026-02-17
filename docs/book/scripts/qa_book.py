#!/usr/bin/env python3
from pathlib import Path
import argparse
import re
from collections import Counter

root = Path(__file__).resolve().parents[1]
repo = root.parents[1]
chapters = sorted((root / 'chapters').glob('*.md'))
keywords = sorted((root / 'keywords').glob('*.md'))

required_global_files = [
    root / 'STYLE.md',
    root / 'keywords' / 'couverture.md',
    root / 'keywords' / 'parcours.md',
    root / 'keywords' / 'packs-apprentissage.md',
    root / 'keywords' / 'non-utilises.md',
    root / 'keywords' / 'erreurs-compilateur.md',
]
ebnf_source = repo / 'src/vitte/grammar/vitte.ebnf'
ebnf_doc = root / 'grammar-surface.ebnf'

link_re = re.compile(r'\[[^\]]+\]\(([^)]+)\)')
level_re = re.compile(r'^Niveau:\s*(Débutant|Intermédiaire|Avancé)\.?\s*$', re.MULTILINE)
num_re = re.compile(r'^(\d+)')
kw_path_re = re.compile(r'`(docs/book/keywords/[a-z0-9\-]+\.md)`')
ch_path_re = re.compile(r'`(docs/book/chapters/[0-9a-z\-]+\.md)`')

required_chapter_sections = [
    '## Objectif',
    '## Exemple',
    '## Pourquoi',
    '## Test mental',
    '## À faire',
    '## Corrigé minimal',
    '## Conforme EBNF',
]

required_keyword_sections = [
    '## Définition',
    '## Syntaxe',
    '## Exemple nominal',
    '## Exemple invalide',
    '## Pièges',
    '## Voir aussi',
    '## Quand l’utiliser / Quand l’éviter',
    '## Erreurs compilateur fréquentes',
    '## Mot-clé voisin',
    '## Utilisé dans les chapitres',
]

parser = argparse.ArgumentParser(description='QA éditoriale docs/book')
parser.add_argument('--strict', action='store_true', help='activer les contrôles stricts (répétitions et formulations génériques)')
args = parser.parse_args()

errors = []
warnings = []


def add_issue(msg: str, strict_only: bool = False):
    if strict_only:
        if args.strict:
            errors.append(msg)
        else:
            warnings.append(msg)
    else:
        errors.append(msg)


def check_links(p: Path, lines: list[str]):
    in_code = False
    for i, l in enumerate(lines, start=1):
        if l.strip().startswith('```'):
            in_code = not in_code
            continue
        if in_code:
            continue

        if re.match(r'^\s*-\s+\S', l):
            if not re.search(r'[\.!\?:;]$', l.strip()):
                add_issue(f"{p}:{i}: ponctuation manquante en fin de puce")

        scan = re.sub(r'`[^`]*`', '', l)
        for m in link_re.finditer(scan):
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
    in_code = False
    for l in lines:
        if l.strip().startswith('```'):
            in_code = not in_code
            continue
        if in_code:
            continue
        s = l.strip().lower()
        if not s:
            continue
        if s.startswith('#'):
            continue
        if s.startswith('lecture ligne par ligne'):
            continue
        if s.startswith('mini tableau entrée -> sortie'):
            continue
        if s.startswith('test mental standard'):
            continue
        if s.startswith('réponse attendue: le bloc doit activer une garde explicite'):
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

# EBNF source/doc must stay strictly aligned.
if ebnf_source.exists() and ebnf_doc.exists():
    src_txt = ebnf_source.read_text(encoding='utf-8')
    doc_txt = ebnf_doc.read_text(encoding='utf-8')
    if src_txt != doc_txt:
        add_issue(f"{ebnf_doc}: diverge de {ebnf_source} (copie documentaire non alignée)")
else:
    add_issue(f"{ebnf_source} ou {ebnf_doc}: fichier EBNF manquant")

# Chapters checks.
for p in chapters:
    t = p.read_text(encoding='utf-8')
    lines = t.splitlines()
    mnum = num_re.match(p.name)
    n = int(mnum.group(1)) if mnum else None

    for sec in required_chapter_sections:
        if sec not in t:
            add_issue(f"{p}: section manquante: {sec}")

    if '## Trame du chapitre' not in t:
        add_issue(f"{p}: section manquante: ## Trame du chapitre")

    if not level_re.search(t):
        add_issue(f"{p}: Niveau manquant ou invalide (Débutant/Intermédiaire/Avancé)")

    if 'Prérequis:' not in t:
        add_issue(f"{p}: bandeau manquant: Prérequis")

    if 'Voir aussi:' not in t:
        add_issue(f"{p}: renvoi manquant: Voir aussi")

    if '## Keywords à revoir' not in t:
        add_issue(f"{p}: section manquante: ## Keywords à revoir")
    else:
        m = re.search(r'## Keywords à revoir\n\n(.*?)(?=\n## )', t, flags=re.DOTALL)
        if m:
            sec_txt = m.group(1)
            links = kw_path_re.findall(sec_txt)
            if len(links) < 3 or len(links) > 5:
                add_issue(f"{p}: Keywords à revoir doit contenir 3 à 5 liens (actuel={len(links)})")
            for link in links:
                lp = repo / link
                if not lp.exists():
                    add_issue(f"{p}: lien keyword introuvable: {link}")
                else:
                    # Bidirectional chapter -> keyword -> chapter.
                    kw_text = lp.read_text(encoding='utf-8')
                    chap_ref = f'docs/book/chapters/{p.name}'
                    if chap_ref not in kw_text:
                        add_issue(f"{p}: lien non bidirectionnel avec {link} (chapitre absent de 'Utilisé dans les chapitres')", strict_only=True)

    if n is not None and n >= 1 and n % 3 == 0 and '## Checkpoint synthèse' not in t:
        add_issue(f"{p}: checkpoint manquant pour chapitre multiple de 3")

    if n is not None and 21 <= n <= 26 and '## Table erreur -> diagnostic -> correction' not in t:
        add_issue(f"{p}: table standard projet manquante")

    check_links(p, lines)
    repetition_check(p, lines)

# Keyword checks.
skip_kw = {'README.md', 'couverture.md', 'parcours.md', 'packs-apprentissage.md', 'non-utilises.md', 'erreurs-compilateur.md'}
for p in keywords:
    if p.name in skip_kw:
        continue

    t = p.read_text(encoding='utf-8')
    lines = t.splitlines()

    if not level_re.search(t):
        add_issue(f"{p}: Niveau manquant ou invalide (Débutant/Intermédiaire/Avancé)")

    for sec in required_keyword_sections:
        if sec not in t:
            add_issue(f"{p}: section manquante: {sec}")

    if 'docs/book/keywords/erreurs-compilateur.md' not in t:
        add_issue(f"{p}: référence manquante vers erreurs-compilateur.md")

    code_blocks = t.count('```vit')
    if code_blocks < 2:
        add_issue(f"{p}: exemples insuffisants (au moins un nominal et un invalide)")

    if 'Entrée:' not in t or 'Sortie observable:' not in t:
        add_issue(f"{p}: format exemple incomplet (Entrée/Sortie observable)")

    # Bidirectional keyword -> chapters and lexical presence.
    m_used = re.search(r'## Utilisé dans les chapitres\n\n(.*?)(?=\n## Voir aussi)', t, flags=re.DOTALL)
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
            kw_ref = f'docs/book/keywords/{kw}.md'
            if not (word_re.search(ch_text) or kw_ref in ch_text):
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
