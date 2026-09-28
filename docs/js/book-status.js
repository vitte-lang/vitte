(function () {
  var out = document.getElementById("book-status-out");
  if (!out) {
    return;
  }

  fetch("chapters-meta.json")
    .then(function (response) {
      return response.json();
    })
    .then(function (data) {
      var chapters = data.chapters || [];
      var levels = { beginner: 0, intermediate: 0, advanced: 0 };
      chapters.forEach(function (chapter) {
        levels[chapter.level] = (levels[chapter.level] || 0) + 1;
      });
      out.textContent = JSON.stringify(
        {
          chapters: chapters.length,
          levels: levels,
          essential: chapters.filter(function (chapter) {
            return chapter.essential;
          }).length,
        },
        null,
        2
      );
    })
    .catch(function () {
      out.textContent = JSON.stringify(
        { error: "Unable to load chapters-meta.json" },
        null,
        2
      );
    });
})();
