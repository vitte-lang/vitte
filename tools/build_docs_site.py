#!/usr/bin/env python3
#
# Post-process generated docs HTML.
# Source HTML is rendered elsewhere; this script normalizes the shared shell,
# injects the canonical docs JS/CSS references, and regenerates search/status artifacts.
from __future__ import annotations
import hashlib
import html
import json
import re
import shutil
import urllib.parse
import urllib.request
from collections import defaultdict
from html.parser import HTMLParser
from datetime import datetime, timezone
from pathlib import Path

DOCS=Path('docs')
LANGS=[]
PAGES=sorted([p for p in DOCS.glob('*.html') if p.name!='status.html'])
ALL_HTML=sorted([p for p in DOCS.rglob('*.html') if p.name != 'status.html'])
HEADER_RE=re.compile(r'<header class="site-header">[\s\S]*?</header>')
FOOTER_RE=re.compile(r'<footer class="site-footer">[\s\S]*?</footer>')
SCRIPT_RE=re.compile(r'<script[^>]*site-interactions\.js[^>]*></script>\n?', re.I)
HEAD_RE=re.compile(r'</head>')
BODY_END_RE=re.compile(r'</body>')
MAIN_JS_RE=re.compile(r'<script[^>]+src="js/main\.js(?:\?v=[^"]+)?"[^>]*></script>\s*', re.I)
ANY_MAIN_JS_RE=re.compile(r'<script[^>]+src="(?:/)?js/main\.js(?:\?v=[^"]+)?"[^>]*></script>\s*', re.I)
BROKEN_SRI_TAIL_RE=re.compile(r'</script>\s*integrity="[^"]+"\s*crossorigin="anonymous">', re.I)
ORPHAN_MAIN_LINE_RE=re.compile(r'^\s*<script[^>]+src="(?:/)?js/main\.js(?:\?v=[^"]+)?"[^>]*></script>\s*integrity="[^"]+"\s*crossorigin="anonymous">\s*$', re.I | re.M)
MAIN_JS_BROKEN_BLOCK_RE=re.compile(
  r'<script[^>]+src="(?:/)?js/main\.js(?:\?v=[^"]+)?"[^>]*></script>\s*integrity="[^"]+"\s*crossorigin="anonymous">\s*',
  re.I
)
ANY_MAIN_JS_TAG_RE=re.compile(
  r'<script[^>]+src="(?:/)?js/main\.js(?:\?v=[^"]+)?"[^>]*>\s*</script>\s*(?:integrity="[^"]+"\s*crossorigin="anonymous">)?\s*',
  re.I
)
ANY_MAIN_JS_LINE_RE=re.compile(
  r'^\s*<script[^\n]*src="(?:/)?js/main\.js(?:\?v=[^"]+)?"[^\n]*\n?',
  re.I | re.M
)
ARTICLE_RE=re.compile(r'<article class="doc-content">([\s\S]*?)</article>')
TITLE_RE=re.compile(r'<title>(.*?)</title>')
TAG_RE=re.compile(r'<[^>]+>')
DESCRIPTION_RE=re.compile(r'<meta name="description" content="([^"]*)">', re.I)

# -----------------------------------------------------------------------------
# Build configuration
# -----------------------------------------------------------------------------

SEARCH_CONTENT_LIMIT = 5000
SEARCH_VERSION = "v5"
STATUS_VERSION = "v5"

SECTION_DOCS = "docs"
SECTION_BOOK = "book"
SECTION_GRAMMAR = "grammar"

PEDAGOGY_VERSION = "v1"
CONCEPT_REPORT_VERSION = "v1"
GLOSSARY_REPORT_VERSION = "v1"
MAX_KEYWORDS = 25

PEDAGOGY_KEYWORDS = [
    'syntax', 'type', 'types', 'module', 'modules', 'compiler',
    'lexer', 'parser', 'ast', 'hir', 'sema', 'typeck',
    'borrowck', 'mir', 'ir', 'backend', 'runtime',
    'async', 'future', 'executor', 'channel', 'task',
    'filesystem', 'network', 'collections', 'graph'
]

NAV=[('Welcome','index.html','i-home'),('Documentation','doc.html','i-docs'),('Visual Preview','social-preview.html','i-home'),('Download','download.html','i-docs'),('Source','source.html','i-docs'),('Community','community.html','i-docs'),('News','news.html','i-news'),('Diagnostics','diagnostics.html','i-docs'),('Suggestions','suggestions.html','i-docs')]
CACHE={}

# -----------------------------------------------------------------------------
# Utility helpers
# -----------------------------------------------------------------------------

def clean_text(value: str) -> str:
    value = TAG_RE.sub(' ', value)
    value = re.sub(r'\s+', ' ', value)
    return value.strip()


def extract_title(html_source: str, fallback: str) -> str:
    match = TITLE_RE.search(html_source)
    if match:
        return match.group(1).strip()
    return fallback


def extract_description(html_source: str, fallback: str) -> str:
    match = DESCRIPTION_RE.search(html_source)
    if match:
        return html.unescape(match.group(1).strip())
    return fallback


def extract_article_text(html_source: str) -> str:
    match = ARTICLE_RE.search(html_source)
    if match:
        return clean_text(match.group(1))[:SEARCH_CONTENT_LIMIT]
    return clean_text(html_source)[:SEARCH_CONTENT_LIMIT]


def extract_keywords(content: str) -> list[str]:
    lowered = content.lower()
    found = []

    for keyword in PEDAGOGY_KEYWORDS:
        if keyword in lowered:
            found.append(keyword)

    return found[:MAX_KEYWORDS]


def estimate_reading_minutes(content: str) -> int:
    words = max(1, len(content.split()))
    return max(1, round(words / 180))


def estimate_difficulty(content: str) -> str:
    lowered = content.lower()

    advanced_hits = sum(
        1 for token in [
            'borrowck', 'hir', 'mir', 'ir',
            'codegen', 'backend', 'optimizer'
        ]
        if token in lowered
    )

    if advanced_hits >= 3:
        return 'advanced'

    if advanced_hits >= 1:
        return 'intermediate'

    return 'beginner'


def build_pedagogy_metadata(content: str) -> dict:
    return {
        'keywords': extract_keywords(content),
        'reading_minutes': estimate_reading_minutes(content),
        'difficulty': estimate_difficulty(content)
    }

def build_concept_index(search_entries: list[dict]) -> dict:
    concepts = defaultdict(list)

    for entry in search_entries:
        for keyword in entry.get('keywords', []):
            concepts[keyword].append({
                'title': entry['title'],
                'path': entry['path'],
                'difficulty': entry.get('difficulty', 'beginner')
            })

    return dict(sorted(concepts.items()))


def build_glossary(concept_index: dict) -> list[dict]:
    out = []

    for concept, refs in sorted(concept_index.items()):
        out.append({
            'term': concept,
            'references': len(refs),
            'pages': refs[:20]
        })

    return out

def text(x):
    return clean_text(x)

def sha(path): return hashlib.sha256(path.read_bytes()).hexdigest()[:10]
def relpath(p): return p.relative_to(DOCS).as_posix()
def infer_section(rel: str) -> str:
  if rel.startswith('grammar/'): return SECTION_GRAMMAR
  if rel.startswith('book/'): return SECTION_BOOK
  return SECTION_DOCS

def tr(txt: str, dst: str) -> str:
  k=(txt,dst)
  if k in CACHE: return CACHE[k]
  if dst=='en' or not txt.strip(): return txt
  try:
    params=urllib.parse.urlencode({'client':'gtx','sl':'en','tl':dst,'dt':'t','q':txt})
    data=json.loads(urllib.request.urlopen('https://translate.googleapis.com/translate_a/single?'+params,timeout=20).read().decode('utf-8'))
    out=''.join([c[0] for c in data[0] if c and c[0]])
    CACHE[k]=out
    return out
  except Exception:
    return txt

class Translator(HTMLParser):
  def __init__(self, lang):
    super().__init__(convert_charrefs=False)
    self.lang=lang; self.out=[]; self.skip=0
  def handle_starttag(self, tag, attrs):
    attrs_s=''.join([f' {k}' if v is None else f' {k}="{html.escape(v,quote=True)}"' for k,v in attrs])
    self.out.append(f'<{tag}{attrs_s}>')
    if tag.lower() in {'script','style','code','pre'}: self.skip+=1
  def handle_endtag(self, tag):
    self.out.append(f'</{tag}>')
    if tag.lower() in {'script','style','code','pre'} and self.skip: self.skip-=1
  def handle_startendtag(self, tag, attrs):
    attrs_s=''.join([f' {k}' if v is None else f' {k}="{html.escape(v,quote=True)}"' for k,v in attrs])
    self.out.append(f'<{tag}{attrs_s} />')
  def handle_data(self, data):
    if self.skip or not data.strip(): self.out.append(data); return
    lead=len(data)-len(data.lstrip()); tail=len(data)-len(data.rstrip())
    core=data.strip(); t=tr(core,self.lang)
    self.out.append(data[:lead]+t+data[len(data)-tail:])
  def handle_entityref(self,name): self.out.append(f'&{name};')
  def handle_charref(self,name): self.out.append(f'&#{name};')
  def handle_comment(self,data): self.out.append(f'<!--{data}-->')
  def render(self): return ''.join(self.out)

def translate_fragment(fragment, lang):
  if lang=='en': return fragment
  p=Translator(lang); p.feed(fragment); return p.render()

def search_form(prefix: str = '', lang: str = 'en') -> str:
  return (
    f'<form class="doc-search" role="search" action="{prefix}search.html" method="get">'
    f'<input class="doc-search-input" type="search" name="q" placeholder="{tr("Search docs, book, grammar",lang)}" aria-label="{tr("Search documentation",lang)}" autocomplete="off">'
    f'<div class="doc-search-controls" aria-label="{tr("Search filters",lang)}">'
    f'<select class="doc-search-filter doc-search-section" name="section" aria-label="{tr("Filter by section",lang)}">'
    f'<option value="all">{tr("All sections",lang)}</option>'
    f'<option value="docs">Docs</option>'
    f'<option value="book">Book</option>'
    f'<option value="grammar">Grammar</option>'
    f'</select>'
    f'<select class="doc-search-filter doc-search-lang" name="lang" aria-label="{tr("Filter by language",lang)}">'
    f'<option value="all">{tr("All languages",lang)}</option>'
    f'<option value="en">EN</option>'
    f'<option value="fr">FR</option>'
    f'</select>'
    f'</div>'
    f'<div class="doc-search-results" hidden></div><div class="doc-search-footer" hidden></div>'
    f'</form>'
  )

def header(lang='en'):
  items=''.join([f'<li><a class="nav-chip" href="{h}"><svg width="14" height="14" aria-hidden="true" focusable="false"><use href="svg/sprite.svg#{i}"></use></svg><span>{tr(l,lang)}</span></a></li>' for l,h,i in NAV])
  return (
    '<header class="site-header">'
    '<a class="site-brand" href="index.html"><img class="site-brand-mark" src="svg/logo.svg" alt="" width="32" height="32"><span>Vitte</span></a>'
    f'<nav class="site-nav" aria-label="Primary"><ul class="nav-band">{items}</ul></nav>'
    f'{search_form("", lang)}'
    '</header>'
  )

def footer(name, lang='en'):
  return (
    f'<footer class="site-footer">\n'
    f'<p class="site-footer-path">{name}</p>\n'
    f'<p><a href="index.html">{tr("Back to home",lang)}</a> · '
    f'<a href="status-public.html">{tr("Public status",lang)}</a></p>\n'
    f'</footer>'
  )

# -----------------------------------------------------------------------------
# Search index generation
# -----------------------------------------------------------------------------

def build_search_entry(path: Path, html_source: str) -> dict:
    rel = relpath(path)
    content = extract_article_text(html_source)
    pedagogy = build_pedagogy_metadata(content)

    return {
        'title': extract_title(html_source, path.stem),
        'path': rel,
        'content': content,
        'lang': 'en',
        'section': infer_section(rel),
        'keywords': pedagogy['keywords'],
        'reading_minutes': pedagogy['reading_minutes'],
        'difficulty': pedagogy['difficulty']
    }

hash_css=sha(DOCS/'css/site.css') if (DOCS/'css/site.css').exists() else 'dev'
hash_js=sha(DOCS/'js/main.js') if (DOCS/'js/main.js').exists() else 'dev'

search=[]
search_docs=[]
search_book=[]
search_grammar=[]
for idx,p in enumerate(PAGES):
  s=p.read_text(encoding='utf-8')
  s=HEADER_RE.sub(header('en'),s)
  s=FOOTER_RE.sub(footer(p.name,'en'),s)
  s=SCRIPT_RE.sub('',s)
  s=ANY_MAIN_JS_LINE_RE.sub('', s)
  s=MAIN_JS_BROKEN_BLOCK_RE.sub('', s)
  s=ANY_MAIN_JS_TAG_RE.sub('', s)
  s=BROKEN_SRI_TAIL_RE.sub('</script>', s)
  s=ORPHAN_MAIN_LINE_RE.sub('', s)
  s=MAIN_JS_RE.sub('',s)
  s=ANY_MAIN_JS_RE.sub('',s)
  title=(TITLE_RE.search(s).group(1) if TITLE_RE.search(s) else p.stem)
  description = extract_description(s, f'{title} on the Vitte documentation site.')
  canonical=f'https://vitte-lang.org/{p.name}'
  meta=(
    f'<link rel="canonical" href="{canonical}">\n'
    f'<meta property="og:title" content="{html.escape(title, quote=True)}">\n'
    f'<meta property="og:description" content="{html.escape(description, quote=True)}">\n'
    f'<meta property="og:type" content="website">\n'
    f'<meta property="og:url" content="{canonical}">\n'
    f'<meta property="og:image" content="https://vitte-lang.org/svg/readme-social-card.svg">\n'
    f'<meta property="og:image:type" content="image/svg+xml">\n'
    f'<meta property="og:image:width" content="1200">\n'
    f'<meta property="og:image:height" content="630">\n'
    f'<meta name="twitter:card" content="summary">\n'
    f'<meta name="twitter:title" content="{html.escape(title, quote=True)}">\n'
    f'<meta name="twitter:description" content="{html.escape(description, quote=True)}">\n'
    f'<meta name="twitter:image" content="https://vitte-lang.org/svg/readme-social-card.svg">\n'
  )
  if 'rel="canonical"' not in s: s=HEAD_RE.sub(meta+'</head>',s,1)
  s=BODY_END_RE.sub(f'<script type="module" src="js/main.js?v={hash_js}"></script>\n</body>',s,1)
  s=s.replace('href="css/site.css"', f'href="css/site.css?v={hash_css}"')
  prevp=PAGES[idx-1].name if idx>0 else None; nextp=PAGES[idx+1].name if idx+1<len(PAGES) else None
  pag='<nav class="doc-pagination">'+(f'<a href="{prevp}">← Previous</a>' if prevp else '<span></span>')+(f'<a href="{nextp}">Next →</a>' if nextp else '')+'</nav>'
  if 'doc-pagination' not in s: s=s.replace('</article>', pag+'</article>')
  p.write_text(s,encoding='utf-8')

for p in ALL_HTML:
  s=p.read_text(encoding='utf-8')
  item = build_search_entry(p, s)
  search.append(item)
  if item['section']==SECTION_DOCS: search_docs.append(item)
  elif item['section']==SECTION_BOOK: search_book.append(item)
  elif item['section']==SECTION_GRAMMAR: search_grammar.append(item)

(DOCS/'search-index.json').write_text(json.dumps({'version':SEARCH_VERSION,'pages':search},ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
(DOCS/'search-index.docs.json').write_text(json.dumps({'version':SEARCH_VERSION,'pages':search_docs},ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
(DOCS/'search-index.book.json').write_text(json.dumps({'version':SEARCH_VERSION,'pages':search_book},ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
(DOCS/'search-index.grammar.json').write_text(json.dumps({'version':SEARCH_VERSION,'pages':search_grammar},ensure_ascii=False,indent=2)+'\n',encoding='utf-8')

section_stats = defaultdict(int)
for item in search:
    section_stats[item['section']] += 1

pedagogy_stats = {
    'beginner': 0,
    'intermediate': 0,
    'advanced': 0,
}

for item in search:
    difficulty = item.get('difficulty', 'beginner')
    pedagogy_stats[difficulty] = pedagogy_stats.get(difficulty, 0) + 1

status={
  'version': STATUS_VERSION,
  'build_utc': datetime.now(timezone.utc).isoformat(),
  'pages': len(PAGES),
  'css_hash': hash_css,
  'js_hash': hash_js,
  'languages': [],
  'browser_support': ['modern evergreen browsers'],
  'sections': dict(section_stats),
  'total_search_entries': len(search),
  'book_pages': len(search_book),
  'docs_pages': len(search_docs),
  'grammar_pages': len(search_grammar),
  'pedagogy_version': PEDAGOGY_VERSION,
  'difficulty_distribution': pedagogy_stats,
  'concept_count': len(set(k for item in search for k in item.get('keywords', []))),
  'glossary_entries': len(set(k for item in search for k in item.get('keywords', []))),
}

(DOCS/'status.json').write_text(json.dumps(status,indent=2)+'\n',encoding='utf-8')

(DOCS/'pedagogy-report.json').write_text(
    json.dumps({
        'version': PEDAGOGY_VERSION,
        'difficulty_distribution': pedagogy_stats,
        'sections': dict(section_stats),
        'pages': len(search)
    }, indent=2) + '\n',
    encoding='utf-8'
)

concept_index = build_concept_index(search)
glossary = build_glossary(concept_index)

(DOCS/'concept-index.json').write_text(
    json.dumps({
        'version': CONCEPT_REPORT_VERSION,
        'concepts': concept_index
    }, ensure_ascii=False, indent=2) + '\n',
    encoding='utf-8'
)

(DOCS/'glossary.json').write_text(
    json.dumps({
        'version': GLOSSARY_REPORT_VERSION,
        'entries': glossary
    }, ensure_ascii=False, indent=2) + '\n',
    encoding='utf-8'
)

checks=[]
for rel in ['search-index.json','status.json','sw.js','js/main.js','css/site.css','svg/sprite.svg']:
  path=DOCS/rel
  if path.exists(): checks.append(f"{hashlib.sha256(path.read_bytes()).hexdigest()}  {rel}")
(DOCS/'checksums.txt').write_text('\n'.join(checks)+'\n',encoding='utf-8')

for lang in LANGS:
  ld=DOCS/lang; ld.mkdir(exist_ok=True)
  for folder in ['css','js','svg']:
    src=DOCS/folder; dst=ld/folder
    if dst.exists(): shutil.rmtree(dst)
    shutil.copytree(src,dst)
  lsearch=[]
  for p in PAGES:
    s=(DOCS/p.name).read_text(encoding='utf-8')
    s=HEADER_RE.sub(header(lang),s)
    s=FOOTER_RE.sub(footer(p.name,lang),s)
    am=ARTICLE_RE.search(s)
    if am: s=s.replace(am.group(1), translate_fragment(am.group(1),lang))
    s=re.sub(r'<link rel="canonical" href="[^"]+">', f'<link rel="canonical" href="https://vitte-lang.org/{lang}/{p.name}">', s)
    s=re.sub(r'<html lang="[^"]+">', f'<html lang="{lang}">', s)
    (ld/p.name).write_text(s,encoding='utf-8')
    bm=ARTICLE_RE.search(s); tt=TITLE_RE.search(s)
    if bm and tt:
      lsearch.append({
        'title':tt.group(1).strip(),
        'path':p.name,
        'content':text(bm.group(1))[:SEARCH_CONTENT_LIMIT],
        'lang':lang,
        'section':SECTION_DOCS
      })
  (ld/'search-index.json').write_text(json.dumps({'version':SEARCH_VERSION,'pages':lsearch},ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
  shutil.copy2(DOCS/'status.json', ld/'status.json'); shutil.copy2(DOCS/'checksums.txt', ld/'checksums.txt')

print('----------------------------------------')
print(f'Total HTML pages      : {len(ALL_HTML)}')
print(f'Documentation pages   : {len(search_docs)}')
print(f'Book pages            : {len(search_book)}')
print(f'Grammar pages         : {len(search_grammar)}')
print(f'Search entries        : {len(search)}')
print(f"Beginner pages        : {pedagogy_stats['beginner']}")
print(f"Intermediate pages    : {pedagogy_stats['intermediate']}")
print(f"Advanced pages        : {pedagogy_stats['advanced']}")
concept_total = len(set(k for item in search for k in item.get('keywords', [])))
print(f'Concepts detected     : {concept_total}')
print('----------------------------------------')
print('docs build done')
