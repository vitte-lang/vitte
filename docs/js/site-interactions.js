(function () {
  var TEXT_SIZE_KEY = 'docs-text-size';
  var LANG_KEY = 'docs-language';

  function initTextSizeToggle() {
    var header = document.querySelector('.site-header');
    if (!header) return;

    var saved = localStorage.getItem(TEXT_SIZE_KEY);
    if (saved === 'large') {
      document.body.classList.add('text-large');
    }

    var button = document.createElement('button');
    button.type = 'button';
    button.className = 'text-size-toggle';
    button.textContent = document.body.classList.contains('text-large') ? 'Texte: grand' : 'Texte: normal';
    button.setAttribute('aria-pressed', String(document.body.classList.contains('text-large')));

    button.addEventListener('click', function () {
      var isLarge = document.body.classList.toggle('text-large');
      localStorage.setItem(TEXT_SIZE_KEY, isLarge ? 'large' : 'normal');
      button.textContent = isLarge ? 'Texte: grand' : 'Texte: normal';
      button.setAttribute('aria-pressed', String(isLarge));
    });

    header.appendChild(button);
  }

  function initLanguageSelector() {
    var header = document.querySelector('.site-header');
    if (!header) return;

    var saved = localStorage.getItem(LANG_KEY) || 'auto';
    var browserLang = (navigator.language || 'en').slice(0, 2).toLowerCase();
    var path = window.location.pathname;
    var supported = ['en','fr'];
    var langMatch = path.match(/\/(en|fr)\//);
    var currentLang = langMatch ? langMatch[1] : 'en';

    var wrap = document.createElement('div');
    wrap.className = 'language-switcher';

    var select = document.createElement('select');
    select.className = 'language-select';
    select.setAttribute('aria-label', 'Language');
    select.innerHTML = '<option value="auto">Auto</option><option value="en">English</option><option value="fr">Français</option>'; 
    if (saved === 'auto') {
      select.value = currentLang;
    } else {
      select.value = saved;
    }

    var status = document.createElement('span');
    status.className = 'language-status';
    status.textContent = '';
    var autoTranslate = document.createElement('a');
    autoTranslate.className = 'language-auto-translate';
    autoTranslate.target = '_blank';
    autoTranslate.rel = 'noopener noreferrer';
    autoTranslate.textContent = 'Traduire automatiquement';


    function buildGoogleTranslateUrl(targetLang) {
      return 'https://translate.google.com/translate?sl=auto&tl=' + encodeURIComponent(targetLang) + '&u=' + encodeURIComponent(window.location.href);
    }

    function applySelection(lang) {
      var targetLang = lang === 'auto' ? (supported.indexOf(browserLang)>=0 ? browserLang : 'en') : lang;
      autoTranslate.href = buildGoogleTranslateUrl(browserLang);
      status.textContent = supported.indexOf(browserLang) === -1 ? 'Langue externe: traduction auto disponible' : ''; 
      localStorage.setItem(LANG_KEY, targetLang);
      document.documentElement.lang = targetLang;
      var nextPath = '/' + targetLang + '/';
      var file = window.location.pathname.split('/').pop() || 'index.html';
      window.location.href = nextPath + file;
    }

    select.addEventListener('change', function () {
      applySelection(select.value);
    });
    wrap.appendChild(select);
    wrap.appendChild(autoTranslate);
    wrap.appendChild(status);
    header.appendChild(wrap);
  }

  function initMobileMenu() {
    var nav = document.querySelector('.site-nav');
    var header = document.querySelector('.site-header');
    if (!nav || !header) return;

    var navList = nav.querySelector('ul');
    if (!navList) return;

    if (!nav.id) nav.id = 'primary-nav';

    var button = document.createElement('button');
    button.type = 'button';
    button.className = 'menu-toggle';
    button.setAttribute('aria-expanded', 'false');
    button.setAttribute('aria-controls', nav.id);
    button.textContent = 'Menu';

    header.insertBefore(button, nav);

    button.addEventListener('click', function () {
      var expanded = button.getAttribute('aria-expanded') === 'true';
      button.setAttribute('aria-expanded', String(!expanded));
      nav.classList.toggle('is-open', !expanded);
    });

    nav.addEventListener('click', function (event) {
      if (!(event.target instanceof Element)) return;
      if (!event.target.closest('a')) return;
      if (window.matchMedia('(max-width: 768px)').matches) {
        button.setAttribute('aria-expanded', 'false');
        nav.classList.remove('is-open');
      }
    });
  }

  function wrapTables() {
    var tables = document.querySelectorAll('.doc-content table');
    tables.forEach(function (table) {
      if (table.parentElement && table.parentElement.classList.contains('table-scroll')) {
        return;
      }
      var wrapper = document.createElement('div');
      wrapper.className = 'table-scroll';
      table.parentNode.insertBefore(wrapper, table);
      wrapper.appendChild(table);
    });
  }

  function initFloatingToc() {
    var article = document.querySelector('.doc-content');
    if (!article) return;

    var headings = Array.prototype.slice.call(article.querySelectorAll('h2, h3'));
    if (headings.length < 4) return;

    var toc = document.createElement('aside');
    toc.className = 'floating-toc';
    toc.setAttribute('aria-label', 'Table of contents');

    var title = document.createElement('p');
    title.className = 'floating-toc-title';
    title.textContent = 'On this page';
    toc.appendChild(title);

    var list = document.createElement('ul');
    toc.appendChild(list);

    headings.forEach(function (heading, index) {
      if (!heading.id) {
        heading.id = 'section-' + (index + 1);
      }
      var item = document.createElement('li');
      item.className = heading.tagName.toLowerCase() === 'h3' ? 'toc-sub' : 'toc-main';
      var link = document.createElement('a');
      link.href = '#' + heading.id;
      link.textContent = heading.textContent ? heading.textContent.trim() : 'Section';
      link.dataset.targetId = heading.id;
      item.appendChild(link);
      list.appendChild(item);
    });

    article.appendChild(toc);

    var links = list.querySelectorAll('a');
    var observer = new IntersectionObserver(
      function (entries) {
        entries.forEach(function (entry) {
          if (!entry.isIntersecting) return;
          links.forEach(function (link) {
            var isActive = link.dataset.targetId === entry.target.id;
            link.classList.toggle('is-active', isActive);
          });
        });
      },
      {
        rootMargin: '-20% 0px -65% 0px',
        threshold: 0.1
      }
    );

    headings.forEach(function (heading) {
      observer.observe(heading);
    });
  }

  function initBackToTop() {
    var button = document.createElement('button');
    button.type = 'button';
    button.className = 'back-to-top';
    button.textContent = 'Retour en haut';
    button.setAttribute('aria-label', 'Retour en haut');
    document.body.appendChild(button);

    function updateVisibility() {
      button.classList.toggle('is-visible', window.scrollY > 320);
    }

    button.addEventListener('click', function () {
      window.scrollTo({ top: 0, behavior: 'smooth' });
    });

    window.addEventListener('scroll', updateVisibility, { passive: true });
    updateVisibility();
  }

  function lazyLoadDecorativeSvgs() {
    var candidates = Array.prototype.slice.call(
      document.querySelectorAll(
        'img.svg-decorative[src$=".svg"][aria-hidden="true"], img.svg-decorative[src$=".svg"][alt=""]'
      )
    );
    if (!candidates.length) return;

    if (!('IntersectionObserver' in window)) return;

    var observer = new IntersectionObserver(
      function (entries) {
        entries.forEach(function (entry) {
          if (!entry.isIntersecting) return;
          var image = entry.target;
          var deferredSrc = image.getAttribute('data-src');
          if (deferredSrc) {
            image.setAttribute('src', deferredSrc);
            image.removeAttribute('data-src');
          }
          observer.unobserve(image);
        });
      },
      { rootMargin: '220px 0px' }
    );

    candidates.forEach(function (image) {
      if (image.closest('.page-band') || image.closest('.site-nav') || image.closest('.site-brand')) return;
      var src = image.getAttribute('src');
      if (!src) return;
      image.setAttribute('data-src', src);
      image.setAttribute('src', 'data:image/gif;base64,R0lGODlhAQABAIAAAAAAAP///ywAAAAAAQABAAACAUwAOw==');
      observer.observe(image);
    });
  }


  function initServiceWorker() {
    if ('serviceWorker' in navigator) {
      navigator.serviceWorker.register('sw.js').catch(function () {});
    }
  }

  function initBreadcrumbs() {
    var article = document.querySelector('.doc-content');
    if (!article) return;
    var nav = document.createElement('nav');
    nav.className = 'breadcrumbs';
    nav.setAttribute('aria-label','Breadcrumb');
    var file = window.location.pathname.split('/').pop() || 'index.html';
    nav.innerHTML = '<a href="index.html">Home</a> <span>/</span> <span>'+file.replace('.html','')+'</span>';
    article.insertBefore(nav, article.firstChild);
  }

  function initContrastToggle() {
    var key='docs-contrast'; var header=document.querySelector('.site-header'); if(!header) return;
    var b=document.createElement('button'); b.className='contrast-toggle'; b.type='button'; b.textContent='Contraste';
    if(localStorage.getItem(key)==='high') document.body.classList.add('high-contrast');
    b.addEventListener('click', function(){document.body.classList.toggle('high-contrast');localStorage.setItem(key,document.body.classList.contains('high-contrast')?'high':'normal');});
    header.appendChild(b);
  }

  function initLocalSearch() {
    var header = document.querySelector('.site-header');
    if (!header) return;

    var form = document.createElement('form');
    form.className = 'doc-search';
    form.setAttribute('role', 'search');

    var input = document.createElement('input');
    input.className = 'doc-search-input';
    input.type = 'search';
    input.placeholder = 'Search docs';
    input.setAttribute('aria-label', 'Search docs');

    var results = document.createElement('div');
    results.className = 'doc-search-results';
    results.hidden = true;

    form.appendChild(input);
    form.appendChild(results);
    header.appendChild(form);

    var pages = [];
    fetch('search-index.json').then(function(r){return r.json();}).then(function(data){ pages=(data.pages||[]); version=(data.version||'v1'); })
      .catch(function(){ pages=[]; });

    var version='v1';

    function render(query) {
      var q = query.trim().toLowerCase();
      if (!q || q.length < 2) {
        results.hidden = true;
        results.innerHTML = '';
        return;
      }
      var matches = pages.filter(function (p) {
        return p.title.toLowerCase().indexOf(q) >= 0 || p.content.toLowerCase().indexOf(q) >= 0;
      }).slice(0, 8);

      if (!matches.length) {
        results.hidden = false;
        results.innerHTML = '<p class="doc-search-empty">No results</p>';
        return;
      }

      results.hidden = false;
      results.innerHTML = matches.map(function (m) {
        return '<a class="doc-search-item" href="' + m.path + '"><strong>' + m.title + '</strong><span>' + m.path + '</span></a>';
      }).join('');
    }

    input.addEventListener('input', function () { render(input.value); });
    document.addEventListener('click', function (event) {
      if (!(event.target instanceof Element)) return;
      if (!form.contains(event.target)) {
        results.hidden = true;
      }
    });
  }
  document.addEventListener('DOMContentLoaded', function () {
    initServiceWorker();
    initBreadcrumbs();
    initContrastToggle();
    initLocalSearch();
    initLanguageSelector();
    initTextSizeToggle();
    initMobileMenu();
    wrapTables();
    initFloatingToc();
    initBackToTop();
    lazyLoadDecorativeSvgs();
  });
})();
