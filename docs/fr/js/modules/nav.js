export function initMobileMenu() {
  const nav = document.querySelector('.site-nav');
  const header = document.querySelector('.site-header');
  if (!nav || !header) return;
  if (!nav.id) nav.id = 'primary-nav';
  const button = document.createElement('button');
  button.type = 'button'; button.className = 'menu-toggle';
  button.setAttribute('aria-expanded', 'false'); button.setAttribute('aria-controls', nav.id);
  button.textContent = 'Menu';
  header.insertBefore(button, nav);
  button.addEventListener('click', () => {
    const expanded = button.getAttribute('aria-expanded') === 'true';
    button.setAttribute('aria-expanded', String(!expanded));
    nav.classList.toggle('is-open', !expanded);
  });
}
