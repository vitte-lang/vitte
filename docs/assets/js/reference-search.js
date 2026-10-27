(function () {
  const input = document.getElementById('reference-search');
  const cards = Array.from(document.querySelectorAll('.reference-card'));
  const noResults = document.getElementById('reference-no-results');
  const status = document.getElementById('reference-search-status');

  if (!input || !cards.length || !noResults || !status) return;

  function decodeEntities(value) {
    return value
      .replace(/&amp;/g, '&')
      .replace(/&lt;/g, '<')
      .replace(/&gt;/g, '>')
      .replace(/&quot;/g, '"')
      .replace(/&#39;/g, "'")
      .replace(/&nbsp;/g, ' ');
  }

  function normalize(value) {
    return decodeEntities(value)
      .toLocaleLowerCase('fr-FR')
      .normalize('NFD')
      .replace(/[\u0300-\u036f]/g, '')
      .replace(/\s+/g, ' ')
      .trim();
  }

  const index = cards.map((card) => {
    const contentNode = card.querySelector('.reference-card-search-content');
    const hitNode = card.querySelector('.reference-card-hit');
    const content = contentNode ? decodeEntities(contentNode.textContent) : '';

    return {
      card,
      content,
      normalizedContent: normalize(content),
      searchable: normalize(`${card.dataset.search || ''} ${content}`),
      hitNode
    };
  });

  function makeExcerpt(content, term) {
    const words = content.replace(/\s+/g, ' ').trim().split(' ');
    const matchingWord = words.findIndex((word) => normalize(word).includes(term));
    if (matchingWord < 0) return '';

    const start = Math.max(0, matchingWord - 5);
    const excerpt = words.slice(start, matchingWord + 11).join(' ');
    return `${start > 0 ? '…' : ''}${excerpt}${matchingWord + 11 < words.length ? '…' : ''}`;
  }

  function updateResults() {
    const query = normalize(input.value);
    const queryTerms = query.split(' ').filter(Boolean);
    let visible = 0;

    index.forEach((entry) => {
      const matches = !queryTerms.length || queryTerms.every((term) => entry.searchable.includes(term));
      entry.card.hidden = !matches;
      if (entry.hitNode) {
        const contentTerm = queryTerms.find((term) => entry.normalizedContent.includes(term));
        const excerpt = matches && contentTerm ? makeExcerpt(entry.content, contentTerm) : '';
        entry.hitNode.textContent = excerpt ? `Dans le contenu : « ${excerpt} »` : '';
        entry.hitNode.hidden = !excerpt;
      }
      if (matches) visible += 1;
    });

    noResults.hidden = visible !== 0;
    status.textContent = query
      ? `${visible} page${visible > 1 ? 's' : ''} trouvée${visible > 1 ? 's' : ''}`
      : `${cards.length} fiches · recherche dans les titres, résumés et contenus`;
  }

  input.addEventListener('input', updateResults);
  document.addEventListener('keydown', (event) => {
    if (event.key === '/' && !['INPUT', 'TEXTAREA'].includes(document.activeElement.tagName)) {
      event.preventDefault();
      input.focus();
    }
    if (event.key === 'Escape' && document.activeElement === input) {
      input.value = '';
      updateResults();
      input.blur();
    }
  });
}());
