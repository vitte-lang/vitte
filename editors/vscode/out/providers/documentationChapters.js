"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.DOC_SECTIONS = exports.DOCS_BASE_URL = void 0;
exports.makeDocUrl = makeDocUrl;
exports.DOCS_BASE_URL = "https://vitte.netlify.app";
exports.DOC_SECTIONS = [
    {
        title: "Debutant",
        chapters: [
            { title: "Vitte Book", path: "/book/" },
            { title: "1. Demarrer", path: "/book/01-demarrer/" },
            { title: "4. Syntaxe de base", path: "/book/04-syntaxe/" },
            { title: "6. Fonctions et procedures", path: "/book/06-procedures/" }
        ]
    },
    {
        title: "Intermediaire",
        chapters: [
            { title: "10. Erreurs et diagnostics", path: "/book/10-diagnostics/" },
            { title: "Guide CLI", path: "/pages/cli/" },
            { title: "Guide Stdlib", path: "/pages/stdlib/" },
            { title: "Guide Grammar", path: "/pages/grammar/" }
        ]
    },
    {
        title: "Avance",
        chapters: [
            { title: "15. HIR, MIR et pipeline", path: "/book/15-pipeline/" },
            { title: "18. Tests et outillage", path: "/book/18-tests/" },
            { title: "19. Performance et profiling", path: "/book/19-performance/" },
            { title: "Guide Errors", path: "/pages/errors/" }
        ]
    },
    {
        title: "Professionnel",
        chapters: [
            { title: "21. Projet CLI robuste", path: "/book/21-projet-cli/" },
            { title: "22. Projet mini-serveur", path: "/book/22-projet-http/" },
            { title: "23. Projet utilitaire systeme", path: "/book/23-projet-sys/" },
            { title: "24. Projet base key-value", path: "/book/24-projet-kv/" }
        ]
    }
];
function makeDocUrl(path) {
    if (path.startsWith("http://") || path.startsWith("https://"))
        return path;
    const normalized = path.startsWith("/") ? path : `/${path}`;
    return `${exports.DOCS_BASE_URL}${normalized}`;
}
//# sourceMappingURL=documentationChapters.js.map