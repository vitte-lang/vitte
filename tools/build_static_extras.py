#!/usr/bin/env python3
from pathlib import Path
import hashlib, json, re, subprocess
from datetime import datetime, timezone

DOCS=Path('docs')
BASE='https://vitte-lang.org'
now=datetime.now(timezone.utc).isoformat()
STRICT_CSP = "default-src 'self'; img-src 'self' data:; style-src 'self'; script-src 'self'; font-src 'self'; connect-src 'self';"
html_files=sorted([p for p in DOCS.rglob('*.html')])

assets=[]
for p in list(DOCS.rglob('*.css'))+list(DOCS.rglob('*.js')):
    h=hashlib.sha256(p.read_bytes()).hexdigest()
    assets.append({'path':str(p.relative_to(DOCS)).replace('\\','/'),'sha256':h,'size':p.stat().st_size})
asset_map={a['path']:a for a in assets}

for p in html_files:
    s=p.read_text(encoding='utf-8',errors='ignore')
    if 'skip-link' not in s:
        s=s.replace('<body class="classic-doc">','<body class="classic-doc">\n<a class="skip-link" href="#main-content">Skip to content</a>')
    if '<main class="site-main">' in s and 'id="main-content"' not in s:
        s=s.replace('<main class="site-main">','<main id="main-content" class="site-main">')

    m=re.search(r'href="css/site.css(?:\?v=[^"]+)?"',s)
    if m and 'rel="preload" as="style" href="css/site.css' not in s:
        href=m.group(0).split('"')[1]
        s=s.replace('</head>',f'<link rel="preload" as="style" href="{href}">\n</head>')

    # Local static assets are served from the same origin.
    # Strip SRI/crossorigin to avoid runtime style/script blocking when
    # hosting layers rewrite/minify responses.
    s=re.sub(r'\s+integrity="[^"]+"', '', s)
    s=re.sub(r'\s+crossorigin="anonymous"', '', s)

    if 'rel="alternate" hreflang=' not in s:
        name=p.name
        en=f'{BASE}/{name}'
        alt=f'<link rel="alternate" hreflang="en" href="{en}">\n<link rel="alternate" hreflang="x-default" href="{en}">\n'
        s=s.replace('</head>',alt+'</head>')

    pat_csp = re.compile(r'<meta http-equiv=\"Content-Security-Policy\" content=\"[^\"]*\">')
    s = pat_csp.sub('', s)
    s = s.replace('</head>', f'<meta http-equiv="Content-Security-Policy" content="{STRICT_CSP}">\n</head>')
    p.write_text(s,encoding='utf-8')

urls=[f'<url><loc>{BASE}/{str(p.relative_to(DOCS)).replace("\\\\","/")}</loc></url>' for p in sorted(DOCS.rglob('*.html'))]
(DOCS/'sitemap.xml').write_text('<?xml version="1.0" encoding="UTF-8"?>\n<urlset xmlns="http://www.sitemaps.org/schemas/sitemap/0.9">\n'+'\n'.join(urls)+'\n</urlset>\n',encoding='utf-8')
(DOCS/'robots.txt').write_text('User-agent: *\nAllow: /\nSitemap: https://vitte-lang.org/sitemap.xml\n',encoding='utf-8')

not_found_html='''<!doctype html><html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width, initial-scale=1"><title>404 - Page not found</title><link rel="stylesheet" href="css/site.css"><meta name="robots" content="noindex"></head><body class="classic-doc"><a class="skip-link" href="#main-content">Skip to content</a><div class="site-shell"><header class="site-header"><a class="site-brand" href="index.html"><img class="site-brand-mark" src="svg/logo.svg" alt="" width="32" height="32"><span>Vitte</span></a><nav class="site-nav" aria-label="Primary"><ul class="nav-band"><li><a class="nav-chip" href="index.html"><span>Welcome</span></a></li><li><a class="nav-chip" href="doc.html"><span>Documentation</span></a></li><li><a class="nav-chip" href="sitemap.html"><span>Sitemap</span></a></li><li><a class="nav-chip" href="status.html"><span>Status</span></a></li></ul></nav></header><main id="main-content" class="site-main"><article class="doc-content"><h1>Page not found</h1><p>The page does not exist or was moved.</p><p>Use the links below or search the docs.</p><div class="page-band"><a href="index.html">Home</a><a href="doc.html">Documentation</a><a href="sitemap.html">Sitemap</a><a href="status.html">Status</a></div></article></main><footer class="site-footer"><p class="site-footer-path">404.html</p><p><a href="index.html">Back to home</a></p></footer></div><script type="module" src="js/main.js"></script></body></html>'''
(DOCS/'404.html').write_text(not_found_html,encoding='utf-8')

offline_cached = [
  'index.html',
  'doc.html',
  'status.html',
  'offline.html',
  'search-index.json',
  'css/site.css',
  'css/layout.css',
  'js/main.js',
  'svg/logo.svg',
  'svg/sprite.svg',
]
cache_list = ''.join(f'<li><code>{p}</code></li>' for p in offline_cached)

offline_script = """
<script>
(function(){
  var status = document.getElementById('network-status');
  function paint(){
    var online = navigator.onLine;
    status.textContent = online ? 'online' : 'offline';
    status.setAttribute('data-state', online ? 'online' : 'offline');
  }
  paint();
  window.addEventListener('online', paint);
  window.addEventListener('offline', paint);
  var retry = document.getElementById('retry-btn');
  if (retry) retry.addEventListener('click', function(){ location.reload(); });
})();
</script>
""".strip()

offline_en=f'''<!doctype html><html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width, initial-scale=1"><title>Offline</title><link rel="stylesheet" href="css/site.css"><meta name="robots" content="noindex"></head><body class="classic-doc"><a class="skip-link" href="#main-content">Skip to content</a><div class="site-shell"><main id="main-content" class="site-main"><article class="doc-content"><h1>⚠ You are offline</h1><p>Network status: <strong id="network-status">offline</strong></p><p>Last docs build: <code>{now}</code></p><p>This page is lightweight and available without network.</p><div class="page-band"><button id="retry-btn" type="button">Retry</button><a href="index.html">Back to home</a><a href="doc.html">Documentation</a></div><h2>Pages available offline</h2><ul>{cache_list}</ul></article></main></div>{offline_script}</body></html>'''
(DOCS/'offline.html').write_text(offline_en,encoding='utf-8')

def make_index(files,out,section,lang='en'):
    pages=[]
    for p in files:
      t=p.read_text(encoding='utf-8',errors='ignore')
      m=re.search(r'<title>(.*?)</title>',t,re.S)
      body=re.sub(r'<[^>]+>',' ',t)
      body=re.sub(r'\s+',' ',body).strip()[:5000]
      pages.append({
        'title':m.group(1).strip() if m else p.stem,
        'path':str(p.relative_to(DOCS)).replace('\\','/'),
        'content':body,
        'lang':lang,
        'section':section
      })
    out.write_text(json.dumps({'version':'v1','pages':pages},ensure_ascii=False,indent=2)+'\n',encoding='utf-8')

make_index(sorted(DOCS.glob('*.html')), DOCS/'search-index.docs.json', 'docs', 'en')
make_index(sorted((DOCS/'grammar').glob('*.html')) if (DOCS/'grammar').exists() else [], DOCS/'search-index.grammar.json', 'grammar', 'en')
make_index(sorted((DOCS/'book').glob('*.html')) if (DOCS/'book').exists() else [], DOCS/'search-index.book.json', 'book', 'en')

try:
    logs=subprocess.check_output(['git','log','--pretty=format:%h %ad %s','--date=short','--','docs','src/vitte/grammar/vitte.ebnf'],text=True).splitlines()[:80]
except Exception:
    logs=[]
items=''.join(f'<li><code>{l}</code></li>' for l in logs)
(DOCS/'changelog.html').write_text(f'<!doctype html><html><head><meta charset="utf-8"><title>Docs Changelog</title><link rel="stylesheet" href="css/site.css"></head><body class="classic-doc"><div class="site-shell"><main class="site-main"><article class="doc-content"><h1>Docs Changelog</h1><ul>{items}</ul></article></main></div></body></html>',encoding='utf-8')

manifest={'version':'v1','built_at_utc':now,'assets':assets,'pages':len(list(DOCS.rglob('*.html')))}
(DOCS/'build-manifest.json').write_text(json.dumps(manifest,indent=2)+'\n',encoding='utf-8')
health={'built_at_utc':now,'pages':manifest['pages'],'asset_count':len(assets),'checksums':{a['path']:a['sha256'] for a in assets}}
health_text=json.dumps(health,indent=2).replace('&','&amp;').replace('<','&lt;')
status_banner=f'<div class="lead-panel"><h2>Docs Build Status</h2><p><strong>Version:</strong> {manifest["version"]} <strong>Date:</strong> {now}</p></div>'
status_html='<!doctype html><html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width, initial-scale=1"><title>Site Health</title><link rel="stylesheet" href="css/site.css"></head><body class="classic-doc"><a class="skip-link" href="#main-content">Skip to content</a><div class="site-shell"><header class="site-header"><a class="site-brand" href="index.html"><img class="site-brand-mark" src="svg/logo.svg" alt="" width="32" height="32"><span>Vitte</span></a><nav class="site-nav" aria-label="Primary"><ul class="nav-band"><li><a class="nav-chip" href="index.html"><span>Welcome</span></a></li><li><a class="nav-chip" href="doc.html"><span>Documentation</span></a></li><li><a class="nav-chip" href="sitemap.html"><span>Sitemap</span></a></li><li><a class="nav-chip" href="status.html"><span>Status</span></a></li></ul></nav></header><main id="main-content" class="site-main"><article class="doc-content"><h1>Site Health</h1>'+status_banner+'<pre>'+health_text+'</pre></article></main><footer class="site-footer"><p class="site-footer-path">status.html</p><p><a href="index.html">Back to home</a></p></footer></div><script type="module" src="js/main.js"></script></body></html>'
(DOCS/'status.html').write_text(status_html,encoding='utf-8')

status_public = f'''<!doctype html><html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width, initial-scale=1"><title>Docs Status</title><link rel="stylesheet" href="css/site.css"></head><body class="classic-doc"><a class="skip-link" href="#main-content">Skip to content</a><div class="site-shell"><header class="site-header"><a class="site-brand" href="index.html"><img class="site-brand-mark" src="svg/logo.svg" alt="" width="32" height="32"><span>Vitte</span></a></header><main id="main-content" class="site-main"><article class="doc-content"><h1>Documentation Status</h1><p><strong>Build version:</strong> {manifest["version"]}</p><p><strong>Last build (UTC):</strong> {now}</p><p><strong>Generated pages:</strong> {manifest["pages"]}</p><p><strong>Assets tracked:</strong> {len(assets)}</p><h2>Checks</h2><ul><li>Search index generated</li><li>Sitemap and robots generated</li><li>Checksums generated</li><li>Grammar sync and docs checks expected in CI</li></ul><p><a href="status.html">Open technical status (JSON view)</a></p></article></main><footer class="site-footer"><p class="site-footer-path">status-public.html</p></footer></div></body></html>'''
(DOCS/'status-public.html').write_text(status_public, encoding='utf-8')

print('static extras built')
