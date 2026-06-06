(function(){
  function q(s,r){return (r||document).querySelector(s)}
  function qa(s,r){return Array.from((r||document).querySelectorAll(s))}
  var KEY='book-progress-v1', LAST='book-last-read';

  function chapterPath(){ return location.pathname.replace(/^\//,''); }

  function updateProgress(){
    var all=[];
    fetch('/book/chapters-meta.json').then(r=>r.json()).then(d=>{
      all=d.chapters||[];
      var seen=JSON.parse(localStorage.getItem(KEY)||'{}');
      var done=all.filter(c=>seen[c.path]).length;
      var pct=all.length?Math.round((done/all.length)*100):0;
      var el=q('.book-progress');
      if(el) el.textContent='Progress: '+pct+'% ('+done+'/'+all.length+')';
    }).catch(()=>{});
  }

  document.addEventListener('click', function(e){
    var b=e.target.closest('.quiz-save');
    if(!b) return;
    var seen=JSON.parse(localStorage.getItem(KEY)||'{}');
    seen[chapterPath()]=true;
    localStorage.setItem(KEY,JSON.stringify(seen));
    localStorage.setItem(LAST,chapterPath());
    b.textContent='Saved';
    updateProgress();
  });

  document.addEventListener('DOMContentLoaded', function(){
    localStorage.setItem(LAST,chapterPath());
    updateProgress();

    // novice mode toggle
    var h=q('.site-header')||q('body');
    var btn=document.createElement('button');
    btn.type='button'; btn.className='novice-toggle';
    var on=localStorage.getItem('book-novice')==='on';
    if(on) document.body.classList.add('novice-mode');
    function paint(){ btn.textContent=document.body.classList.contains('novice-mode')?'Mode novice: on':'Mode novice: off'; }
    paint();
    btn.addEventListener('click',()=>{document.body.classList.toggle('novice-mode');localStorage.setItem('book-novice',document.body.classList.contains('novice-mode')?'on':'off');paint();});
    h.appendChild(btn);

    // keyboard nav
    document.addEventListener('keydown', (e)=>{
      if(e.key==='/' && !/input|textarea/i.test((document.activeElement||{}).tagName||'')){
        var s=q('.doc-search-input'); if(s){e.preventDefault(); s.focus();}
      }
      if(e.key===']'){ var n=q('.doc-pagination a:last-child'); if(n) location.href=n.getAttribute('href'); }
      if(e.key==='['){ var p=q('.doc-pagination a:first-child'); if(p) location.href=p.getAttribute('href'); }
    });

    // continue reading
    qa('.continue-reading').forEach(a=>{
      var last=localStorage.getItem(LAST);
      if(last) a.setAttribute('href','/'+last);
    });
  });
})();
