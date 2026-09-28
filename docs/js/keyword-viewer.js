const params = new URLSearchParams(window.location.search);
const keyword = (params.get("id") || "all").trim();
const article = document.getElementById("keyword-article");

if (article) {
  fetch(`json/${encodeURIComponent(keyword)}.json`)
    .then((response) => {
      if (!response.ok) {
        throw new Error("keyword json not found");
      }
      return response.json();
    })
    .then((data) => {
      document.title = data.title;
      article.innerHTML = data.body_html;
    })
    .catch(() => {
      article.innerHTML =
        "<h1>Keyword Viewer</h1><p>The requested keyword JSON could not be loaded.</p>";
    });
}
