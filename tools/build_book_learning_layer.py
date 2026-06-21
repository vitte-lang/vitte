#!/usr/bin/env python3
from pathlib import Path
import json
import re
from html import escape

ROOT = Path(__file__).resolve().parents[1]
DOCS = ROOT / 'docs'
BOOK = DOCS / 'book'

DOCS.mkdir(parents=True, exist_ok=True)
BOOK.mkdir(parents=True, exist_ok=True)
CHAPTERS = sorted(
    list((BOOK / 'chapters').glob('*.html'))
    + list((BOOK / 'poche').glob('*.html'))
)

READING_WPM = 180
CODE_BLOCK_WEIGHT = 0.75
DEFAULT_REVIEW_DATE = '2026-05-09'


def relative_prefix(path: Path) -> str:
    rel = path.relative_to(DOCS)
    return "../" * (len(rel.parts) - 1)


def doc_ref_link(ref: str, prefix: str) -> str:
    if not ref:
        return "<code>none</code>"
    target = ref.replace("\\", "/")
    if target.startswith("docs/"):
        target = target[len("docs/") :]
    return f'<a href="{prefix}{target}"><code>{target}</code></a>'


def extract_text(html: str) -> str:
    text = re.sub(r'<[^>]+>', ' ', html)
    return re.sub(r'\s+', ' ', text).strip()


def estimate_reading_time(html: str) -> int:
    text = extract_text(html)
    words = max(1, len(text.split()))
    code_blocks = len(re.findall(r'<pre\b', html, re.I))
    return max(
        3,
        round(words / READING_WPM + code_blocks * CODE_BLOCK_WEIGHT),
    )


def chapter_score(html: str) -> int:
    score = 0
    score += len(re.findall(r'<h2\b', html, re.I)) * 3
    score += len(re.findall(r'<h3\b', html, re.I))
    score += len(re.findall(r'<pre\b', html, re.I)) * 5
    score += len(re.findall(r'<table\b', html, re.I)) * 4
    score += len(re.findall(r'FAQ', html, re.I)) * 10
    score += len(re.findall(r'Exercises', html, re.I)) * 8
    score += len(re.findall(r'Key Concepts', html, re.I)) * 8
    score += len(re.findall(r'Architecture Overview', html, re.I)) * 10
    return min(score, 100)
def generate_concepts(family: str) -> str:
    concepts = {
        'async': [
            'Future',
            'Executor',
            'Channel',
            'Task Scheduling',
        ],
        'collections': [
            'Map',
            'Filter',
            'Reduce',
            'Iteration',
        ],
        'graph': [
            'Vertex',
            'Edge',
            'BFS',
            'DFS',
            'Dijkstra',
        ],
        'filesystem': [
            'Path',
            'Directory',
            'File',
            'Metadata',
        ],
        'network': [
            'Socket',
            'Client',
            'Server',
            'Protocol',
        ],
    }

    items = ''.join(
        f'<li>{escape(c)}</li>'
        for c in concepts.get(family, ['Core Concept'])
    )

    return (
        '<section class="chapter-concepts">'
        '<h2>Key Concepts</h2>'
        f'<ul>{items}</ul>'
        '</section>'
    )

def generate_common_mistakes(family: str) -> str:
    mistakes = {
        'async': [
            'Blocking inside asynchronous tasks.',
            'Awaiting the wrong future.',
            'Ignoring task failures.',
        ],
        'collections': [
            'Using map when filter is required.',
            'Copying large collections unnecessarily.',
        ],
        'graph': [
            'Using DFS when shortest path is needed.',
            'Ignoring graph density.',
        ],
    }

    items = ''.join(
        f'<li>{escape(m)}</li>'
        for m in mistakes.get(family, ['Review the API contract carefully.'])
    )

    return (
        '<section class="chapter-mistakes">'
        '<h2>Common Mistakes</h2>'
        f'<ul>{items}</ul>'
        '</section>'
    )

def generate_next_chapter(current_index: int, meta: list, prefix: str) -> str:
    if current_index + 1 >= len(meta):
        return ''

    nxt = meta[current_index + 1]

    return (
        '<section class="chapter-next">'
        '<h2>Next Chapter</h2>'
        f'<p><a href="{prefix}{nxt["path"]}">{escape(nxt["title"])}</a></p>'
        '</section>'
    )


def detect_family(name: str, text: str) -> str:
    low = (name + ' ' + text).lower()

    if 'async' in low:
        return 'async'
    if 'collection' in low:
        return 'collections'
    if 'graph' in low:
        return 'graph'
    if 'filesystem' in low or 'directory' in low:
        return 'filesystem'
    if 'network' in low or 'socket' in low:
        return 'network'

    return 'general'


def family_diagram(family: str) -> str:
    diagrams = {
        'async': (
            'Task\n'
            ' |\n'
            ' v\n'
            'Future\n'
            ' |\n'
            ' v\n'
            'Executor\n'
            ' |\n'
            ' +--> Worker\n'
            ' |\n'
            ' v\n'
            'Result'
        ),
        'collections': (
            'Input\n'
            ' |\n'
            ' v\n'
            'Filter\n'
            ' |\n'
            ' v\n'
            'Map\n'
            ' |\n'
            ' v\n'
            'Reduce\n'
            ' |\n'
            ' v\n'
            'Output'
        ),
        'graph': (
            'Graph\n'
            ' |\n'
            ' +--> BFS\n'
            ' |\n'
            ' +--> DFS\n'
            ' |\n'
            ' +--> Dijkstra\n'
            ' |\n'
            ' v\n'
            'Result'
        ),
        'filesystem': (
            'Path\n'
            ' |\n'
            ' v\n'
            'Directory\n'
            ' |\n'
            ' v\n'
            'File\n'
            ' |\n'
            ' v\n'
            'Storage'
        ),
        'network': (
            'Client\n'
            ' |\n'
            ' v\n'
            'Socket\n'
            ' |\n'
            ' v\n'
            'Protocol\n'
            ' |\n'
            ' v\n'
            'Server'
        ),
    }

    return diagrams.get(family, 'Concept -> Example -> Practice')

# ---- Contextual pedagogical content ----
def generate_examples(family: str) -> str:
    examples = {
        'async': '''space demo
use vitte/stdlib/async

proc main() -> int {
    let fut = async(work)
    let value = await(fut)
    give value
}''',
        'collections': '''let values = [1, 2, 3]
let doubled = map(values)''',
        'graph': '''let graph = graph_new()
let path = bfs(graph, 0)''',
        'filesystem': '''let text = read_file("hello.txt")''',
        'network': '''let client = connect("127.0.0.1", 8080)''',
    }

    code = examples.get(family, 'let value = 42')

    return (
        '<section class="chapter-example">'
        '<h2>Example</h2>'
        f'<pre><code class="language-vit">{escape(code)}</code></pre>'
        '</section>'
    )


def generate_faq(family: str) -> str:
    questions = {
        'async': [
            'What is a Future?',
            'What is an Executor?',
            'When should I use channels?',
        ],
        'collections': [
            'When should I use map?',
            'What is reduce?',
        ],
        'graph': [
            'When should I use BFS?',
            'When should I use DFS?',
        ],
    }

    items = ''.join(
        f'<li>{escape(q)}</li>'
        for q in questions.get(family, ['What should I learn next?'])
    )

    return (
        '<section class="chapter-faq">'
        '<h2>FAQ</h2>'
        f'<ul>{items}</ul>'
        '</section>'
    )


def generate_exercises(level: str, family: str) -> str:
    if level == 'beginner':
        tasks = [
            'Create a small example.',
            'Modify one value.',
            'Run the code.',
        ]
    elif level == 'advanced':
        tasks = [
            'Implement a complete feature.',
            'Benchmark the implementation.',
            'Document tradeoffs.',
        ]
    else:
        tasks = [
            'Extend the example.',
            'Add error handling.',
            'Write tests.',
        ]

    items = ''.join(f'<li>{escape(t)}</li>' for t in tasks)

    return (
        '<section class="chapter-exercises">'
        '<h2>Exercises</h2>'
        f'<ol>{items}</ol>'
        '</section>'
    )


def chapter_quality_report(html: str) -> str:
    issues = []

    if '<pre' not in html:
        issues.append('No code examples detected.')

    if 'FAQ' not in html:
        issues.append('No FAQ section detected.')

    if '<table' not in html:
        issues.append('No tables detected.')

    if not issues:
        issues.append('Chapter passes all quality checks.')

    items = ''.join(f'<li>{escape(i)}</li>' for i in issues)

    return (
        '<section class="chapter-quality">'
        '<h2>Quality Report</h2>'
        f'<ul>{items}</ul>'
        '</section>'
    )

def chapter_health_metrics(html: str) -> dict:
    return {
        'has_examples': '<pre' in html,
        'has_tables': '<table' in html,
        'has_faq': 'FAQ' in html,
        'has_h2': '<h2' in html,
        'score': chapter_score(html),
    }


def level_from_name(name: str) -> str:
    m = re.match(r'(\d+)', name)
    if not m:
        return 'intermediate'
    n = int(m.group(1))
    if n <= 20:
        return 'beginner'
    if n <= 50:
        return 'intermediate'
    return 'advanced'


def tags_for(name: str, text: str):
    t = []
    low = (name + ' ' + text).lower()
    for k in [
        'syntax',
        'types',
        'errors',
        'compiler',
        'modules',
        'tests',
        'grammar',
        'performance',
        'diagnostics',
        'stdlib',
    ]:
        if k in low:
            t.append(k)
    return t[:5] or ['general']


def build_toc(html: str) -> str:
    headings = re.findall(r'<h2[^>]*>(.*?)</h2>', html, re.I | re.S)

    if not headings:
        return ''

    items = ''.join(
        f'<li>{escape(re.sub(r"<[^>]+>", "", h).strip())}</li>'
        for h in headings
    )

    return (
        '<section class="chapter-toc">'
        '<h2>Contents</h2>'
        f'<ul>{items}</ul>'
        '</section>'
    )


book_health = []

meta = []
for p in CHAPTERS:
    s = p.read_text(encoding='utf-8', errors='ignore')
    title_match = re.search(r'<h1[^>]*>(.*?)</h1>', s, re.S)
    title = (
        re.sub(r'<[^>]+>', '', title_match.group(1)).strip()
        if title_match
        else p.stem
    )
    text = extract_text(s)
    level = level_from_name(p.name)
    minutes = estimate_reading_time(s)
    tags = tags_for(p.name, text)
    prereq = '' if not meta else meta[-1]['path']
    essential = level == 'beginner'
    family = detect_family(p.name, text)
    meta.append(
        {
            'path': str(p.relative_to(DOCS)).replace('\\', '/'),
            'title': title,
            'level': level,
            'minutes': minutes,
            'tags': tags,
            'prereq': prereq,
            'essential': essential,
            'maturity': 'reviewed' if essential else 'draft',
            'last_review': DEFAULT_REVIEW_DATE,
            'family': family,
        }
    )

BOOK.mkdir(parents=True, exist_ok=True)
(DOCS / 'book' / 'chapters-meta.json').write_text(
    json.dumps({'version': 'v1', 'chapters': meta}, ensure_ascii=False, indent=2)
    + '\n',
    encoding='utf-8',
)

# inject lightweight learning blocks
for i, p in enumerate(CHAPTERS):
    s = p.read_text(encoding='utf-8', errors='ignore')
    rel = str(p.relative_to(DOCS)).replace('\\', '/')
    m = next(x for x in meta if x['path'] == rel)
    if 'data-chapter-meta=' in s:
        continue
    prefix = relative_prefix(p)

    tldr = (
        '<section class="chapter-tldr">'
        '<h2>TL;DR (5 lines)</h2>'
        '<ul>'
        '<li>Read the core idea first.</li>'
        '<li>Understand one concept at a time.</li>'
        '<li>Run small examples.</li>'
        '<li>Fix errors early.</li>'
        '<li>Move to next chapter only when clear.</li>'
        '</ul>'
        '</section>'
    )

    note = (
        '<section class="chapter-note">'
        '<h2>Note</h2>'
        '<p>Remember to take breaks and revisit difficult concepts.</p>'
        '</section>'
    )

    performance_tip = (
        '<section class="chapter-performance">'
        '<h2>Performance Tip</h2>'
        '<p>Optimize code by focusing on critical paths and avoiding premature optimization.</p>'
        '</section>'
    )

    quiz = (
        '<section class="chapter-quiz">'
        '<h2>Mini quiz</h2>'
        '<ol>'
        '<li>What is the main goal of this chapter?</li>'
        '<li>Which concept is most important?</li>'
        '<li>What will you try right now?</li>'
        '</ol>'
        '<button class="quiz-save" type="button">Mark as reviewed</button>'
        '</section>'
    )

    cta = (
        '<section class="chapter-cta">'
        '<h2>Next best action</h2>'
        '<p>Apply one idea from this chapter in a tiny example, then continue.</p>'
        '</section>'
    )

    learning_score = (
        f'<section class="chapter-score">'
        f'<h2>Learning Score</h2>'
        f'<p>Score: <strong>{chapter_score(s)}</strong> / 100</p>'
        '</section>'
    )

    badge = (
        f'<p class="chapter-meta" data-chapter-meta="1">'
        f'Level: <strong>{m["level"]}</strong> · '
        f'Reading time: <strong>{m["minutes"]} min</strong> · '
        f'Prerequisite: {doc_ref_link(m["prereq"], prefix)} · '
        f'Last review: <strong>{m["last_review"]}</strong>'
        f'</p>'
    )

    toc = build_toc(s)

    family = m['family']
    diagram = (
        '<section class="chapter-diagram">'
        '<h2>Architecture Overview</h2>'
        f'<pre>{family_diagram(family)}</pre>'
        '</section>'
    )

    # Contextual pedagogical content
    example_section = generate_examples(family)
    faq_section = generate_faq(family)
    exercise_section = generate_exercises(m['level'], family)
    quality_section = chapter_quality_report(s)
    book_health.append(
        {
            'path': rel,
            'title': m['title'],
            **chapter_health_metrics(s),
        }
    )
    concepts_section = generate_concepts(family)
    mistakes_section = generate_common_mistakes(family)
    next_section = generate_next_chapter(i, meta, prefix)

    # insert after first h1
    s = re.sub(
        r'(</h1>)',
        r'\1\n' + badge + toc + tldr + note + performance_tip,
        s,
        count=1,
    )

    # add see also from nearby chapters
    also = []
    for j in [i - 1, i + 1, i + 2]:
        if 0 <= j < len(meta):
            also.append(meta[j])
    also_html = ''.join(
        f'<li><a href="{prefix}{a["path"]}">{a["title"]}</a></li>' for a in also[:3]
    )
    see = f'<section class="chapter-see-also"><h2>See also</h2><ul>{also_html}</ul></section>'

    s = s.replace(
        '</article>',
        diagram
        + concepts_section
        + example_section
        + mistakes_section
        + faq_section
        + exercise_section
        + quality_section
        + quiz
        + see
        + next_section
        + cta
        + learning_score
        + '</article>',
    )

    p.write_text(s, encoding='utf-8')


# Write learning-path.json after chapters-meta.json
learning_path = {
    'beginner': [x['path'] for x in meta if x['level'] == 'beginner'],
    'intermediate': [x['path'] for x in meta if x['level'] == 'intermediate'],
    'advanced': [x['path'] for x in meta if x['level'] == 'advanced'],
}

BOOK.mkdir(parents=True, exist_ok=True)
(DOCS / 'book' / 'learning-path.json').write_text(
    json.dumps(learning_path, ensure_ascii=False, indent=2) + '\n',
    encoding='utf-8',
)

BOOK.mkdir(parents=True, exist_ok=True)
(DOCS / 'book' / 'book-health.json').write_text(
    json.dumps(book_health, ensure_ascii=False, indent=2) + '\n',
    encoding='utf-8',
)

average_score = (
    round(sum(x['score'] for x in book_health) / max(1, len(book_health)), 1)
)

table_rows = []
for ch in book_health:
    table_rows.append(
        '<tr>'
        f'<td>{escape(ch["title"])}</td>'
        f'<td>{ch["score"]}</td>'
        f'<td>{"Yes" if ch["has_examples"] else "No"}</td>'
        f'<td>{"Yes" if ch["has_faq"] else "No"}</td>'
        f'<td>{"Yes" if ch["has_tables"] else "No"}</td>'
        '</tr>'
    )

html_report = f'''<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8" />
<title>Book Health Report</title>
<style>
table {{
  border-collapse: collapse;
  width: 100%;
}}
th, td {{
  border: 1px solid #ccc;
  padding: 8px;
  text-align: left;
}}
th {{
  cursor: pointer;
  background-color: #f2f2f2;
}}
</style>
</head>
<body>
<h1>Book Health Report</h1>
<p>Average Score: <strong>{average_score}</strong> / 100</p>
<p>Total Chapters: <strong>{len(book_health)}</strong></p>
<table id="healthTable">
<thead>
<tr>
  <th>Title</th>
  <th>Score</th>
  <th>Examples</th>
  <th>FAQ</th>
  <th>Tables</th>
</tr>
</thead>
<tbody>
{''.join(table_rows)}
</tbody>
</table>
</body>
</html>
'''

BOOK.mkdir(parents=True, exist_ok=True)
(DOCS / 'book' / 'book-health.html').write_text(html_report, encoding='utf-8')

print(
    f'book learning layer built: '
    f'{len(meta)} chapters, '
    f'average score {average_score}/100'
)
