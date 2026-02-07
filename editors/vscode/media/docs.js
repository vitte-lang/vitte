

(function(){
  // --- VS Code bridge
  const vscode = acquireVsCodeApi();
  const qs = (sel, root=document) => root.querySelector(sel);
  const qsa = (sel, root=document) => Array.from(root.querySelectorAll(sel));

  // --- DOM refs
  const grid = qs('#grid');
  const empty = qs('#empty');
  const count = qs('#count');
  const status = qs('#status');
  const q = qs('#q');
  const prevBtn = qs('#prev');
  const nextBtn = qs('#next');
  const reloadBtn = qs('#reload');
  const openFolderBtn = qs('#openFolder');
  const sortSel = qs('#sort');
  const toggleViewBtn = qs('#toggleView');
  const chkIncludeContent = qs('#includeContent');
  const dlgHelp = qs('#help');

  // facets (optionnels)
  const fMd = qs('#f-md');
  const fReadme = qs('#f-readme');
  const fPathDocs = qs('#f-path-docs');
  const fPathWiki = qs('#f-path-wiki');

  // --- State
  const defaultState = {
    page: 1,
    pageSize: 100,
    lastQuery: '',
    total: 0,
    sort: 'relevance', // relevance | title | path
    view: 'grid',      // grid | list
    includeContent: true,
    filters: { md: true, readme: false, pathDocs: false, pathWiki: false },
    version: 1
  };
  let state = Object.assign({}, defaultState, (vscode.getState() || {}));

  // --- Helpers
  const setState = (patch) => { state = Object.assign({}, state, patch); vscode.setState(state); };
  const post = (type, payload) => vscode.postMessage(Object.assign({ type }, payload));
  const escapeHtml = (s) => String(s || '').replace(/[&<>"']/g, c => ({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;','\'':'&#39;'}[c]));
  const fmtCount = (start, end, total) => `${start}–${end} / ${total}`;

  function applyView(){
    document.body.classList.toggle('list', state.view === 'list');
    if (toggleViewBtn) toggleViewBtn.textContent = state.view === 'list' ? '▥ Grille' : '≣ Liste';
  }

  function card(item, query){
    const el = document.createElement('div');
    el.className = 'card';
    const title = highlight(item.title, query);
    const path = highlight(item.path, query);
    const excerpt = item.excerpt ? highlight(item.excerpt, query) : '';
    el.innerHTML =
      '<h4 class="title" title="' + escapeHtml(item.title) + '">' + title + '</h4>' +
      '<div class="path" title="' + escapeHtml(item.path) + '">' + path + '</div>' +
      (excerpt ? '<div class="excerpt">' + excerpt + '</div>' : '') +
      '<div class="row">' +
        '<button class="btn primary" data-fs="' + (item.fsPath || item.path) + '">Ouvrir</button>' +
        '<button class="btn ghost" data-copy="' + (item.fsPath || item.path) + '">Copier</button>' +
      '</div>';

    qs('button.primary', el).addEventListener('click', () => post('openFile', { fsPath: item.fsPath || item.path }));
    qs('button[data-copy]', el).addEventListener('click', async () => {
      try { await navigator.clipboard.writeText(item.fsPath || item.path); toast('Chemin copié'); } catch {}
    });
    return el;
  }

  function highlight(text, q){
    if (!q) return escapeHtml(text);
    const esc = q.replace(/[.*+?^${}()|[\]\\/]/g, '\\$&');
    return escapeHtml(text).replace(new RegExp(esc, 'ig'), (m) => '<mark>' + m + '</mark>');
  }

  function toast(msg){
    if (!status) return;
    status.textContent = msg;
    const t = Date.now();
    const my = t;
    setTimeout(() => { if (my === t) status.textContent = ''; }, 1200);
  }

  function render(items, total, query){
    grid.innerHTML = '';
    const start = (state.page - 1) * state.pageSize + 1;
    const end = (state.page - 1) * state.pageSize + items.length;
    if (items.length === 0) {
      grid.innerHTML = '';
      if (empty) empty.hidden = false;
      if (count) count.textContent = '';
    } else {
      if (empty) empty.hidden = true;
      for (const it of items) grid.appendChild(card(it, query));
      if (count) count.textContent = total ? fmtCount(start, end, total) : '';
    }
    prevBtn && (prevBtn.disabled = state.page <= 1);
    nextBtn && (nextBtn.disabled = end >= total);
  }

  function doSearch({ immediate=false } = {}){
    const query = (q && q.value) ? q.value.trim() : '';
    const payload = {
      q: query,
      page: state.page,
      pageSize: state.pageSize,
      sort: state.sort,
      includeContent: !!state.includeContent,
      filters: state.filters
    };
    if (immediate) {
      setState({ lastQuery: query, page: 1 });
      post('search', Object.assign({}, payload, { page: 1 }));
      return;
    }
    clearTimeout(doSearch._tid);
    doSearch._tid = setTimeout(() => {
      setState({ lastQuery: query, page: 1 });
      post('search', Object.assign({}, payload, { page: 1 }));
    }, 160);
  }

  // --- Events
  if (q) q.addEventListener('input', () => doSearch({ immediate:false }));
  if (prevBtn) prevBtn.addEventListener('click', () => { if (state.page > 1) { setState({ page: state.page - 1 }); post('search', { q: state.lastQuery, page: state.page, pageSize: state.pageSize, sort: state.sort, includeContent: state.includeContent, filters: state.filters }); }});
  if (nextBtn) nextBtn.addEventListener('click', () => { setState({ page: state.page + 1 }); post('search', { q: state.lastQuery, page: state.page, pageSize: state.pageSize, sort: state.sort, includeContent: state.includeContent, filters: state.filters }); });
  if (reloadBtn) reloadBtn.addEventListener('click', () => { post('reloadIndex', {}); toast('Reconstruction de l’index…'); });
  if (openFolderBtn) openFolderBtn.addEventListener('click', () => post('openDocsFolder', {}));
  if (toggleViewBtn) toggleViewBtn.addEventListener('click', () => { const view = state.view === 'grid' ? 'list' : 'grid'; setState({ view }); applyView(); });
  if (sortSel) sortSel.addEventListener('change', () => { setState({ sort: sortSel.value || 'relevance' }); doSearch({ immediate:true }); });
  if (chkIncludeContent) chkIncludeContent.addEventListener('change', () => { setState({ includeContent: !!chkIncludeContent.checked }); doSearch({ immediate:true }); });

  // facets (optionnels)
  function updateFilters(){
    setState({ filters: {
      md: !!(fMd && fMd.checked),
      readme: !!(fReadme && fReadme.checked),
      pathDocs: !!(fPathDocs && fPathDocs.checked),
      pathWiki: !!(fPathWiki && fPathWiki.checked)
    }});
    doSearch({ immediate:true });
  }
  if (fMd)      fMd.addEventListener('change', updateFilters);
  if (fReadme)  fReadme.addEventListener('change', updateFilters);
  if (fPathDocs)fPathDocs.addEventListener('change', updateFilters);
  if (fPathWiki)fPathWiki.addEventListener('change', updateFilters);

  // Open external links via extension
  qsa('a.link').forEach(a => a.addEventListener('click', (ev) => { ev.preventDefault(); const href = a.getAttribute('data-href'); if (href) post('openLink', { href }); }));

  // Keyboard shortcuts
  window.addEventListener('keydown', (ev) => {
    if (ev.key === 'F' && (ev.ctrlKey || ev.metaKey)) { ev.preventDefault(); q && q.focus(); q && q.select(); }
    if (ev.altKey && ev.key === 'ArrowLeft')  { ev.preventDefault(); prevBtn && prevBtn.click(); }
    if (ev.altKey && ev.key === 'ArrowRight') { ev.preventDefault(); nextBtn && nextBtn.click(); }
    if (ev.key === 'Enter' && document.activeElement === q) { ev.preventDefault(); doSearch({ immediate:true }); }
    if (ev.key === '?' && ev.shiftKey) { if (dlgHelp && typeof dlgHelp.showModal === 'function') dlgHelp.showModal(); }
  });

  // Apply initial UI state
  if (chkIncludeContent) chkIncludeContent.checked = !!state.includeContent;
  if (sortSel) sortSel.value = state.sort || 'relevance';
  applyView();

  // Messages from extension
  window.addEventListener('message', (e) => {
    const m = e.data || {};
    switch (m.type) {
      case 'results': {
        const items = Array.isArray(m.items) ? m.items : [];
        const total = Number(m.total || 0);
        setState({ total });
        render(items, total, m.query || '');
        break;
      }
      case 'status': {
        if (m.text) toast(String(m.text));
        break;
      }
      case 'badge': {
        const el = qs('#badgeVer'); if (el) el.textContent = String(m.text || '');
        break;
      }
      case 'focusSearch': {
        q && q.focus(); q && q.select();
        break;
      }
      default:
        break;
    }
  });

  // Initial search
  doSearch({ immediate:true });
})();
