(function () {
  const sidebar = document.querySelector('.sidebar');
  const drawer = sidebar && sidebar.querySelector('.sidebar-drawer');
  if (!sidebar || !drawer) return;

  const compactNavigation = window.matchMedia('(max-width: 850px)');
  const drawerAction = drawer.querySelector('.drawer-action');
  const activeLink = sidebar.querySelector('.is-active');

  function updateDrawerLabel() {
    if (drawerAction) drawerAction.textContent = drawer.open ? 'Masquer le menu' : 'Afficher le menu';
  }

  function revealActiveLink() {
    if (!activeLink || !drawer.open) return;
    const parentSection = activeLink.closest('.sidebar-section');
    if (parentSection) parentSection.open = true;

    requestAnimationFrame(() => {
      const sidebarBounds = sidebar.getBoundingClientRect();
      const linkBounds = activeLink.getBoundingClientRect();
      if (linkBounds.top < sidebarBounds.top || linkBounds.bottom > sidebarBounds.bottom) {
        activeLink.scrollIntoView({ block: 'nearest' });
      }
    });
  }

  function syncDrawer() {
    drawer.open = !compactNavigation.matches;
    updateDrawerLabel();
    revealActiveLink();
  }

  syncDrawer();
  drawer.addEventListener('toggle', () => {
    updateDrawerLabel();
    revealActiveLink();
  });
  if (compactNavigation.addEventListener) compactNavigation.addEventListener('change', syncDrawer);
  else compactNavigation.addListener(syncDrawer);
}());
