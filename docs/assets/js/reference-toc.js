(function () {
  const body = document.querySelector('.reference-body, .lesson-body');
  const nav = document.querySelector('.reference-toc, .lesson-toc');
  if (!body || !nav) return;

  const headings = Array.from(body.querySelectorAll('h2'));
  const list = nav.querySelector('ul');
  if (headings.length < 3 || !list) return;

  function slugify(value) {
    return value.toLocaleLowerCase('fr-FR')
      .normalize('NFD')
      .replace(/[\u0300-\u036f]/g, '')
      .replace(/[^a-z0-9]+/g, '-')
      .replace(/^-|-$/g, '');
  }

  headings.forEach((heading, index) => {
    if (!heading.id) heading.id = `section-${index + 1}-${slugify(heading.textContent)}`;
    const item = document.createElement('li');
    const link = document.createElement('a');
    link.href = `#${heading.id}`;
    link.textContent = heading.textContent;
    item.appendChild(link);
    list.appendChild(item);
  });

  nav.hidden = false;
}());
