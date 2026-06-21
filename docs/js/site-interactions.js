(function () {
  var TEXT_SIZE_KEY = "docs-text-size";
  var LANG_KEY = "docs-language";

  function q(sel, root) { return (root || document).querySelector(sel); }
  function qa(sel, root) { return Array.prototype.slice.call((root || document).querySelectorAll(sel)); }
  function esc(s) { return (s || "").replace(/[.*+?^${}()|[\]\\]/g, "\\$&"); }

  function setupTextSizeToggle() {
    var header = q(".site-header"); if (!header) return;
    if (localStorage.getItem(TEXT_SIZE_KEY) === "large") document.body.classList.add("text-large");
    var btn = document.createElement("button"); btn.type = "button"; btn.className = "text-size-toggle";
    function render() { var large = document.body.classList.contains("text-large"); btn.textContent = large ? "Texte: grand" : "Texte: normal"; btn.setAttribute("aria-pressed", String(large)); }
    render();
    btn.addEventListener("click", function () { var large = document.body.classList.toggle("text-large"); localStorage.setItem(TEXT_SIZE_KEY, large ? "large" : "normal"); render(); });
    header.appendChild(btn);
  }

  function setupLanguageSwitcher() {
    var header = q(".site-header"); if (!header) return;
    var navLangs = ["en"], browserLang = (navigator.language || "en").slice(0, 2).toLowerCase();
    var stored = localStorage.getItem(LANG_KEY) || "auto", path = window.location.pathname, currentLang = /\/fr\//.test(path) ? "fr" : "en";
    var wrap = document.createElement("div"); wrap.className = "language-switcher";
    var select = document.createElement("select"); select.className = "language-select"; select.setAttribute("aria-label", "Language");
    select.innerHTML = '<option value="en">English</option>'; select.value = "en";
    var autoLink = document.createElement("a"); autoLink.className = "language-auto-translate"; autoLink.target = "_blank"; autoLink.rel = "noopener noreferrer"; autoLink.textContent = "Traduire automatiquement";
    var status = document.createElement("span"); status.className = "language-status";
    function translateUrl(targetLang) { return "https://translate.google.com/translate?sl=auto&tl=" + encodeURIComponent(targetLang) + "&u=" + encodeURIComponent(window.location.href); }
    function switchLang(value) { var resolved = value === "auto" ? "en" : value; autoLink.href = translateUrl(browserLang); status.textContent = navLangs.indexOf(browserLang) === -1 ? "External language: browser translation available" : ""; localStorage.setItem(LANG_KEY, resolved); document.documentElement.lang = "en"; var file = window.location.pathname.split("/").pop() || "index.html"; window.location.href = "/" + file; }
    select.addEventListener("change", function () { switchLang(select.value); }); wrap.appendChild(select); wrap.appendChild(autoLink); wrap.appendChild(status); header.appendChild(wrap);
  }

  function setupMobileMenu() {
    var nav = q(".site-nav"), header = q(".site-header"); if (!nav || !header || !q("ul", nav)) return;
    if (!nav.id) nav.id = "primary-nav";
    var btn = document.createElement("button"); btn.type = "button"; btn.className = "menu-toggle"; btn.setAttribute("aria-expanded", "false"); btn.setAttribute("aria-controls", nav.id); btn.textContent = "Menu"; header.insertBefore(btn, nav);
    btn.addEventListener("click", function () { var open = btn.getAttribute("aria-expanded") === "true"; btn.setAttribute("aria-expanded", String(!open)); nav.classList.toggle("is-open", !open); });
    nav.addEventListener("click", function (e) { if (!(e.target instanceof Element) || !e.target.closest("a") || !window.matchMedia("(max-width: 768px)").matches) return; btn.setAttribute("aria-expanded", "false"); nav.classList.remove("is-open"); });
  }

  function wrapTables() { qa(".doc-content table").forEach(function (t) { if (t.parentElement && t.parentElement.classList.contains("table-scroll")) return; var d = document.createElement("div"); d.className = "table-scroll"; t.parentNode.insertBefore(d, t); d.appendChild(t); }); }

  function addFloatingToc() {
    var content = q(".doc-content"); if (!content) return;
    var headings = qa("h2, h3", content); if (headings.length < 4) return;
    var toc = document.createElement("aside"); toc.className = "floating-toc"; toc.setAttribute("aria-label", "Table of contents");
    var title = document.createElement("p"); title.className = "floating-toc-title"; title.textContent = "On this page"; toc.appendChild(title);
    var ul = document.createElement("ul"); toc.appendChild(ul);
    headings.forEach(function (h, i) { if (!h.id) h.id = "section-" + (i + 1); var li = document.createElement("li"); li.className = h.tagName.toLowerCase() === "h3" ? "toc-sub" : "toc-main"; var a = document.createElement("a"); a.href = "#" + h.id; a.textContent = (h.textContent || "Section").trim(); a.dataset.targetId = h.id; li.appendChild(a); ul.appendChild(li); });
    content.appendChild(toc);
  }

  function addBackToTop() {
    var btn = document.createElement("button"); btn.type = "button"; btn.className = "back-to-top"; btn.textContent = "Retour en haut"; btn.setAttribute("aria-label", "Retour en haut"); document.body.appendChild(btn);
    function onScroll() { btn.classList.toggle("is-visible", window.scrollY > 320); }
    btn.addEventListener("click", function () { window.scrollTo({ top: 0, behavior: "smooth" }); });
    window.addEventListener("scroll", onScroll, { passive: true }); onScroll();
  }

  function lazyLoadDecorativeSvg() {
    var imgs = qa('img.svg-decorative[src$=".svg"][aria-hidden="true"], img.svg-decorative[src$=".svg"][alt=""]'); if (!imgs.length || !("IntersectionObserver" in window)) return;
    var io = new IntersectionObserver(function (entries) { entries.forEach(function (entry) { if (!entry.isIntersecting) return; var img = entry.target, src = img.getAttribute("data-src"); if (src) { img.setAttribute("src", src); img.removeAttribute("data-src"); } io.unobserve(img); }); }, { rootMargin: "220px 0px" });
    imgs.forEach(function (img) { if (img.closest(".page-band") || img.closest(".site-nav") || img.closest(".site-brand")) return; var src = img.getAttribute("src"); if (!src) return; img.setAttribute("data-src", src); img.setAttribute("src", "data:image/gif;base64,R0lGODlhAQABAIAAAAAAAP///ywAAAAAAQABAAACAUwAOw=="); io.observe(img); });
  }

  function registerServiceWorker() { if ("serviceWorker" in navigator) navigator.serviceWorker.register("sw.js").catch(function () {}); }

  function addBreadcrumbs() {
    var content = q(".doc-content"); if (!content || q(".breadcrumbs", content)) return;
    var nav = document.createElement("nav"); nav.className = "breadcrumbs"; nav.setAttribute("aria-label", "Breadcrumb");
    var parts = window.location.pathname.replace(/^\/+|\/+$/g, "").split("/").filter(Boolean); if (!parts.length) parts = ["index.html"];
    var file = parts[parts.length - 1] || "index.html"; var dir = parts.length > 1 ? parts[parts.length - 2] : ""; var label = file.replace(/\.html$/i, "").replace(/[-_]/g, " ");
    var homeHref = "/index.html";
    nav.innerHTML = '<a href="' + homeHref + '">Home</a>' + (dir ? '<span> / </span><span>' + dir + '</span>' : '') + '<span> / </span><span>' + (label || 'index') + '</span>';
    content.insertBefore(nav, content.firstChild);
  }

  function setupContrastToggle() { var KEY = "docs-contrast", header = q(".site-header"); if (!header) return; var btn = document.createElement("button"); btn.className = "contrast-toggle"; btn.type = "button"; btn.textContent = "Contraste"; if (localStorage.getItem(KEY) === "high") document.body.classList.add("high-contrast"); btn.addEventListener("click", function () { document.body.classList.toggle("high-contrast"); localStorage.setItem(KEY, document.body.classList.contains("high-contrast") ? "high" : "normal"); }); header.appendChild(btn); }

  function tokenize(s) { return (s || "").toLowerCase().trim(); }
  function scoreItem(item, qv) {
    var t = (item.title || "").toLowerCase(), p = (item.path || "").toLowerCase(), c = (item.content || "").toLowerCase();
    var score = 0;
    if (t.indexOf(qv) >= 0) score += 3;
    if (p.indexOf(qv) >= 0) score += 2;
    if (c.indexOf(qv) >= 0) score += 1;
    return score;
  }
  function highlight(text, qv) {
    if (!qv) return text || "";
    return (text || "").replace(new RegExp("(" + esc(qv) + ")", "ig"), "<mark>$1</mark>");
  }
  function snippet(content, qv) {
    var raw = content || "";
    var lc = raw.toLowerCase(), i = lc.indexOf(qv);
    if (i < 0) return raw.slice(0, 160);
    var start = Math.max(0, i - 70), end = Math.min(raw.length, i + qv.length + 90);
    var cut = raw.slice(start, end);
    return (start > 0 ? "…" : "") + cut + (end < raw.length ? "…" : "");
  }

  function setupGlobalSearch() {
    var header = q(".site-header");
    if (!header) return;

    var form = q(".doc-search", header);
    if (!form) {
      form = document.createElement("form");
      form.className = "doc-search";
      form.setAttribute("role", "search");
      form.setAttribute("action", searchPageURL());
      form.setAttribute("method", "get");
      header.appendChild(form);
    }

    form.setAttribute("role", "search");
    form.classList.remove("is-open");

    var input = q(".doc-search-input", form);
    if (!input) {
      input = document.createElement("input");
      input.className = "doc-search-input";
      form.insertBefore(input, form.firstChild);
    }
    input.type = "search";
    input.name = "q";
    input.placeholder = input.placeholder || "Search docs, book, grammar";
    input.setAttribute("aria-label", input.getAttribute("aria-label") || "Search documentation");
    input.setAttribute("autocomplete", "off");

    var controls = q(".doc-search-controls", form);
    if (!controls) {
      controls = document.createElement("div");
      controls.className = "doc-search-controls";
      input.insertAdjacentElement("afterend", controls);
    }

    var filters = qa(".doc-search-filter", controls);
    var sectionSel = q(".doc-search-section", controls) || filters[0];
    if (!sectionSel) {
      sectionSel = document.createElement("select");
      sectionSel.className = "doc-search-filter doc-search-section";
      controls.appendChild(sectionSel);
    }
    sectionSel.name = "section";
    sectionSel.setAttribute("aria-label", "Filter by section");
    sectionSel.innerHTML = '<option value="all">All sections</option><option value="docs">Docs</option><option value="book">Book</option><option value="grammar">Grammar</option>';

    var langSel = q(".doc-search-lang", controls) || filters[1];
    if (!langSel) {
      langSel = document.createElement("select");
      langSel.className = "doc-search-filter doc-search-lang";
      controls.appendChild(langSel);
    }
    langSel.name = "lang";
    langSel.setAttribute("aria-label", "Filter by language");
    langSel.innerHTML = '<option value="all">All languages</option><option value="en">EN</option><option value="fr">FR</option>';

    var results = q(".doc-search-results", form);
    if (!results) {
      results = document.createElement("div");
      results.className = "doc-search-results";
      form.appendChild(results);
    }
    results.hidden = true;

    var footer = q(".doc-search-footer", form);
    if (!footer) {
      footer = document.createElement("div");
      footer.className = "doc-search-footer";
      form.appendChild(footer);
    }
    footer.hidden = true;

    syncHeaderOffset();

    var pages = [], active = -1;

    function closeSearch() {
      results.hidden = true;
      footer.hidden = true;
      form.classList.remove("is-open");
      active = -1;
    }

    function openSearch() {
      form.classList.add("is-open");
    }

    function syncURL(qv, section, lang) {
      var u = new URL(window.location.href);
      if (qv) u.searchParams.set("q", qv); else u.searchParams.delete("q");
      if (section && section !== "all") u.searchParams.set("section", section); else u.searchParams.delete("section");
      if (lang && lang !== "all") u.searchParams.set("lang", lang); else u.searchParams.delete("lang");
      history.replaceState({}, "", u.toString());
    }

    function applyFromURL() {
      var u = new URL(window.location.href);
      input.value = u.searchParams.get("q") || "";
      sectionSel.value = u.searchParams.get("section") || "all";
      langSel.value = u.searchParams.get("lang") || "all";
    }

    function renderFooter(qv, total) {
      if (!qv || qv.length < 2) {
        footer.hidden = true;
        footer.innerHTML = "";
        return;
      }
      var u = new URL(searchPageURL());
      u.searchParams.set("q", qv);
      if (sectionSel.value !== "all") u.searchParams.set("section", sectionSel.value);
      if (langSel.value !== "all") u.searchParams.set("lang", langSel.value);
      footer.hidden = false;
      footer.innerHTML = '<a class="doc-search-more" href="' + u.href + '">Open full results</a><span class="doc-search-count">' + total + ' match(es)</span>';
    }

    function render() {
      var qv = tokenize(input.value), section = sectionSel.value || "all", lang = langSel.value || "all";
      syncURL(qv, section, lang);
      if (!qv || qv.length < 2) {
        results.hidden = true;
        results.innerHTML = "";
        renderFooter("", 0);
        form.classList.remove("is-open");
        return;
      }
      var ranked = rankPages(pages, qv, section, lang, 8);
      renderFooter(qv, ranked.length);
      openSearch();
      if (!ranked.length) {
        results.hidden = false;
        results.innerHTML = '<p class="doc-search-empty">No results. Browse the <a href="' + sitemapURL() + '">sitemap</a>.</p>';
        return;
      }
      results.hidden = false;
      results.innerHTML = ranked.map(function (r, idx) {
        var title = highlight(r.item.title, qv);
        var ex = highlight(snippet(r.item.content, qv), qv);
        return '<a class="doc-search-item" data-idx="' + idx + '" href="' + r.item.href + '"><strong>' + title + '</strong><span>' + r.item.path + ' · ' + r.item.section + ' · ' + r.item.lang + '</span><em>' + ex + '</em></a>';
      }).join("");
      active = -1;
    }

    loadSearchPages().then(function (loaded) {
      pages = loaded;
      applyFromURL();
      render();
      syncHeaderOffset();
    }).catch(function () {
      pages = [];
      applyFromURL();
      render();
      syncHeaderOffset();
    });

    function focusMove(delta) {
      var items = qa(".doc-search-item", results);
      if (!items.length) return;
      active = (active + delta + items.length) % items.length;
      items.forEach(function (el, i) { el.classList.toggle("is-active", i === active); });
      items[active].focus();
    }

    input.addEventListener("focus", function () { if (tokenize(input.value).length >= 2) render(); });
    input.addEventListener("input", render);
    sectionSel.addEventListener("change", render);
    langSel.addEventListener("change", render);
    form.addEventListener("submit", function (e) {
      e.preventDefault();
      var items = qa(".doc-search-item", results);
      if (active >= 0 && items[active]) { window.location.href = items[active].getAttribute("href"); return; }
      if (items[0]) { window.location.href = items[0].getAttribute("href"); return; }
      var u = new URL(searchPageURL());
      if (input.value.trim()) u.searchParams.set("q", input.value.trim());
      if (sectionSel.value !== "all") u.searchParams.set("section", sectionSel.value);
      if (langSel.value !== "all") u.searchParams.set("lang", langSel.value);
      window.location.href = u.href;
    });
    form.addEventListener("keydown", function (e) {
      if (e.key === "Escape") { e.preventDefault(); closeSearch(); input.blur(); return; }
      if (results.hidden) return;
      if (e.key === "ArrowDown") { e.preventDefault(); focusMove(1); }
      else if (e.key === "ArrowUp") { e.preventDefault(); focusMove(-1); }
      else if (e.key === "Enter" && active >= 0) {
        var items = qa(".doc-search-item", results); if (items[active]) window.location.href = items[active].getAttribute("href");
      }
    });
    document.addEventListener("click", function (e) {
      if (!(e.target instanceof Element) || form.contains(e.target)) return;
      closeSearch();
    });
  }


  function inferBookLevel(path) {
    var m = path.match(/\/book\/(?:chapters\/)?(\d+)/i);
    if (!m) return null;
    var n = parseInt(m[1], 10);
    if (isNaN(n)) return null;
    if (n <= 20) return 'Niveau: débutant';
    if (n <= 50) return 'Niveau: intermédiaire';
    return 'Niveau: avancé';
  }

  function addBookLevelBadge() {
    var content = q('.doc-content');
    if (!content || q('.book-level-badge', content)) return;
    var level = inferBookLevel(window.location.pathname);
    if (!level) return;
    var h1 = q('h1', content);
    var badge = document.createElement('p');
    badge.className = 'book-level-badge';
    badge.textContent = level;
    if (h1 && h1.nextSibling) h1.parentNode.insertBefore(badge, h1.nextSibling);
    else content.insertBefore(badge, content.firstChild);
  }

  function applyLongPageTemplate() {
    var content = q('.doc-content');
    if (!content || q('.long-page-intro', content)) return;
    var headings = qa('h2, h3', content);
    var textLen = (content.textContent || '').trim().length;
    if (headings.length < 5 && textLen < 2600) return;
    var intro = document.createElement('p');
    intro.className = 'long-page-intro';
    intro.textContent = "Lecture recommandée: lis l'intro, puis avance section par section avec les titres.";
    var firstH2 = q('h2', content);
    if (firstH2) content.insertBefore(intro, firstH2);
    else content.insertBefore(intro, content.firstChild);
  }

  function addSmartPagination() {
    var content = q('.doc-content'); if (!content || q('.doc-pagination', content)) return;
    if ((content.textContent || "").trim().length < 2000 && qa('h2, h3', content).length < 6) return;
    var links = qa('.site-nav .nav-chip'); if (links.length < 2) return;
    var current = window.location.pathname.split('/').pop() || 'index.html';
    var items = links.map(function (a) { return { href: a.getAttribute('href') || '', label: (a.textContent || '').trim() }; }).filter(function (it) { return /\.html?$/i.test(it.href); });
    var index = items.findIndex(function (it) { return it.href === current; }); if (index === -1) return;
    var prev = items[index - 1] || null, next = items[index + 1] || null; if (!prev && !next) return;
    var nav = document.createElement('nav'); nav.className = 'doc-pagination';
    nav.innerHTML = (prev ? '<a href="' + prev.href + '">← ' + prev.label + '</a>' : '<span></span>') + (next ? '<a href="' + next.href + '">' + next.label + ' →</a>' : '');
    content.appendChild(nav);
  }

  document.addEventListener("DOMContentLoaded", function () {
    registerServiceWorker(); addBreadcrumbs(); setupContrastToggle(); setupGlobalSearch(); setupLanguageSwitcher(); setupThemeToggle(); setupTextSizeToggle(); setupMobileMenu(); wrapTables(); addFloatingToc(); addBackToTop(); lazyLoadDecorativeSvg(); addSmartPagination();
  });
})();
