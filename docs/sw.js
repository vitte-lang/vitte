const CACHE = 'vitte-docs-v1';
const ASSETS = [
  '/', '/index.html', '/doc.html', '/search-index.json',
  '/css/site.css', '/js/site-interactions.js', '/svg/logo.svg', '/svg/sprite.svg'
];
self.addEventListener('install', (e) => {
  e.waitUntil(caches.open(CACHE).then((c) => c.addAll(ASSETS)).then(() => self.skipWaiting()));
});
self.addEventListener('activate', (e) => {
  e.waitUntil(caches.keys().then((keys) => Promise.all(keys.filter((k) => k !== CACHE).map((k) => caches.delete(k)))));
  self.clients.claim();
});
self.addEventListener('fetch', (e) => {
  e.respondWith(caches.match(e.request).then((r) => r || fetch(e.request).then((resp) => {
    const copy = resp.clone();
    caches.open(CACHE).then((c) => c.put(e.request, copy));
    return resp;
  }).catch(() => caches.match('/index.html'))));
});
