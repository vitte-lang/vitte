"use strict";
// completion.ts — complétions LSP sans dépendance externe, robustes et contextuelles
var __createBinding = (this && this.__createBinding) || (Object.create ? (function(o, m, k, k2) {
    if (k2 === undefined) k2 = k;
    var desc = Object.getOwnPropertyDescriptor(m, k);
    if (!desc || ("get" in desc ? !m.__esModule : desc.writable || desc.configurable)) {
      desc = { enumerable: true, get: function() { return m[k]; } };
    }
    Object.defineProperty(o, k2, desc);
}) : (function(o, m, k, k2) {
    if (k2 === undefined) k2 = k;
    o[k2] = m[k];
}));
var __setModuleDefault = (this && this.__setModuleDefault) || (Object.create ? (function(o, v) {
    Object.defineProperty(o, "default", { enumerable: true, value: v });
}) : function(o, v) {
    o["default"] = v;
});
var __importStar = (this && this.__importStar) || (function () {
    var ownKeys = function(o) {
        ownKeys = Object.getOwnPropertyNames || function (o) {
            var ar = [];
            for (var k in o) if (Object.prototype.hasOwnProperty.call(o, k)) ar[ar.length] = k;
            return ar;
        };
        return ownKeys(o);
    };
    return function (mod) {
        if (mod && mod.__esModule) return mod;
        var result = {};
        if (mod != null) for (var k = ownKeys(mod), i = 0; i < k.length; i++) if (k[i] !== "default") __createBinding(result, mod, k[i]);
        __setModuleDefault(result, mod);
        return result;
    };
})();
Object.defineProperty(exports, "__esModule", { value: true });
exports.provideCompletions = provideCompletions;
exports.resolveCompletion = resolveCompletion;
exports.triggerCharacters = triggerCharacters;
const path = __importStar(require("node:path"));
const node_url_1 = require("node:url");
const node_1 = require("vscode-languageserver/node");
const languageFacts_js_1 = require("./languageFacts.js");
const indexer_js_1 = require("./indexer.js");
/* ============================================================================
 * Tables de base
 * ========================================================================== */
// Heuristiques de membres courants, proposées après un point.
const COMMON_MEMBERS = [
    { recv: /(str(ing)?|text)$/i, members: ["len", "is_empty()", "trim()", "to_upper()", "to_lower()", "slice(${1:start}, ${2:end})", "as_bytes()"] },
    { recv: /(vec|list|array|slice)$/i, members: ["len", "is_empty()", "push(${1:x})", "pop()", "clear()", "iter()"] },
    { recv: /(map|dict|table)$/i, members: ["len", "get(${1:key})", "set(${1:key}, ${2:val})", "remove(${1:key})", "keys()", "values()"] }
];
// Idées d’imports rapides. Utiles lorsque la ligne commence par `import `.
const QUICK_IMPORTS = [
    "std.core",
    "std.io",
    "std.net",
    "std.fs",
    "std.math",
    "std.time",
    "std.testing",
    "std.os",
    "std.user"
];
const BUILTIN_FUNCTIONS = [
    "print",
    "println",
    "len",
    "size",
    "slice",
    "as_bytes",
    "push",
    "pop",
    "clear",
    "iter",
    "map",
    "filter",
    "reduce",
    "open",
    "read",
    "write",
    "close",
    "assert",
    "panic",
];
const SYMBOL_KIND_LABEL = {
    [node_1.SymbolKind.File]: "File",
    [node_1.SymbolKind.Module]: "Module",
    [node_1.SymbolKind.Namespace]: "Namespace",
    [node_1.SymbolKind.Package]: "Package",
    [node_1.SymbolKind.Class]: "Class",
    [node_1.SymbolKind.Method]: "Method",
    [node_1.SymbolKind.Property]: "Property",
    [node_1.SymbolKind.Field]: "Field",
    [node_1.SymbolKind.Constructor]: "Constructor",
    [node_1.SymbolKind.Enum]: "Enum",
    [node_1.SymbolKind.Interface]: "Interface",
    [node_1.SymbolKind.Function]: "Function",
    [node_1.SymbolKind.Variable]: "Variable",
    [node_1.SymbolKind.Constant]: "Constant",
    [node_1.SymbolKind.String]: "String",
    [node_1.SymbolKind.Number]: "Number",
    [node_1.SymbolKind.Boolean]: "Boolean",
    [node_1.SymbolKind.Array]: "Array",
    [node_1.SymbolKind.Object]: "Object",
    [node_1.SymbolKind.Key]: "Key",
    [node_1.SymbolKind.Null]: "Null",
    [node_1.SymbolKind.EnumMember]: "EnumMember",
    [node_1.SymbolKind.Struct]: "Struct",
    [node_1.SymbolKind.Event]: "Event",
    [node_1.SymbolKind.Operator]: "Operator",
    [node_1.SymbolKind.TypeParameter]: "TypeParameter"
};
/* ============================================================================
 * Snippets
 * ========================================================================== */
const SNIPPETS = [
    ciSnippet("module", "Déclare un module", "Déclare le module courant.", "module ${1:my.module};"),
    ciSnippet("space", "Déclare un espace", "Déclare l’espace courant.", "space ${1:std/os/user}"),
    ciSnippet("import", "Importe un module", "Importe un chemin depuis un autre module.", "import ${1:std::core};"),
    ciSnippet("use", "Importe un symbole", "Importe un chemin depuis un module.", "use ${1:std/core/types.i32}"),
    ciSnippet("fn", "Déclare une fonction", "Fonction avec paramètres et type de retour optionnel.", "fn ${1:name}(${2:params})${3: -> ${4:Type}} {\n\t$0\n}"),
    ciSnippet("proc", "Déclare une procédure", "Procédure avec paramètres et type de retour optionnel.", "proc ${1:name}(${2:params})${3: -> ${4:Type}} {\n\t$0\n}"),
    ciSnippet("docfn", "Doc + fonction", "Ajoute une docstring puis une fonction.", "/// ${1:Summary}\n///\n/// Params:\n/// - ${2:param}: ${3:description}\n/// Returns: ${4:description}\n/// Example:\n/// ${5:example}\nfn ${6:name}(${7:params})${8: -> ${9:Type}} {\n\t$0\n}"),
    ciSnippet("docproc", "Doc + procédure", "Ajoute une docstring puis une procédure.", "/// ${1:Summary}\n///\n/// Params:\n/// - ${2:param}: ${3:description}\n/// Returns: ${4:description}\n/// Example:\n/// ${5:example}\nproc ${6:name}(${7:params})${8: -> ${9:Type}} {\n\t$0\n}"),
    ciSnippet("main", "Point d’entrée", "Déclare la fonction principale.", "fn main() {\n\t$0\n}"),
    ciSnippet("struct", "Déclare une struct", "Structure avec des champs typés.", "struct ${1:Name} {\n\t${2:field}: ${3:Type},\n}"),
    ciSnippet("form", "Déclare un form", "Structure avec des champs typés.", "form ${1:Name} {\n\t${2:field}: ${3:Type},\n}"),
    ciSnippet("docstruct", "Doc + struct", "Ajoute une docstring puis une struct.", "/// ${1:Summary}\n///\n/// Fields:\n/// - ${2:field}: ${3:description}\n/// Example:\n/// ${4:example}\nstruct ${5:Name} {\n\t${6:field}: ${7:Type},\n}"),
    ciSnippet("docform", "Doc + form", "Ajoute une docstring puis un form.", "/// ${1:Summary}\n///\n/// Fields:\n/// - ${2:field}: ${3:description}\n/// Example:\n/// ${4:example}\nform ${5:Name} {\n\t${6:field}: ${7:Type},\n}"),
    ciSnippet("externproc", "Procédure externe", "Déclare une procédure externe.", "#[extern]\nproc ${1:name}(${2:params})${3: -> ${4:Type}}"),
    ciSnippet("enum", "Déclare une enum", "Énumération avec variantes.", "enum ${1:Name} {\n\t${2:Variant1},\n\t${3:Variant2}\n}"),
    ciSnippet("union", "Déclare une union", "Union tagged simple.", "union ${1:Name} {\n\t${2:field}: ${3:Type},\n}"),
    ciSnippet("type", "Alias de type", "Déclare un alias de type.", "type ${1:Alias} = ${2:Existing};"),
    ciSnippet("match", "Match expression", "Expression de branchement avec motifs.", "match ${1:expr} {\n\t${2:Pattern} => ${3:expr},\n\t_ => ${0:default}\n}"),
    ciSnippet("ifelse", "If / Else", "Structure conditionnelle complète.", "if ${1:cond} {\n\t${2}\n} else {\n\t${0}\n}"),
    ciSnippet("for", "Boucle for", "Boucle for-in sur un itérable.", "for ${1:pat} in ${2:iter} {\n\t$0\n}"),
    ciSnippet("while", "Boucle while", "Boucle conditionnelle.", "while ${1:cond} {\n\t$0\n}"),
    ciSnippet("loop", "Boucle infinie", "Boucle sans fin, utiliser break pour sortir.", "loop {\n\t$0\n}"),
    ciSnippet("let", "Binding local", "Déclare une variable locale.", "let ${1:name}${2: : ${3:Type}}${4: = ${5:value}};"),
    ciSnippet("const", "Constante", "Déclare une constante globale.", "const ${1:NAME}: ${2:Type} = ${3:value};"),
    ciSnippet("static", "Statique", "Déclare un statique global.", "static ${1:NAME}: ${2:Type} = ${3:value};"),
    ciSnippet("print", "Affichage console", "Affiche un message sur la sortie standard.", "println(\"${1:msg}\");"),
    ciSnippet("doc", "Doc comment", "Insert a documentation comment.", "/// ${1:Summary}\n///\n/// ${0:Details}")
];
function extractSymbols(doc) {
    const text = doc.getText();
    const rules = [
        { rx: /^\s*module\s+([A-Za-z_][\w./:]*)/gm, kind: node_1.SymbolKind.Namespace, g: 1 },
        { rx: /^\s*space\s+([A-Za-z_][\w./:]*)/gm, kind: node_1.SymbolKind.Namespace, g: 1 },
        { rx: /^\s*(?:pub\s+)?(?:fn|proc)\s+([A-Za-z_]\w*)/gm, kind: node_1.SymbolKind.Function, g: 1 },
        { rx: /^\s*(?:pub\s+)?struct\s+([A-Za-z_]\w*)/gm, kind: node_1.SymbolKind.Struct, g: 1 },
        { rx: /^\s*(?:pub\s+)?form\s+([A-Za-z_]\w*)/gm, kind: node_1.SymbolKind.Struct, g: 1 },
        { rx: /^\s*(?:pub\s+)?enum\s+([A-Za-z_]\w*)/gm, kind: node_1.SymbolKind.Enum, g: 1 },
        { rx: /^\s*(?:pub\s+)?union\s+([A-Za-z_]\w*)/gm, kind: node_1.SymbolKind.Struct, g: 1 },
        { rx: /^\s*(?:pub\s+)?type\s+([A-Za-z_]\w*)/gm, kind: node_1.SymbolKind.Interface, g: 1 },
        { rx: /^\s*(?:pub\s+)?const\s+([A-Za-z_]\w*)/gm, kind: node_1.SymbolKind.Constant, g: 1 },
        { rx: /^\s*(?:pub\s+)?static\s+([A-Za-z_]\w*)/gm, kind: node_1.SymbolKind.Variable, g: 1 },
    ];
    const out = [];
    for (const { rx, kind, g } of rules) {
        rx.lastIndex = 0;
        let m;
        while ((m = rx.exec(text))) {
            const name = m[g];
            if (name)
                out.push({ name, kind });
            if (m[0].length === 0)
                rx.lastIndex++;
        }
    }
    const seen = new Set();
    return out.filter(s => {
        const k = `${s.kind}:${s.name}`;
        if (seen.has(k))
            return false;
        seen.add(k);
        return true;
    });
}
function mapSymbolKindToCompletionKind(k) {
    switch (k) {
        case node_1.SymbolKind.Function: return node_1.CompletionItemKind.Function;
        case node_1.SymbolKind.Method: return node_1.CompletionItemKind.Method;
        case node_1.SymbolKind.Struct: return node_1.CompletionItemKind.Struct;
        case node_1.SymbolKind.Enum: return node_1.CompletionItemKind.Enum;
        case node_1.SymbolKind.Interface: return node_1.CompletionItemKind.Interface;
        case node_1.SymbolKind.Namespace: return node_1.CompletionItemKind.Module;
        case node_1.SymbolKind.Class: return node_1.CompletionItemKind.Class;
        case node_1.SymbolKind.Property: return node_1.CompletionItemKind.Property;
        case node_1.SymbolKind.Field: return node_1.CompletionItemKind.Field;
        case node_1.SymbolKind.Variable: return node_1.CompletionItemKind.Variable;
        case node_1.SymbolKind.Constant: return node_1.CompletionItemKind.Constant;
        case node_1.SymbolKind.Constructor: return node_1.CompletionItemKind.Constructor;
        case node_1.SymbolKind.TypeParameter: return node_1.CompletionItemKind.TypeParameter;
        default: return node_1.CompletionItemKind.Text;
    }
}
/* ============================================================================
 * Utilitaires tokenisation et score
 * ========================================================================== */
function getLinePrefix(doc, pos) {
    return doc.getText(node_1.Range.create({ line: pos.line, character: 0 }, pos));
}
function currentToken(doc, pos) {
    const line = doc.getText(node_1.Range.create({ line: pos.line, character: 0 }, { line: pos.line, character: 10000 }));
    const idx = Math.min(pos.character, line.length);
    const left = line.slice(0, idx);
    const right = line.slice(idx);
    const leftMatch = /[A-Za-z_]\w*$/.exec(left);
    const rightMatch = /^\w*/.exec(right);
    const start = leftMatch ? idx - leftMatch[0].length : idx;
    const end = rightMatch ? idx + rightMatch[0].length : idx;
    const token = line.slice(start, end);
    return {
        token,
        range: node_1.Range.create({ line: pos.line, character: start }, { line: pos.line, character: end })
    };
}
function fuzzyScore(candidate, query) {
    if (!query)
        return 1;
    if (candidate.startsWith(query))
        return 2; // boost prefix strict
    let qi = 0;
    for (let i = 0; i < candidate.length && qi < query.length; i++) {
        if (candidate[i].toLowerCase() === query[qi].toLowerCase())
            qi++;
    }
    return 0.5 + qi / (2 * Math.max(1, query.length));
}
/* ============================================================================
 * Complétions contextuelles
 * ========================================================================== */
function diagnosticsCompletion(linePrefix) {
    const items = [];
    if (linePrefix.includes("???")) {
        items.push({
            label: "TODO",
            kind: node_1.CompletionItemKind.Text,
            insertText: "TODO",
            detail: "Remplacer ??? par TODO",
            documentation: md("Transforme la séquence `???` en TODO/FIXME."),
            sortText: tier(0)
        });
    }
    if (/^\s*import\s+/.test(linePrefix)) {
        for (const imp of QUICK_IMPORTS) {
            items.push({
                label: imp,
                kind: node_1.CompletionItemKind.Module,
                detail: "Import rapide",
                documentation: md(`Ajoute l'import \`${imp}\`.`),
                sortText: tier(0, imp)
            });
        }
    }
    return items;
}
function memberCompletion(leftOfCursor) {
    const items = [];
    const memberCtx = /([A-Za-z_]\w*)\.\s*$/.exec(leftOfCursor);
    if (!memberCtx)
        return items;
    const recv = memberCtx[1];
    for (const group of COMMON_MEMBERS) {
        if (group.recv.test(recv)) {
            for (const m of group.members) {
                items.push({
                    label: m,
                    kind: node_1.CompletionItemKind.Method,
                    detail: `Méthode de ${recv}`,
                    documentation: md(`Proposition basée sur le nom \`${recv}\`.`),
                    sortText: tier(0, m),
                    insertText: m,
                    insertTextFormat: /[\$\{]/.test(m) ? node_1.InsertTextFormat.Snippet : node_1.InsertTextFormat.PlainText
                });
            }
        }
    }
    // Fallback générique
    if (items.length === 0) {
        for (const m of ["len()", "is_empty()", "to_string()", "clone()", "dbg()"]) {
            items.push({
                label: m,
                kind: node_1.CompletionItemKind.Method,
                detail: `Méthode`,
                documentation: md("Proposition générique."),
                sortText: tier(1, m),
                insertText: m,
                insertTextFormat: node_1.InsertTextFormat.PlainText
            });
        }
    }
    return items;
}
function workspaceSymbolCompletions(doc, token, range, context) {
    const normalized = token.trim();
    if (!normalized && context === "general")
        return [];
    const limit = context === "import" ? 200 : 80;
    const results = (0, indexer_js_1.searchWorkspaceSymbols)(normalized, limit);
    if (results.length === 0)
        return [];
    const items = [];
    const seen = new Set();
    for (const sym of results) {
        if (!sym?.name)
            continue;
        if (sym.uri === doc.uri)
            continue;
        if (context === "general" && fuzzyScore(sym.name, token) <= 0)
            continue;
        if (context === "import" && !isImportableKind(sym.kind))
            continue;
        const key = `${sym.name}|${sym.uri}|${sym.kind}`;
        if (seen.has(key))
            continue;
        seen.add(key);
        const kind = mapSymbolKindToCompletionKind(sym.kind);
        const detailParts = [`${SYMBOL_KIND_LABEL[sym.kind] ?? "Symbole"}`];
        if (sym.uri) {
            const base = basenameFromUri(sym.uri);
            if (base)
                detailParts.push(base);
        }
        items.push({
            label: sym.name,
            kind,
            detail: detailParts.join(" — "),
            documentation: md(`Symbole workspace \`${sym.name}\`.`),
            sortText: tier(context === "import" ? 0 : 1, `ws:${sym.name}`),
            filterText: sym.name,
            textEdit: edit(range, sym.name),
            labelDetails: context === "import"
                ? { detail: "workspace" }
                : { description: SYMBOL_KIND_LABEL[sym.kind] ?? "symbol" }
        });
        if (context === "general" && items.length >= 40)
            break;
        if (context === "import" && items.length >= 60)
            break;
    }
    return items;
}
function isImportableKind(kind) {
    return kind === indexer_js_1.SK.Module || kind === indexer_js_1.SK.Namespace;
}
function basenameFromUri(uri) {
    if (!uri)
        return "";
    try {
        if (uri.startsWith("file://")) {
            return path.basename((0, node_url_1.fileURLToPath)(uri));
        }
    }
    catch {
        // ignore decoding issues
    }
    try {
        return path.basename(new URL(uri).pathname);
    }
    catch {
        return uri;
    }
}
/* ============================================================================
 * API
 * ========================================================================== */
function provideCompletions(doc, position) {
    const items = [];
    const { token, range } = currentToken(doc, position);
    const linePrefix = getLinePrefix(doc, position);
    // Mots-clés
    for (const kw of languageFacts_js_1.KEYWORDS) {
        const score = fuzzyScore(kw, token);
        if (score > 0) {
            items.push({
                label: kw,
                kind: node_1.CompletionItemKind.Keyword,
                detail: "Mot-clé",
                documentation: md(`Mot-clé du langage \`${kw}\`.`),
                sortText: tier(2 - (score >= 2 ? 1 : 0), kw),
                filterText: kw,
                textEdit: edit(range, kw),
                preselect: kw === "fn" || kw === "let",
                commitCharacters: [" ", "\t", "(", ")", "{", "}", "[", "]", ";", ",", "."],
                labelDetails: { description: "keyword" }
            });
        }
    }
    // Littéraux
    for (const lit of [...languageFacts_js_1.BOOL_LITERALS, ...languageFacts_js_1.NIL_LITERALS]) {
        if (fuzzyScore(lit, token) > 0) {
            items.push({
                label: lit,
                kind: node_1.CompletionItemKind.Value,
                detail: "Littéral",
                documentation: md(`Littéral \`${lit}\`.`),
                sortText: tier(2, `~${lit}`),
                textEdit: edit(range, lit),
                labelDetails: { description: "literal" }
            });
        }
    }
    // Types primitifs
    for (const ty of languageFacts_js_1.PRIMITIVE_TYPES) {
        if (fuzzyScore(ty, token) > 0) {
            items.push({
                label: ty,
                kind: node_1.CompletionItemKind.TypeParameter,
                detail: "Type primitif",
                documentation: md(`Type primitif \`${ty}\`.`),
                sortText: tier(2, `type:${ty}`),
                textEdit: edit(range, ty),
                labelDetails: { description: "type" }
            });
        }
    }
    // Fonctions builtin
    for (const fn of BUILTIN_FUNCTIONS) {
        if (fuzzyScore(fn, token) > 0) {
            items.push({
                label: fn,
                kind: node_1.CompletionItemKind.Function,
                detail: "Builtin",
                documentation: md(`Fonction builtin \`${fn}\`.`),
                sortText: tier(2, `builtin:${fn}`),
                textEdit: edit(range, fn),
                labelDetails: { description: "builtin" }
            });
        }
    }
    // Snippets
    for (const it of SNIPPETS) {
        items.push({
            ...it,
            sortText: tier(3, it.label),
            filterText: it.label
        });
    }
    // Symboles du document
    for (const s of extractSymbols(doc)) {
        const label = s.name;
        if (fuzzyScore(label, token) > 0) {
            items.push({
                label,
                kind: mapSymbolKindToCompletionKind(s.kind),
                detail: `Symbole (${SYMBOL_KIND_LABEL[s.kind] ?? "?"})`,
                documentation: md("Déclaré dans ce document."),
                sortText: tier(1, label),
                filterText: label,
                textEdit: edit(range, label),
                labelDetails: { description: SYMBOL_KIND_LABEL[s.kind] ?? "symbol" }
            });
        }
    }
    // Contexte: membres après un point
    items.push(...memberCompletion(linePrefix));
    // Contexte: import et autres heuristiques
    const isImportContext = /^\s*import\s+/.test(linePrefix);
    items.push(...diagnosticsCompletion(linePrefix).map(ci => ({
        ...ci,
        textEdit: ci.insertText ? undefined : edit(range, ci.label),
        sortText: ci.sortText ?? tier(0, ci.label)
    })));
    if (isImportContext) {
        items.push(...workspaceSymbolCompletions(doc, token, range, "import"));
    }
    else if (token.length >= 2) {
        items.push(...workspaceSymbolCompletions(doc, token, range, "general"));
    }
    return dedupe(items, it => `${it.label}|${it.kind}|${it.sortText ?? ""}`);
}
function resolveCompletion(item) {
    if (!item.documentation && typeof item.label === "string") {
        item.documentation = md(`Entrée de complétion \`${item.label}\`.`);
    }
    return item;
}
function triggerCharacters() {
    return [".", ":", ">", "=", " ", "(", "[", "{", "\"", "'"];
}
/* ============================================================================
 * Helpers
 * ========================================================================== */
function ciSnippet(label, detail, documentation, insertText) {
    return {
        label,
        kind: node_1.CompletionItemKind.Snippet,
        insertText,
        insertTextFormat: node_1.InsertTextFormat.Snippet,
        detail,
        documentation: md(documentation)
    };
}
function tier(n, suffix = "") {
    // 0 = top, 1 = symboles, 2 = mots-clés/littéraux, 3 = snippets
    return `${n.toString(10).padStart(2, "0")}_${suffix}`;
}
function md(value) {
    return { kind: node_1.MarkupKind.Markdown, value };
}
function edit(range, newText) {
    return { range, newText };
}
function dedupe(arr, key) {
    const seen = new Set();
    const out = [];
    for (const el of arr) {
        const k = key(el);
        if (seen.has(k))
            continue;
        seen.add(k);
        out.push(el);
    }
    return out;
}
//# sourceMappingURL=completion.js.map