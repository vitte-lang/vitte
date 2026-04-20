// =========================================================
// Vitte Site Script (routing + diagnostics + basic UX)
// =========================================================

// Redirect diagnostics subdomain to /diagnostics/
(function () {
  if (window.location.hostname === "diagnostics.vitte-lang.org") {
    if (!window.location.pathname.startsWith("/site/docs/diagnostics")) {
      window.location.replace("/site/docs/diagnostics/");
    }
  }
})();

// Simple client-side router (optional future use)
function navigate(path) {
  window.history.pushState({}, "", path);
  renderRoute();
}

window.addEventListener("popstate", renderRoute);

function renderRoute() {
  const path = window.location.pathname;

  // basic routing hooks (expand later)
  if (path.startsWith("/site/docs/diagnostics")) {
    document.body.classList.add("diagnostics-mode");
  } else {
    document.body.classList.remove("diagnostics-mode");
  }
}

// Init on load
window.addEventListener("DOMContentLoaded", function () {
  renderRoute();

  // Attach navigation to internal links
  document.querySelectorAll("a[href^='/']").forEach(link => {
    link.addEventListener("click", function (e) {
      // skip external / target links
      if (link.target === "_blank") return;

      e.preventDefault();
      navigate(link.getAttribute("href"));
    });
  });
});

// Smooth scroll for anchors
(function () {
  document.querySelectorAll("a[href^='#']").forEach(anchor => {
    anchor.addEventListener("click", function (e) {
      const target = document.querySelector(this.getAttribute("href"));
      if (target) {
        e.preventDefault();
        target.scrollIntoView({ behavior: "smooth" });
      }
    });
  });
})();

// Basic diagnostics helper (future use)
function showError(code, message) {
  console.error("[Vitte] " + code + ": " + message);
}
