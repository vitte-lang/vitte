(function () {
  const root = document.documentElement;
  const button = document.getElementById('theme-toggle');
  if (!button) return;

  const label = button.querySelector('[data-theme-label]');

  function applyTheme(theme, persist) {
    const activeTheme = theme === 'light' ? 'light' : 'dark';
    const nextTheme = activeTheme === 'dark' ? 'clair' : 'sombre';
    root.dataset.theme = activeTheme;
    const themeColor = document.querySelector('meta[name="theme-color"]');
    if (themeColor) themeColor.content = activeTheme === 'dark' ? '#0c1313' : '#f5f7f2';
    button.setAttribute('aria-pressed', String(activeTheme === 'light'));
    button.setAttribute('aria-label', `Activer le thème ${nextTheme}`);
    if (label) label.textContent = nextTheme.charAt(0).toUpperCase() + nextTheme.slice(1);

    if (persist) {
      try {
        window.localStorage.setItem('vitte-docs-theme', activeTheme);
      } catch (_) {}
    }
  }

  applyTheme(root.dataset.theme, false);
  button.addEventListener('click', () => {
    applyTheme(root.dataset.theme === 'dark' ? 'light' : 'dark', true);
  });
}());
