#!/usr/bin/env python3
from pathlib import Path
import json, re

DOCS=Path('docs')
BOOK=DOCS/'book'
CHAPTERS=sorted(list((BOOK/'chapters').glob('*.html'))+list((BOOK/'poche').glob('*.html')))

def level_from_name(name:str)->str:
    m=re.match(r'(\d+)', name)
    if not m: return 'intermediate'
    n=int(m.group(1))
    if n<=20: return 'beginner'
    if n<=50: return 'intermediate'
    return 'advanced'

def tags_for(name:str, text:str):
    t=[]
    low=(name+' '+text).lower()
    for k in ['syntax','types','errors','compiler','modules','tests','grammar','performance','diagnostics','stdlib']:
        if k in low: t.append(k)
    return t[:5] or ['general']

meta=[]
for p in CHAPTERS:
    s=p.read_text(encoding='utf-8',errors='ignore')
    title=re.search(r'<h1[^>]*>(.*?)</h1>',s,re.S)
    title=(re.sub(r'<[^>]+>','',title.group(1)).strip() if title else p.stem)
    text=re.sub(r'<[^>]+>',' ',s)
    text=re.sub(r'\s+',' ',text)
    level=level_from_name(p.name)
    words=max(1,len(text.split()))
    minutes=max(3,round(words/180))
    tags=tags_for(p.name,text)
    prereq='' if not meta else meta[-1]['path']
    essential = level=='beginner'
    meta.append({
        'path':str(p.relative_to(DOCS)).replace('\\','/'),
        'title':title,
        'level':level,
        'minutes':minutes,
        'tags':tags,
        'prereq':prereq,
        'essential':essential,
        'maturity':'reviewed' if essential else 'draft',
        'last_review':'2026-05-09'
    })

(DOCS/'book'/'chapters-meta.json').write_text(json.dumps({'version':'v1','chapters':meta},ensure_ascii=False,indent=2)+'\n',encoding='utf-8')

# inject lightweight learning blocks
for i,p in enumerate(CHAPTERS):
    s=p.read_text(encoding='utf-8',errors='ignore')
    rel=str(p.relative_to(DOCS)).replace('\\','/')
    m=next(x for x in meta if x['path']==rel)
    if 'data-chapter-meta=' in s:
        continue
    tldr='''<section class="chapter-tldr"><h2>TL;DR (5 lines)</h2><ul><li>Read the core idea first.</li><li>Understand one concept at a time.</li><li>Run small examples.</li><li>Fix errors early.</li><li>Move to next chapter only when clear.</li></ul></section>'''
    errors='''<section class="chapter-errors"><h2>Frequent mistakes</h2><ul><li>Skipping prerequisites.</li><li>Reading without trying examples.</li><li>Fixing too many errors at once.</li></ul></section>'''
    quiz='''<section class="chapter-quiz"><h2>Mini quiz</h2><ol><li>What is the main goal of this chapter?</li><li>Which concept is most important?</li><li>What will you try right now?</li></ol><button class="quiz-save" type="button">Mark as reviewed</button></section>'''
    cta='''<section class="chapter-cta"><h2>Next best action</h2><p>Apply one idea from this chapter in a tiny example, then continue.</p></section>'''
    badge=f'<p class="chapter-meta" data-chapter-meta="1">Level: <strong>{m["level"]}</strong> · Reading time: <strong>{m["minutes"]} min</strong> · Prerequisite: <code>{m["prereq"] or "none"}</code> · Last review: <strong>{m["last_review"]}</strong></p>'
    # insert after first h1
    s=re.sub(r'(</h1>)', r'\1\n'+badge+tldr+errors, s, count=1)
    # add see also from nearby chapters
    also=[]
    for j in [i-1,i+1,i+2]:
        if 0<=j<len(meta):
            also.append(meta[j])
    also_html=''.join([f'<li><a href="/{a["path"]}">{a["title"]}</a></li>' for a in also[:3]])
    see=f'<section class="chapter-see-also"><h2>See also</h2><ul>{also_html}</ul></section>'
    s=s.replace('</article>', quiz+see+cta+'</article>')
    p.write_text(s,encoding='utf-8')

print('book learning layer built')
