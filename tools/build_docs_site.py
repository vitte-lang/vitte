#!/usr/bin/env python3
from __future__ import annotations
import hashlib, json, re, shutil, urllib.parse, urllib.request, html
from html.parser import HTMLParser
from datetime import datetime, timezone
from pathlib import Path

DOCS=Path('docs')
LANGS=['en','fr']
PAGES=sorted([p for p in DOCS.glob('*.html') if p.name!='status.html'])
HEADER_RE=re.compile(r'<header class="site-header">[\s\S]*?</header>')
FOOTER_RE=re.compile(r'<footer class="site-footer">[\s\S]*?</footer>')
SCRIPT_RE=re.compile(r'<script[^>]*site-interactions\.js[^>]*></script>\n?')
HEAD_RE=re.compile(r'</head>')
BODY_END_RE=re.compile(r'</body>')
ARTICLE_RE=re.compile(r'<article class="doc-content">([\s\S]*?)</article>')
TITLE_RE=re.compile(r'<title>(.*?)</title>')
TAG_RE=re.compile(r'<[^>]+>')

NAV=[('Welcome','index.html','i-home'),('Documentation','doc.html','i-docs'),('Download','download.html','i-docs'),('Source','source.html','i-docs'),('Community','community.html','i-docs'),('News','news.html','i-news'),('Diagnostics','diagnostics.html','i-docs'),('Suggestions','suggestions.html','i-docs')]
CACHE={}

def text(x): return re.sub(r'\s+',' ',TAG_RE.sub(' ',x)).strip()
def sha(path): return hashlib.sha256(path.read_bytes()).hexdigest()[:10]

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

def header(lang='en'):
  items=''.join([f'<li><a class="nav-chip" href="{h}"><svg width="14" height="14" aria-hidden="true" focusable="false"><use href="svg/sprite.svg#{i}"></use></svg><span>{tr(l,lang)}</span></a></li>' for l,h,i in NAV])
  return f'<header class="site-header">\n<a class="site-brand" href="index.html"><img class="site-brand-mark" src="svg/logo.svg" alt="" width="32" height="32"><span>Vitte</span></a>\n<nav class="site-nav" aria-label="Primary"><ul class="nav-band">{items}</ul></nav>\n</header>'

def footer(name, lang='en'): return f'<footer class="site-footer">\n<p class="site-footer-path">{name}</p>\n<p><a href="index.html">{tr("Back to home",lang)}</a></p>\n</footer>'

hash_css=sha(DOCS/'css/site.css') if (DOCS/'css/site.css').exists() else 'dev'
hash_js=sha(DOCS/'js/main.js') if (DOCS/'js/main.js').exists() else 'dev'

search=[]
for idx,p in enumerate(PAGES):
  s=p.read_text(encoding='utf-8')
  s=HEADER_RE.sub(header('en'),s)
  s=FOOTER_RE.sub(footer(p.name,'en'),s)
  s=SCRIPT_RE.sub('',s)
  title=(TITLE_RE.search(s).group(1) if TITLE_RE.search(s) else p.stem)
  canonical=f'https://vitte.dev/{p.name}'
  meta=(f'<link rel="canonical" href="{canonical}">\n<meta property="og:title" content="{title}">\n<meta property="og:type" content="website">\n<meta property="og:url" content="{canonical}">\n<meta name="twitter:card" content="summary">\n')
  if 'rel="canonical"' not in s: s=HEAD_RE.sub(meta+'</head>',s,1)
  s=BODY_END_RE.sub(f'<script type="module" src="js/main.js?v={hash_js}"></script>\n</body>',s,1)
  s=s.replace('href="css/site.css"', f'href="css/site.css?v={hash_css}"')
  prevp=PAGES[idx-1].name if idx>0 else None; nextp=PAGES[idx+1].name if idx+1<len(PAGES) else None
  pag='<nav class="doc-pagination">'+(f'<a href="{prevp}">← Previous</a>' if prevp else '<span></span>')+(f'<a href="{nextp}">Next →</a>' if nextp else '')+'</nav>'
  if 'doc-pagination' not in s: s=s.replace('</article>', pag+'</article>')
  p.write_text(s,encoding='utf-8')
  b=ARTICLE_RE.search(s)
  if b: search.append({'title':title,'path':p.name,'content':text(b.group(1))[:5000]})

(DOCS/'search-index.json').write_text(json.dumps({'version':'v4','pages':search},ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
status={'version':'v4','build_utc':datetime.now(timezone.utc).isoformat(),'pages':len(PAGES),'css_hash':hash_css,'js_hash':hash_js,'languages':LANGS,'browser_support':['modern evergreen browsers']}
(DOCS/'status.json').write_text(json.dumps(status,indent=2)+'\n',encoding='utf-8')
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
    s=re.sub(r'<link rel="canonical" href="[^"]+">', f'<link rel="canonical" href="https://vitte.dev/{lang}/{p.name}">', s)
    s=re.sub(r'<html lang="[^"]+">', f'<html lang="{lang}">', s)
    (ld/p.name).write_text(s,encoding='utf-8')
    bm=ARTICLE_RE.search(s); tt=TITLE_RE.search(s)
    if bm and tt: lsearch.append({'title':tt.group(1).strip(),'path':p.name,'content':text(bm.group(1))[:5000]})
  (ld/'search-index.json').write_text(json.dumps({'version':'v4','pages':lsearch},ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
  shutil.copy2(DOCS/'status.json', ld/'status.json'); shutil.copy2(DOCS/'checksums.txt', ld/'checksums.txt')

print('docs build done')
