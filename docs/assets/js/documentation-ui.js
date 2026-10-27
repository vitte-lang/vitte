(function () {
  function copyFallback(value) {
    return new Promise((resolve, reject) => {
      const textarea = document.createElement('textarea');
      textarea.value = value;
      textarea.setAttribute('readonly', '');
      textarea.style.position = 'fixed';
      textarea.style.opacity = '0';
      const previousFocus = document.activeElement;
      document.body.appendChild(textarea);
      textarea.focus();
      textarea.select();
      textarea.setSelectionRange(0, textarea.value.length);
      let copied = false;
      try {
        copied = document.execCommand('copy');
      } finally {
        textarea.remove();
        if (previousFocus && typeof previousFocus.focus === 'function') previousFocus.focus();
      }
      copied ? resolve() : reject(new Error('Copy failed'));
    });
  }

  function copyText(value) {
    if (navigator.clipboard && window.isSecureContext) {
      return navigator.clipboard.writeText(value).catch(() => copyFallback(value));
    }
    return copyFallback(value);
  }

  document.querySelectorAll('pre').forEach((pre) => {
    const code = pre.querySelector('code') || pre;
    if (pre.closest('.code-block')) return;

    const wrapper = document.createElement('div');
    wrapper.className = 'code-block';
    pre.parentNode.insertBefore(wrapper, pre);
    wrapper.appendChild(pre);
    pre.classList.add('copyable-code');

    const button = document.createElement('button');
    button.type = 'button';
    button.className = 'copy-code-button';
    button.textContent = 'Copier';
    button.setAttribute('aria-label', 'Copier ce bloc de code');
    button.setAttribute('aria-live', 'polite');
    button.addEventListener('click', async () => {
      try {
        await copyText(code.textContent || '');
        button.textContent = 'Copié';
        button.dataset.copyState = 'copied';
      } catch (error) {
        button.textContent = 'Copie impossible';
        button.dataset.copyState = 'error';
      }
      button.setAttribute('aria-label', button.textContent);
      window.setTimeout(() => {
        button.textContent = 'Copier';
        button.setAttribute('aria-label', 'Copier ce bloc de code');
        delete button.dataset.copyState;
      }, 1800);
    });
    wrapper.appendChild(button);
  });

  document.querySelectorAll('.lesson-body table, .reference-body table').forEach((table, index) => {
    if (table.parentElement.classList.contains('table-scroll')) return;

    const region = document.createElement('div');
    region.className = 'table-scroll';
    region.tabIndex = 0;
    region.setAttribute('role', 'region');
    const caption = table.querySelector('caption');
    region.setAttribute('aria-label', `Tableau défilant ${caption ? `: ${caption.textContent.trim()}` : index + 1}`);
    table.classList.add('scroll-table');
    table.parentNode.insertBefore(region, table);
    region.appendChild(table);
  });

  document.querySelectorAll('a[href]').forEach((link) => {
    const rawHref = link.getAttribute('href') || '';
    if (!rawHref || rawHref.startsWith('#')) return;
    if (link.closest('.topbar-nav, .sidebar, .breadcrumbs, .site-footer')) return;

    let destination;
    try {
      destination = new URL(rawHref, window.location.href);
    } catch (error) {
      return;
    }

    if (destination.hostname === 'github.com' || destination.hostname.endsWith('.github.com')) {
      link.classList.add('github-link');
      if (/[↗→]\s*$/.test(link.textContent.trim())) link.classList.add('has-link-cue');
      if (!link.querySelector('.visually-hidden')) {
        const note = document.createElement('span');
        note.className = 'visually-hidden';
        note.textContent = ' (lien externe vers GitHub)';
        link.appendChild(note);
      }
      return;
    }

    if (destination.origin === window.location.origin) {
      link.classList.add('internal-link');
      if (/[↗→]\s*$/.test(link.textContent.trim())) link.classList.add('has-link-cue');
    }
  });
}());
