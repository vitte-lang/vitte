"use strict";
/**
 * Vitte LSP — Language Service (complete minimal)
 * -----------------------------------------------
 * Diagnostics & completion without external parser, with configurable rules.
 * Pure TypeScript. API remains compatible with lsp.ts (doValidation, doComplete, doResolve).
 */
Object.defineProperty(exports, "__esModule", { value: true });
exports.VitteLanguageService = void 0;
const node_1 = require("vscode-languageserver/node");
const DEFAULT_SETTINGS = {
    maxLineLength: 140,
    warnTrailingWhitespace: true,
    hintTodoFixme: true,
    warnMixedIndent: true,
};
// Small helpers
function hasTabsOnly(s) { return /^\t+$/.test(s); }
function hasSpacesOnly(s) { return /^ +$/.test(s); }
// ---------------------------------------------------------------------------
// Language Service
// ---------------------------------------------------------------------------
class VitteLanguageService {
    constructor(opts = {}) {
        this.settings = { ...DEFAULT_SETTINGS, ...opts };
    }
    /** Update settings at runtime without recreating the service. */
    setSettings(partial) {
        this.settings = { ...this.settings, ...partial };
    }
    /** Basic diagnostics: TODO/FIXME/XXX, long lines, trailing spaces, mixed indentation. */
    doValidation(doc) {
        const cfg = this.settings;
        const text = doc.getText();
        const diagnostics = [];
        const lines = text.replace(/\r\n/g, '\n').split('\n');
        let sawTabIndent = false;
        let sawSpaceIndent = false;
        for (let i = 0; i < lines.length; i++) {
            const line = lines[i];
            // Leading indentation sampling for mixed‑indent detection
            const mIndent = /^(\s*)/.exec(line);
            if (mIndent) {
                const lead = mIndent[1];
                if (lead.length > 0) {
                    // Only consider pure tab or pure space runs at the beginning
                    if (hasTabsOnly(lead))
                        sawTabIndent = true;
                    else if (hasSpacesOnly(lead))
                        sawSpaceIndent = true;
                }
            }
            // TODO/FIXME/XXX as Hint
            if (cfg.hintTodoFixme) {
                const m = /(TODO|FIXME|XXX)(:?)(.*)/.exec(line);
                if (m) {
                    diagnostics.push({
                        severity: node_1.DiagnosticSeverity.Hint,
                        message: `Note: ${m[1]}${m[2]}${m[3] ?? ''}`.trim(),
                        range: { start: { line: i, character: m.index }, end: { line: i, character: m.index + m[0].length } },
                        source: 'vitte-lsp',
                        code: 'vitte.todofixme',
                    });
                }
            }
            // Long line
            if (cfg.maxLineLength > 0 && line.length > cfg.maxLineLength) {
                diagnostics.push({
                    severity: node_1.DiagnosticSeverity.Warning,
                    message: `Ligne trop longue (${line.length} > ${cfg.maxLineLength})`,
                    range: { start: { line: i, character: cfg.maxLineLength }, end: { line: i, character: line.length } },
                    source: 'vitte-lsp',
                    code: 'vitte.maxLineLength',
                });
            }
            // Trailing whitespace
            if (cfg.warnTrailingWhitespace) {
                const tw = /(\s+)$/.exec(line);
                if (tw && tw[1].length > 0) {
                    const start = line.length - tw[1].length;
                    diagnostics.push({
                        severity: node_1.DiagnosticSeverity.Warning,
                        message: 'Espace en fin de ligne',
                        range: { start: { line: i, character: start }, end: { line: i, character: line.length } },
                        source: 'vitte-lsp',
                        code: 'vitte.trailingWhitespace',
                    });
                }
            }
        }
        // Mixed indentation (file scope)
        if (cfg.warnMixedIndent && sawTabIndent && sawSpaceIndent) {
            diagnostics.push({
                severity: node_1.DiagnosticSeverity.Warning,
                message: 'Indentation mixte détectée (tabs et espaces). Normalisez votre indentation.',
                range: { start: { line: 0, character: 0 }, end: { line: Math.max(0, lines.length - 1), character: 0 } },
                source: 'vitte-lsp',
                code: 'vitte.mixedIndent',
            });
        }
        return Promise.resolve(diagnostics);
    }
    /**
     * Completions: keywords + simple snippets for declarations.
     * We avoid context parsing; snippets are provided for productivity.
     */
    doComplete(_params) {
        const items = [
            { label: 'module', kind: node_1.CompletionItemKind.Keyword, detail: 'Déclaration de module', insertTextFormat: node_1.InsertTextFormat.Snippet, insertText: 'module ${1:my.module};' },
            { label: 'import', kind: node_1.CompletionItemKind.Keyword, detail: 'Import', insertTextFormat: node_1.InsertTextFormat.Snippet, insertText: 'import ${1:path::to::item};' },
            { label: 'fn', kind: node_1.CompletionItemKind.Keyword, detail: 'Déclaration de fonction', insertTextFormat: node_1.InsertTextFormat.Snippet, insertText: 'fn ${1:name}(${2:args}) {\n\t$0\n}' },
            { label: 'struct', kind: node_1.CompletionItemKind.Keyword, detail: 'Déclaration de structure', insertTextFormat: node_1.InsertTextFormat.Snippet, insertText: 'struct ${1:Name} {\n\t$0\n}' },
            { label: 'enum', kind: node_1.CompletionItemKind.Keyword, detail: 'Déclaration d’énumération', insertTextFormat: node_1.InsertTextFormat.Snippet, insertText: 'enum ${1:Name} {\n\t${2:Variant}\n}' },
            { label: 'union', kind: node_1.CompletionItemKind.Keyword, detail: 'Déclaration d’union', insertTextFormat: node_1.InsertTextFormat.Snippet, insertText: 'union ${1:Name} {\n\t$0\n}' },
            { label: 'type', kind: node_1.CompletionItemKind.Keyword, detail: 'Alias de type', insertTextFormat: node_1.InsertTextFormat.Snippet, insertText: 'type ${1:Name} = ${2:Existing};' },
            { label: 'let', kind: node_1.CompletionItemKind.Keyword, detail: 'Binding (variable)', insertTextFormat: node_1.InsertTextFormat.Snippet, insertText: 'let ${1:name} = ${2:value};' },
            { label: 'const', kind: node_1.CompletionItemKind.Keyword, detail: 'Constante', insertTextFormat: node_1.InsertTextFormat.Snippet, insertText: 'const ${1:NAME} = ${2:value};' },
            { label: 'static', kind: node_1.CompletionItemKind.Keyword, detail: 'Statique', insertTextFormat: node_1.InsertTextFormat.Snippet, insertText: 'static ${1:NAME}: ${2:Type} = ${3:value};' },
            { label: 'return', kind: node_1.CompletionItemKind.Keyword, detail: 'Retour de fonction', insertTextFormat: node_1.InsertTextFormat.Snippet, insertText: 'return ${1:value};' },
            { label: 'match', kind: node_1.CompletionItemKind.Keyword, detail: 'Expression de correspondance', insertTextFormat: node_1.InsertTextFormat.Snippet, insertText: 'match ${1:expr} {\n\t${2:pattern} => ${3:result},\n}' },
            { label: 'if', kind: node_1.CompletionItemKind.Keyword, detail: 'Condition', insertTextFormat: node_1.InsertTextFormat.Snippet, insertText: 'if ${1:cond} {\n\t$0\n}' },
            { label: 'else', kind: node_1.CompletionItemKind.Keyword, detail: 'Alternative', insertTextFormat: node_1.InsertTextFormat.Snippet, insertText: 'else {\n\t$0\n}' },
            { label: 'while', kind: node_1.CompletionItemKind.Keyword, detail: 'Boucle while', insertTextFormat: node_1.InsertTextFormat.Snippet, insertText: 'while ${1:cond} {\n\t$0\n}' },
            { label: 'for', kind: node_1.CompletionItemKind.Keyword, detail: 'Boucle for', insertTextFormat: node_1.InsertTextFormat.Snippet, insertText: 'for ${1:item} in ${2:iter} {\n\t$0\n}' },
            { label: 'loop', kind: node_1.CompletionItemKind.Keyword, detail: 'Boucle infinie', insertTextFormat: node_1.InsertTextFormat.Snippet, insertText: 'loop {\n\t$0\n}' },
        ];
        return items;
    }
    /** Attach a bit more detail on resolve. */
    doResolve(item) {
        const label = String(item.label);
        const extra = this._docFor(label);
        item.detail ?? (item.detail = extra.title);
        const documentation = { kind: node_1.MarkupKind.Markdown, value: extra.markdown };
        item.documentation = documentation;
        return item;
    }
    _docFor(label) {
        switch (label) {
            case 'module': return { title: 'Déclaration de module', markdown: '**module** — Déclare le module courant.\n\n```vitte\nmodule my.module;\n```' };
            case 'import': return { title: 'Import', markdown: '**import** — Ajoute un symbole d’un autre module.\n\n```vitte\nimport std::core;\n```' };
            case 'fn': return { title: 'Déclaration de fonction', markdown: '**fn** — Déclare une fonction.\n\n```vitte\nfn name(args) {\n    // corps\n}\n```' };
            case 'struct': return { title: 'Déclaration de structure', markdown: '**struct** — Regroupe des champs.\n\n```vitte\nstruct Name {\n    field: Type\n}\n```' };
            case 'enum': return { title: 'Déclaration d’énumération', markdown: '**enum** — Variantes nommées.\n\n```vitte\nenum Name {\n    Variant\n}\n```' };
            case 'union': return { title: 'Déclaration d’union', markdown: '**union** — Représentation mémoire partagée.\n\n```vitte\nunion Name {\n    field: Type\n}\n```' };
            case 'type': return { title: 'Alias de type', markdown: '**type** — Alias vers un autre type.\n\n```vitte\ntype Alias = Existing;\n```' };
            case 'let': return { title: 'Binding', markdown: '**let** — Nouveau binding.\n\n```vitte\nlet x = 1;\n```' };
            case 'const': return { title: 'Constante', markdown: '**const** — Valeur immuable.\n\n```vitte\nconst NAME = 1;\n```' };
            case 'static': return { title: 'Statique', markdown: '**static** — Valeur globale statique.\n\n```vitte\nstatic NAME: Type = init();\n```' };
            case 'match': return { title: 'Expression de correspondance', markdown: '**match** — Branches par motif.\n\n```vitte\nmatch x {\n    0 => 1,\n}\n```' };
            default: return { title: String(label), markdown: `**${label}**` };
        }
    }
}
exports.VitteLanguageService = VitteLanguageService;
exports.default = VitteLanguageService;
//# sourceMappingURL=languageService.js.map