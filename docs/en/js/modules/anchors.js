export function initAnchors() {
  document.querySelectorAll('.doc-content h2, .doc-content h3').forEach((h, i) => {
    if (!h.id) h.id = `section-${i + 1}`;
    const a = document.createElement('button');
    a.className = 'anchor-copy'; a.type = 'button'; a.textContent = '#';
    a.setAttribute('aria-label', 'Copy section link');
    a.addEventListener('click', async () => {
      const url = `${location.origin}${location.pathname}#${h.id}`;
      try { await navigator.clipboard.writeText(url); a.textContent = '✓'; setTimeout(() => a.textContent = '#', 900); } catch {}
    });
    h.appendChild(a);
  });
}
