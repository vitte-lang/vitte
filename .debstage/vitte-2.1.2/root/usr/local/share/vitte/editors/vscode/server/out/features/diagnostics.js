"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.registerDiagnostics = registerDiagnostics;
exports.updateConfig = updateConfig;
const node_1 = require("vscode-languageserver/node");
const DEFAULT_CONFIG = {
    maxProblems: 200,
    longLine: 160,
    allowTabs: false,
    todoSeverity: node_1.DiagnosticSeverity.Hint,
};
// Internal state
let currentConfig = { ...DEFAULT_CONFIG };
const debounceMs = 200; // light debounce between rapid edits
const timers = new Map();
/** Register diagnostics on connection & documents */
function registerDiagnostics(connection, documents) {
    documents.onDidChangeContent((change) => scheduleValidate(connection, change.document));
    documents.onDidOpen((open) => scheduleValidate(connection, open.document));
    documents.onDidClose((close) => clearDiagnostics(connection, close.document));
    // Optional: react to config changes from client (if supported)
    connection.onDidChangeConfiguration((params) => {
        try {
            const cfg = params.settings?.vitte?.diagnostics;
            if (cfg && typeof cfg === 'object') {
                updateConfig(cfg);
                // revalidate all open docs after config change
                for (const doc of documents.all())
                    scheduleValidate(connection, doc);
            }
        }
        catch {
            // ignore invalid settings payloads
        }
    });
}
function updateConfig(partial) {
    const next = { ...currentConfig };
    if (typeof partial.maxProblems === 'number' && Number.isFinite(partial.maxProblems) && partial.maxProblems >= 0) {
        next.maxProblems = partial.maxProblems;
    }
    if (typeof partial.longLine === 'number' && Number.isFinite(partial.longLine) && partial.longLine >= 0) {
        next.longLine = partial.longLine;
    }
    if (typeof partial.allowTabs === 'boolean') {
        next.allowTabs = partial.allowTabs;
    }
    if (typeof partial.todoSeverity === 'number') {
        next.todoSeverity = partial.todoSeverity;
    }
    currentConfig = next;
}
function scheduleValidate(connection, doc) {
    const uri = doc.uri;
    const existing = timers.get(uri);
    if (existing)
        clearTimeout(existing);
    const t = setTimeout(() => {
        timers.delete(uri);
        void validateAndPublish(connection, doc);
    }, debounceMs);
    timers.set(uri, t);
}
function clearDiagnostics(connection, doc) {
    // Publish an empty list to clear diagnostics on close
    connection.sendDiagnostics({ uri: doc.uri, diagnostics: [] });
    const timer = timers.get(doc.uri);
    if (timer) {
        clearTimeout(timer);
        timers.delete(doc.uri);
    }
}
async function validateAndPublish(connection, doc) {
    const diagnostics = computeDiagnostics(doc, currentConfig);
    // Respect cap
    const limited = diagnostics.length > currentConfig.maxProblems
        ? diagnostics.slice(0, currentConfig.maxProblems)
        : diagnostics;
    // Publish
    // In LSP, diagnostics are pushed by server to client
    try {
        connection.sendDiagnostics({ uri: doc.uri, diagnostics: limited });
    }
    catch { /* ignore */ }
}
/** Compute diagnostics with lightweight, syntax-agnostic heuristics. */
function computeDiagnostics(doc, cfg) {
    const text = doc.getText();
    const lines = splitLines(text);
    const out = [];
    // Simple brace balance check (pairs of {} [] ()) — heuristic
    const braceStack = [];
    const openers = new Set(['{', '(', '[']);
    const closers = { '}': '{', ')': '(', ']': '[' };
    for (let i = 0; i < lines.length; i++) {
        const line = lines[i];
        // 1) Trailing whitespace
        const tw = /\s+$/.exec(line);
        if (tw) {
            out.push(diagnostic(range(i, Math.max(0, line.length - (tw[0]?.length ?? 0)), i, line.length), 'Espace(s) de fin de ligne inutile(s)', node_1.DiagnosticSeverity.Hint));
        }
        // 2) Tabs when not allowed
        if (!cfg.allowTabs && line.includes('\t')) {
            const col = line.indexOf('\t');
            out.push(diagnostic(range(i, col, i, col + 1), 'Tabulation trouvée (préférer des espaces)', node_1.DiagnosticSeverity.Information));
        }
        // 3) Long lines
        if (cfg.longLine > 0 && line.length > cfg.longLine) {
            out.push(diagnostic(range(i, cfg.longLine, i, line.length), `Ligne trop longue (${line.length} > ${cfg.longLine})`, node_1.DiagnosticSeverity.Hint));
        }
        // 4) TODO markers
        const todoIdx = line.indexOf('TODO');
        if (todoIdx >= 0) {
            out.push(diagnostic(range(i, todoIdx, i, Math.min(line.length, todoIdx + 4)), 'TODO trouvé', cfg.todoSeverity));
        }
        // 5) Brace tracking
        for (let j = 0; j < line.length; j++) {
            const ch = line[j];
            if (openers.has(ch)) {
                braceStack.push({ ch, line: i, col: j });
            }
            else if (ch in closers) {
                const need = closers[ch];
                const top = braceStack.pop();
                if (!top || top.ch !== need) {
                    out.push(diagnostic(range(i, j, i, j + 1), `Fermeture inattendue « ${ch} »`, node_1.DiagnosticSeverity.Warning));
                }
            }
        }
    }
    // Unclosed openers
    for (const b of braceStack) {
        out.push(diagnostic(range(b.line, b.col, b.line, Math.max(b.col + 1, 0)), `Délimiteur « ${b.ch} » non refermé`, node_1.DiagnosticSeverity.Warning));
    }
    return out;
}
// ----------------- helpers -----------------
function diagnostic(r, message, severity) {
    return { range: r, message, severity, source: 'vitte' };
}
function range(sl, sc, el, ec) {
    return node_1.Range.create(node_1.Position.create(sl, sc), node_1.Position.create(el, ec));
}
function splitLines(text) {
    // Keep it simple and fast; LSP Document provides all text
    // Normalize to \n and split, preserving last empty line if any
    return text.replace(/\r\n?/g, '\n').split('\n');
}
//# sourceMappingURL=diagnostics.js.map