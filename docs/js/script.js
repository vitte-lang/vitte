

// =========================================================
// Vitte Site Script (routing + diagnostics + menu UX)
// =========================================================

// Redirect diagnostics subdomain
(function () {
  if (window.location.hostname === "diagnostics.vitte-lang.org") {
    if (!window.location.pathname.startsWith("/site/diagnostics.html")) {
      window.location.replace("/site/diagnostics.html");
    }
  }
})();

// Simple router
function navigate(path) {
  window.history.pushState({}, "", path);
  renderRoute();
}

window.addEventListener("popstate", renderRoute);

function renderRoute() {
  const path = window.location.pathname;

  if (path === "/site/diagnostics.html") {
    document.body.classList.add("diagnostics-mode");
  } else {
    document.body.classList.remove("diagnostics-mode");
  }
}

// Init
window.addEventListener("DOMContentLoaded", function () {
  renderRoute();

  // Internal links navigation
  document.querySelectorAll("a[href^='/']").forEach(link => {
    link.addEventListener("click", function (e) {
      if (link.target === "_blank") return;

      e.preventDefault();
      navigate(link.getAttribute("href"));
    });
  });

  // Dropdown menu logic
  const menuBtn = document.querySelector(".menu-button");
  const dropdown = document.getElementById("dropdown");

  if (menuBtn && dropdown) {
    menuBtn.addEventListener("click", function () {
      dropdown.style.display = dropdown.style.display === "block" ? "none" : "block";
    });

    window.addEventListener("click", function (e) {
      if (!e.target.closest('.menu')) {
        dropdown.style.display = "none";
      }
    });
  }
});

// Smooth scroll
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

// Diagnostics helper
function showError(code, message) {
  console.error("[Vitte] " + code + ": " + message);
}
