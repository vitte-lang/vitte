(function () {
  const sidebar = document.querySelector('.sidebar');
  const activeLink = sidebar && sidebar.querySelector('.is-active');
  if (!sidebar || !activeLink) return;

  const parentSection = activeLink.closest('details');
  if (parentSection) parentSection.open = true;

  requestAnimationFrame(() => {
    const sidebarBounds = sidebar.getBoundingClientRect();
    const linkBounds = activeLink.getBoundingClientRect();
    if (linkBounds.top < sidebarBounds.top || linkBounds.bottom > sidebarBounds.bottom) {
      activeLink.scrollIntoView({ block: 'nearest' });
    }
  });
}());
