"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.registerCompletion = registerCompletion;
exports.updateConfig = updateConfig;
const node_1 = require("vscode-languageserver/node");
const DEFAULT_CONFIG = {
    enableSnippets: true,
    maxItems: 200,
};
let currentConfig = { ...DEFAULT_CONFIG };
/** Register completion feature */
function registerCompletion(connection, documents) {
    connection.onDidChangeConfiguration((params) => {
        try {
            const cfg = params.settings?.vitte?.completion;
            if (cfg && typeof cfg === 'object') {
                updateConfig(cfg);
            }
        }
        catch {
            // ignore invalid payload
        }
    });
    connection.onCompletion((params) => {
        const doc = documents.get(params.textDocument.uri);
        if (!doc)
            return emptyList();
        return completeAt(doc, params);
    });
    // Provide resolve to enrich details/documentation lazily
    connection.onCompletionResolve((item) => {
        if (item.data === 'vitte:snippet:for') {
            item.detail = 'Boucle for (snippet)';
            item.documentation = 'Insère une boucle `for` Vitte avec index et bornes incluses.';
        }
        else if (item.data === 'vitte:kw:fn') {
            item.detail = 'Déclaration de fonction';
        }
        return item;
    });
}
function updateConfig(partial) {
    const next = { ...currentConfig };
    if (typeof partial.enableSnippets === 'boolean')
        next.enableSnippets = partial.enableSnippets;
    if (typeof partial.maxItems === 'number' && Number.isFinite(partial.maxItems) && partial.maxItems > 0)
        next.maxItems = partial.maxItems;
    currentConfig = next;
}
// ---------------- core logic ----------------
function completeAt(doc, params) {
    const { position } = params;
    const prefix = linePrefix(doc, position);
    const items = [];
    // Keywords (subset placeholder — adapt to Vitte real grammar)
    const keywords = [
        { label: 'fn', kind: node_1.CompletionItemKind.Keyword, detail: 'Déclare une fonction', data: 'vitte:kw:fn' },
        { label: 'let', kind: node_1.CompletionItemKind.Keyword, detail: 'Bind immuable' },
        { label: 'mut', kind: node_1.CompletionItemKind.Keyword, detail: 'Mutabilité' },
        { label: 'struct', kind: node_1.CompletionItemKind.Keyword },
        { label: 'enum', kind: node_1.CompletionItemKind.Keyword },
        { label: 'use', kind: node_1.CompletionItemKind.Keyword },
        { label: 'mod', kind: node_1.CompletionItemKind.Keyword },
        { label: 'impl', kind: node_1.CompletionItemKind.Keyword },
        { label: 'return', kind: node_1.CompletionItemKind.Keyword },
        { label: 'if', kind: node_1.CompletionItemKind.Keyword },
        { label: 'else', kind: node_1.CompletionItemKind.Keyword },
        { label: 'for', kind: node_1.CompletionItemKind.Keyword },
        { label: 'while', kind: node_1.CompletionItemKind.Keyword },
    ];
    for (const k of keywords) {
        if (startsWithWord(prefix, k.label)) {
            items.push({
                label: k.label,
                kind: k.kind,
                detail: k.detail,
                data: k.data,
            });
        }
    }
    if (currentConfig.enableSnippets) {
        items.push(...snippetItems(prefix));
    }
    // Cap results
    const limited = items.length > currentConfig.maxItems ? items.slice(0, currentConfig.maxItems) : items;
    return { isIncomplete: false, items: limited };
}
function snippetItems(prefix) {
    const out = [];
    if (startsWithWord(prefix, 'for')) {
        out.push({
            label: 'for (i in 0..N)',
            kind: node_1.CompletionItemKind.Snippet,
            detail: 'Boucle for (0..N)',
            insertTextFormat: node_1.InsertTextFormat.Snippet,
            insertText: 'for (let ${1:i} in 0..${2:N}) {\n    ${3:// ...}\n}',
            data: 'vitte:snippet:for',
        });
    }
    // Function template
    if (startsWithWord(prefix, 'fn')) {
        out.push({
            label: 'fn name(args) -> T {}',
            kind: node_1.CompletionItemKind.Snippet,
            detail: 'Déclaration de fonction',
            insertTextFormat: node_1.InsertTextFormat.Snippet,
            insertText: 'fn ${1:name}(${2:args}) -> ${3:T} {\n    ${4:// body}\n}',
            data: 'vitte:snippet:fn',
        });
    }
    return out;
}
// ---------------- helpers ----------------
function linePrefix(doc, pos) {
    const start = node_1.Position.create(pos.line, 0);
    const r = node_1.Range.create(start, pos);
    const text = doc.getText(r);
    // Keep only the last wordish fragment to compare startsWith
    const m = /([A-Za-z_][A-Za-z0-9_]*)$/.exec(text);
    return m ? m[1] : '';
}
function startsWithWord(prefix, word) {
    if (prefix.length === 0)
        return true; // offer at BOF
    return word.startsWith(prefix);
}
function emptyList() {
    return { isIncomplete: false, items: [] };
}
//# sourceMappingURL=completion.js.map