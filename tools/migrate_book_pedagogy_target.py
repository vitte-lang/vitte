#!/usr/bin/env python3
from pathlib import Path
import argparse
import json
import re

ROOT = Path('docs/book')
SCOPE_FILE = Path('tools/book_pedagogy_scope.json')

def load_scope(phase_arg: str | None):
    data = json.loads(SCOPE_FILE.read_text(encoding='utf-8'))
    phase = phase_arg or data.get('default_phase', 'phase1')
    cfg = data['phases'].get(phase)
    if not cfg:
        raise SystemExit(f'unknown phase: {phase}')
    return phase, cfg

def target_files(include_patterns: list[str]):
    out: list[Path] = []
    for p in sorted(ROOT.rglob('*.html')):
        p2 = Path(p.as_posix())
        for pat in include_patterns:
            if p2.match(pat):
                out.append(p)
                break
    return out

TLDR = '<section class="chapter-tldr"><h2>TL;DR (5 lines)</h2><ul><li>Read the core idea first.</li><li>Understand one concept at a time.</li><li>Run a small example.</li><li>Fix one issue at a time.</li><li>Apply before moving on.</li></ul></section>'
CTA = '<section class="chapter-cta"><h2>Next best action</h2><p>Apply one key idea from this chapter in a tiny example, then continue.</p></section>'

INTRO_TEMPLATE = '''<section class="chapter-intro">
<h2>Introduction</h2>
<p>This section introduces the main topics covered in this chapter and provides context for the reader.</p>
</section>'''

CONCEPTS_TEMPLATE = '''<section class="chapter-concepts">
<h2>Key Concepts</h2>
<ul>
<li>Concept 1: Description of the concept.</li>
<li>Concept 2: Description of the concept.</li>
<li>Concept 3: Description of the concept.</li>
</ul>
</section>'''

MISTAKES_TEMPLATE = '''<section class="chapter-mistakes">
<h2>Common Mistakes</h2>
<p>Here we discuss frequent errors and misconceptions related to the chapter's topics.</p>
</section>'''

FAQ_TEMPLATE = '''<section class="chapter-faq">
<h2>Frequently Asked Questions</h2>
<dl>
{faq_items}
</dl>
</section>'''

TOC_TEMPLATE = '''<nav class="chapter-toc">
<ul>
{items}
</ul>
</nav>'''

NOTE_TEMPLATE = '''<section class="callout-note">
<h3>Note</h3>
<p>This is an important note to keep in mind.</p>
</section>'''

WARNING_TEMPLATE = '''<section class="callout-warning">
<h3>Warning</h3>
<p>Be careful about this common pitfall.</p>
</section>'''

PERF_TEMPLATE = '''<section class="callout-performance">
<h3>Performance Tip</h3>
<p>Consider this advice to improve performance.</p>
</section>'''

ASYNC_INTRO_TEMPLATE = '''<section class="chapter-intro">
<h2>Async Module Introduction</h2>
<p>This module covers asynchronous programming concepts such as tasks, futures, and executors.</p>
</section>'''

COLLECTIONS_INTRO_TEMPLATE = '''<section class="chapter-intro">
<h2>Collections Module Introduction</h2>
<p>This module deals with data processing pipelines including filtering, mapping, and reducing collections.</p>
</section>'''

GRAPH_INTRO_TEMPLATE = '''<section class="chapter-intro">
<h2>Graph Module Introduction</h2>
<p>This module introduces graph algorithms and traversals, including BFS, DFS, and shortest path computations.</p>
</section>'''

FS_INTRO_TEMPLATE = '''<section class="chapter-intro">
<h2>Filesystem Module Introduction</h2>
<p>This module explains filesystem abstractions including paths, directories, files, and storage operations.</p>
</section>'''

NET_INTRO_TEMPLATE = '''<section class="chapter-intro">
<h2>Network Module Introduction</h2>
<p>This module covers networking concepts such as clients, sockets, protocols, and servers.</p>
</section>'''

def pedagogical_score(html: str) -> int:
    score = 0
    score += len(re.findall(r'<h2\b', html, re.I)) * 3
    score += len(re.findall(r'<pre\b', html, re.I)) * 5
    score += len(re.findall(r'<table\b', html, re.I)) * 4
    score += len(re.findall(r'FAQ', html, re.I)) * 10
    score += len(re.findall(r'example', html, re.I))
    return min(score, 100)

def looks_like_api_dump(html: str) -> bool:
    markers = [
        'Declaration map',
        'Representative signatures',
        'Complete API catalog',
        'Top-level API inventory',
        'Implementation profile',
        'Keyword coverage',
    ]
    hits = sum(1 for m in markers if m.lower() in html.lower())
    return hits >= 2

def insert_toc_after_h1(html: str) -> str:
    h1_match = re.search(r'<h1\b[^>]*>.*?</h1>', html, re.I|re.S)
    if not h1_match:
        return html
    # collect all h2 headings
    headings = re.findall(r'<h2\b[^>]*>(.*?)</h2>', html, re.I|re.S)
    if not headings:
        return html
    items = ''.join(f'<li>{h.strip()}</li>' for h in headings)
    toc = TOC_TEMPLATE.format(items=items)
    insert_pos = h1_match.end()
    return html[:insert_pos] + '\n' + toc + html[insert_pos:]

def inject_callouts(html: str) -> str:
    # Check if callouts already present
    if 'callout-note' not in html:
        # Insert Note before first major h2
        h2_match = re.search(r'<h2\b', html, re.I)
        if h2_match:
            pos = h2_match.start()
            html = html[:pos] + NOTE_TEMPLATE + '\n' + html[pos:]
        else:
            # Append at end of article if no h2
            article_end = re.search(r'</article>', html, re.I)
            if article_end:
                pos = article_end.start()
                html = html[:pos] + NOTE_TEMPLATE + '\n' + html[pos:]
    if 'callout-warning' not in html:
        h2_match = re.search(r'<h2\b', html, re.I)
        if h2_match:
            pos = h2_match.start()
            html = html[:pos] + WARNING_TEMPLATE + '\n' + html[pos:]
        else:
            article_end = re.search(r'</article>', html, re.I)
            if article_end:
                pos = article_end.start()
                html = html[:pos] + WARNING_TEMPLATE + '\n' + html[pos:]
    if 'callout-performance' not in html:
        h2_match = re.search(r'<h2\b', html, re.I)
        if h2_match:
            pos = h2_match.start()
            html = html[:pos] + PERF_TEMPLATE + '\n' + html[pos:]
        else:
            article_end = re.search(r'</article>', html, re.I)
            if article_end:
                pos = article_end.start()
                html = html[:pos] + PERF_TEMPLATE + '\n' + html[pos:]
    return html

def inject_pedagogical_sections(html: str) -> str:
    # Check which sections are missing
    has_intro = re.search(r'class=["\']chapter-intro["\']', html, re.I)
    has_concepts = re.search(r'class=["\']chapter-concepts["\']', html, re.I)
    has_faq = re.search(r'class=["\']chapter-faq["\']', html, re.I)
    has_mistakes = re.search(r'class=["\']chapter-mistakes["\']', html, re.I)
    insert_pos = None
    match = re.search(r'</article>', html, re.I)
    if match:
        insert_pos = match.start()
    else:
        insert_pos = len(html)
    additions = ''
    if not has_intro:
        additions += INTRO_TEMPLATE + '\n'
    if not has_concepts:
        additions += CONCEPTS_TEMPLATE + '\n'
    if not has_mistakes:
        additions += MISTAKES_TEMPLATE + '\n'
    if not has_faq:
        # Use generic FAQ with empty faq_items placeholder, will be replaced later
        additions += FAQ_TEMPLATE.format(faq_items='') + '\n'
    if additions:
        html = html[:insert_pos] + additions + html[insert_pos:]
    return html

def detect_module_family(module_name: str) -> str:
    name = module_name.lower()

    if 'async' in name:
        return 'async'
    if 'collection' in name:
        return 'collections'
    if 'graph' in name:
        return 'graph'
    if 'filesystem' in name or '/fs' in name or name.startswith('fs'):
        return 'fs'
    if 'network' in name or name.startswith('net'):
        return 'net'

    return 'generic'

def family_intro_template(family: str) -> str:
    if family == 'async':
        return ASYNC_INTRO_TEMPLATE
    if family == 'collections':
        return COLLECTIONS_INTRO_TEMPLATE
    if family == 'graph':
        return GRAPH_INTRO_TEMPLATE
    if family == 'fs':
        return FS_INTRO_TEMPLATE
    if family == 'net':
        return NET_INTRO_TEMPLATE
    return INTRO_TEMPLATE

def inject_family_content(html: str, module_name: str) -> str:
    family = detect_module_family(module_name)

    # Inject family intro before first <h2>
    intro_html = family_intro_template(family)
    h2_match = re.search(r'<h2\b', html, re.I)
    if h2_match:
        pos = h2_match.start()
        html = html[:pos] + intro_html + '\n' + html[pos:]
    else:
        # fallback: insert before </article> or at end
        article_end = re.search(r'</article>', html, re.I)
        if article_end:
            pos = article_end.start()
            html = html[:pos] + intro_html + '\n' + html[pos:]
        else:
            html += intro_html + '\n'

    # Prepare family-specific architecture diagrams
    diagram_map = {
        'async': '''Task
 |
 v
Future
 |
 v
Executor
 |
 +--> Worker 1
 +--> Worker 2
 +--> Worker N
 |
 v
Result''',
        'collections': '''Input
 |
 v
Filter
 |
 v
Map
 |
 v
Reduce
 |
 v
Output''',
        'graph': '''Graph
 |
 +--> BFS
 +--> DFS
 +--> Dijkstra
 +--> MST
 |
 v
Result''',
        'fs': '''Path
 |
 v
Directory
 |
 v
File
 |
 v
Read / Write
 |
 v
Storage''',
        'net': '''Client
 |
 v
Socket
 |
 v
Protocol
 |
 v
Server''',
    }

    diagram_text = diagram_map.get(family)
    if not diagram_text:
        return html

    pre_block = f'''<section class="chapter-diagram">
<h2>Architecture Overview</h2>
<pre>{diagram_text}</pre>
</section>
'''

    # Insert before FAQ if present
    faq_match = re.search(r'(<section\b[^>]*class=["\'][^"\']*chapter-faq[^"\']*["\'][^>]*>)', html, re.I)
    if faq_match:
        pos = faq_match.start()
        html = html[:pos] + pre_block + html[pos:]
    else:
        # Insert before </article> or at end
        article_end = re.search(r'</article>', html, re.I)
        if article_end:
            pos = article_end.start()
            html = html[:pos] + pre_block + html[pos:]
        else:
            html += pre_block

    # Replace FAQ content with family-specific FAQ entries
    faq_entries_map = {
        'async': [
            ('What is a Future?', 'A Future represents a value which may not be available yet.'),
            ('What is an Executor?', 'An Executor runs asynchronous tasks.'),
            ('When should I use channels?', 'Use channels to communicate between tasks safely.'),
        ],
        'collections': [
            ('What is filter?', 'Filter selects elements based on a predicate.'),
            ('What is reduce?', 'Reduce aggregates elements into a single value.'),
            ('When should I sort?', 'Sort when order matters in your collection.'),
        ],
        'graph': [
            ('BFS vs DFS?', 'BFS explores neighbors level by level; DFS dives deep first.'),
            ('When should I use Dijkstra?', 'Use Dijkstra to find shortest paths in weighted graphs.'),
        ],
        'fs': [
            ('Difference between path and file?', 'A path is a location; a file is the data at that location.'),
        ],
        'net': [
            ('TCP vs UDP?', 'TCP is connection-oriented; UDP is connectionless and faster.'),
        ],
        'generic': [],
    }

    faq_entries = faq_entries_map.get(family, [])

    faq_dl_items = ''
    for q, a in faq_entries:
        faq_dl_items += f'<dt>{q}</dt><dd>{a}</dd>\n'

    # Replace FAQ section content
    def replace_faq_content(match):
        # Replace <dl>...</dl> content with generated faq_dl_items
        faq_html = match.group(0)
        new_faq_html = re.sub(r'(<dl>).*?(</dl>)', rf'\1\n{faq_dl_items}\2', faq_html, flags=re.S)
        return new_faq_html

    html = re.sub(r'(<section\b[^>]*class=["\'][^"\']*chapter-faq[^"\']*["\'][^>]*>.*?</section>)', replace_faq_content, html, flags=re.I|re.S)

    return html

def ensure_viewport(html: str) -> str:
    if re.search(r'<meta\s+name=["\']viewport["\']', html, re.I):
        return html

    charset_match = re.search(r'<meta\s+charset=["\']utf-8["\']\s*/?>', html, re.I)
    viewport = '\n<meta name="viewport" content="width=device-width, initial-scale=1">'

    if charset_match:
        end = charset_match.end()
        return html[:end] + viewport + html[end:]

    head_match = re.search(r'<head\b[^>]*>', html, re.I)
    if head_match:
        end = head_match.end()
        return html[:end] + viewport + html[end:]

    return html


def ensure_single_h1(html: str, title: str) -> str:
    if len(re.findall(r'<h1\b', html, re.I)) > 0:
        return html

    article_match = re.search(r'<article\b[^>]*class=["\'][^"\']*doc-content[^"\']*["\'][^>]*>', html, re.I)
    if article_match:
        end = article_match.end()
        return html[:end] + f'\n<h1>{title}</h1>' + html[end:]

    return re.sub(
        r'(<main\b[^>]*>)',
        rf'\1\n<h1>{title}</h1>',
        html,
        count=1,
        flags=re.I,
    )

ap = argparse.ArgumentParser()
ap.add_argument('--phase', choices=['phase1', 'phase2', 'phase3'])
args = ap.parse_args()
phase_name, phase_cfg = load_scope(args.phase)

for p in target_files(phase_cfg.get('include', [])):
    s = p.read_text(encoding='utf-8', errors='ignore')
    if re.search(r'<meta[^>]+http-equiv=["\']refresh["\']', s, re.I):
        continue
    s = ensure_single_h1(s, p.stem)
    s = ensure_viewport(s)
    # inject TLDR after first h1
    if 'chapter-tldr' not in s and 'TL;DR' not in s:
        if re.search(r'</h1>', s, re.I):
            s = re.sub(r'(</h1>)', r'\1\n' + TLDR, s, count=1, flags=re.I)
        elif re.search(r'<main\b[^>]*>', s, re.I):
            s = re.sub(r'(<main\b[^>]*>)', r'\1\n' + TLDR, s, count=1, flags=re.I)
        else:
            s = s.replace('</body>', TLDR + '\n</body>')
    # inject CTA before end of article
    if 'chapter-cta' not in s and 'Next best action' not in s:
        if '</article>' in s:
            s = s.replace('</article>', CTA + '\n</article>')
        elif re.search(r'</main>', s, re.I):
            s = re.sub(r'</main>', CTA + '\n</main>', s, count=1, flags=re.I)
        else:
            s = s.replace('</body>', CTA + '\n</body>')
    # ensure img alt
    s = re.sub(r'<img(?![^>]*\salt=)([^>]*)>', r'<img alt=""\1>', s, flags=re.I)

    # New pedagogical processing
    if looks_like_api_dump(s):
        s = inject_pedagogical_sections(s)
        s = inject_family_content(s, p.stem)
    s = inject_callouts(s)
    s = insert_toc_after_h1(s)
    score = pedagogical_score(s)
    # inject pedagogy score before </article>
    pedagogy_score_html = f'<section class="pedagogy-score">\n<h2>Learning Score</h2>\n<p>Pedagogical score: {score}/100</p>\n</section>\n'
    if '</article>' in s:
        s = s.replace('</article>', pedagogy_score_html + '</article>')
    else:
        s += pedagogy_score_html

    s = re.sub(r'\n{3,}', '\n\n', s)
    p.write_text(s, encoding='utf-8')

print(f'migrated pedagogy target subset ({phase_name})')
