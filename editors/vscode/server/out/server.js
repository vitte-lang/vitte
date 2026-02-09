"use strict";
// server.ts — Vitte/Vit hardened LSP server
// Goals:
// - Full capabilities: completion, hover, symbols, def/refs, rename, formatting, diagnostics, semantic tokens
// - Dynamic config management + watchers + graceful shutdown
// - Lint with debounce, size guards, cancellation
// - Controlled logging, lightweight metrics, pervasive try/catch
Object.defineProperty(exports, "__esModule", { value: true });
const node_1 = require("vscode-languageserver/node");
const vscode_languageserver_textdocument_1 = require("vscode-languageserver-textdocument");
/* Modules internes */
const completion_js_1 = require("./completion.js");
const navigation_js_1 = require("./navigation.js");
const formatting_js_1 = require("./formatting.js");
const semantic_js_1 = require("./semantic.js");
const lint_js_1 = require("./lint.js");
const commands_js_1 = require("./commands.js");
const indexer_js_1 = require("./indexer.js");
const createSemanticTokens = semantic_js_1.buildSemanticTokens;
const metricStore = new Map();
/* --------------------------- Connection + documents ----------------------- */
const connection = (0, node_1.createConnection)(node_1.ProposedFeatures.all);
const documents = new node_1.TextDocuments(vscode_languageserver_textdocument_1.TextDocument);
function emptySemanticTokens() {
    return { data: [] };
}
const DEFAULT_SETTINGS = {
    trace: "off",
    lintDebounceMs: 200,
    enableFormatting: true,
    maxFileSizeKB: 1024, // 1 MB
};
let globalSettings = { ...DEFAULT_SETTINGS };
let hasConfigurationCapability = false;
let hasWorkspaceFoldersCapability = false;
connection.onInitialize((params) => {
    const capabilities = params.capabilities;
    hasConfigurationCapability = !!capabilities.workspace?.configuration;
    hasWorkspaceFoldersCapability = !!capabilities.workspace?.workspaceFolders;
    const legend = (0, semantic_js_1.getSemanticTokensLegend)();
    const result = {
        capabilities: {
            textDocumentSync: node_1.TextDocumentSyncKind.Incremental,
            // Enable resolve to enrich completion items lazily and use centralized trigger characters
            completionProvider: { resolveProvider: true, triggerCharacters: (0, completion_js_1.triggerCharacters)() },
            hoverProvider: true,
            documentSymbolProvider: true,
            documentFormattingProvider: true,
            definitionProvider: true,
            referencesProvider: true,
            renameProvider: { prepareProvider: true },
            workspaceSymbolProvider: true,
            semanticTokensProvider: { legend, full: true, range: false },
        },
    };
    if (hasWorkspaceFoldersCapability) {
        result.capabilities.workspace = { workspaceFolders: { supported: true } };
    }
    return result;
});
connection.onInitialized(() => {
    (0, commands_js_1.registerCommands)(connection);
    if (hasConfigurationCapability) {
        void connection.client.register(node_1.DidChangeConfigurationNotification.type, undefined);
    }
    // Watch classiques pour compat large versions du client
    connection.onDidChangeWatchedFiles((change) => {
        for (const ev of change.changes) {
            if (ev.type === node_1.FileChangeType.Deleted) {
                void connection.sendDiagnostics({ uri: ev.uri, diagnostics: [] });
            }
        }
    });
});
connection.onRequest("vitte/metrics", () => {
    const snapshot = Array.from(metricStore.entries()).map(([name, data]) => ({
        name,
        count: data.count,
        averageMs: data.count > 0 ? data.totalMs / data.count : 0,
        maxMs: data.maxMs,
        lastMs: data.lastMs,
        lastAt: data.lastAt,
        lastUri: data.lastUri,
        lastCount: data.lastCount ?? null,
    }));
    snapshot.sort((a, b) => b.averageMs - a.averageMs);
    return snapshot;
});
connection.onShutdown(() => {
    for (const t of lintTimers.values())
        clearTimeout(t);
    lintTimers.clear();
    (0, indexer_js_1.clearIndex)();
});
/* ------------------------------- Config updates --------------------------- */
async function applyConfiguration() {
    const workspace = Reflect.get(connection, "workspace");
    if (hasConfigurationCapability && workspace) {
        try {
            const cfg = await workspace.getConfiguration({ section: "vitte" });
            globalSettings = { ...DEFAULT_SETTINGS, ...(cfg ?? {}) };
        }
        catch {
            globalSettings = { ...DEFAULT_SETTINGS };
        }
    }
    else {
        globalSettings = { ...DEFAULT_SETTINGS };
    }
    for (const doc of documents.all())
        scheduleLint(doc);
}
connection.onDidChangeConfiguration(() => {
    void applyConfiguration();
});
/* --------------------------------- Guards -------------------------------- */
function tooLarge(doc) {
    const kb = Buffer.byteLength(doc.getText(), "utf8") / 1024;
    return kb > (globalSettings.maxFileSizeKB | 0);
}
function cancelled(token) { return !!token?.isCancellationRequested; }
/* --------------------------------- Handlers ------------------------------- */
connection.onCompletion((params, token) => {
    const start = now();
    const doc = documents.get(params.textDocument.uri);
    if (!doc || cancelled(token))
        return [];
    try {
        const result = (0, completion_js_1.provideCompletions)(doc, params.position);
        metric("completion", start, doc.uri, result.length);
        return result;
    }
    catch (e) {
        logErr("completion", e);
        return [];
    }
});
// Allow the client to resolve/enrich completion items on demand (details, docs)
connection.onCompletionResolve((item) => {
    const start = now();
    try {
        const resolved = (0, completion_js_1.resolveCompletion)(item);
        metric("completionResolve", start, "completionResolve");
        return resolved;
    }
    catch (e) {
        logErr("completionResolve", e);
        return item;
    }
});
connection.onHover((params, token) => {
    const start = now();
    const doc = documents.get(params.textDocument.uri);
    if (!doc || cancelled(token))
        return null;
    try {
        const hover = (0, semantic_js_1.provideHover)(doc, params.position);
        metric("hover", start, doc.uri);
        return hover;
    }
    catch (e) {
        logErr("hover", e);
        return null;
    }
});
connection.onDocumentSymbol((params, token) => {
    const start = now();
    const doc = documents.get(params.textDocument.uri);
    if (!doc || cancelled(token))
        return [];
    try {
        const symbols = (0, navigation_js_1.documentSymbols)(doc);
        metric("documentSymbols", start, doc.uri, symbols.length);
        return symbols;
    }
    catch (e) {
        logErr("documentSymbols", e);
        return [];
    }
});
connection.onDocumentFormatting((params, token) => {
    const start = now();
    const doc = documents.get(params.textDocument.uri);
    if (!doc || !globalSettings.enableFormatting || cancelled(token))
        return [];
    try {
        const edits = (0, formatting_js_1.provideFormattingEdits)(doc, params.options);
        metric("formatting", start, doc.uri, edits.length);
        return edits;
    }
    catch (e) {
        logErr("formatting", e);
        return [];
    }
});
connection.onDefinition((params, token) => {
    const start = now();
    const doc = documents.get(params.textDocument.uri);
    if (!doc || cancelled(token))
        return [];
    try {
        const defs = (0, navigation_js_1.definitionAtPosition)(doc, params.position, params.textDocument.uri);
        metric("definition", start, doc.uri, defs.length);
        return defs;
    }
    catch (e) {
        logErr("definition", e);
        return [];
    }
});
connection.onReferences((params, token) => {
    const start = now();
    const doc = documents.get(params.textDocument.uri);
    if (!doc || cancelled(token))
        return [];
    try {
        const refs = (0, navigation_js_1.referencesAtPosition)(doc, params.position, params.textDocument.uri);
        metric("references", start, doc.uri, refs.length);
        return refs;
    }
    catch (e) {
        logErr("references", e);
        return [];
    }
});
connection.onPrepareRename((params, token) => {
    const start = now();
    const doc = documents.get(params.textDocument.uri);
    if (!doc || cancelled(token))
        return null;
    try {
        const result = (0, navigation_js_1.prepareRename)(doc, params.position);
        if (result)
            metric("prepareRename", start, doc.uri);
        return result;
    }
    catch (e) {
        logErr("prepareRename", e);
        return null;
    }
});
// Correction de type: renvoie WorkspaceEdit
connection.onRenameRequest((params, token) => {
    const start = now();
    const doc = documents.get(params.textDocument.uri);
    if (!doc || cancelled(token))
        return null;
    try {
        const edits = (0, navigation_js_1.renameSymbol)(doc, params.position, params.newName).map(e => node_1.TextEdit.replace(e.range, e.newText));
        const we = { changes: { [doc.uri]: edits } };
        metric("rename", start, doc.uri, edits.length);
        return we;
    }
    catch (e) {
        logErr("rename", e);
        return null;
    }
});
connection.onWorkspaceSymbol((params, token) => {
    const start = now();
    if (cancelled(token))
        return [];
    try {
        const openDocs = documents.all().map(d => ({ uri: d.uri, doc: d }));
        const symbols = (0, navigation_js_1.workspaceSymbols)(params.query ?? "", openDocs, 200);
        metric("workspaceSymbols", start, params.query ?? "", symbols.length);
        return symbols;
    }
    catch (e) {
        logErr("workspaceSymbols", e);
        return [];
    }
});
connection.languages.semanticTokens.on((params, token) => {
    const start = now();
    const doc = documents.get(params.textDocument.uri);
    if (!doc || cancelled(token))
        return Promise.resolve(emptySemanticTokens());
    if (tooLarge(doc))
        return Promise.resolve(emptySemanticTokens());
    try {
        const tokens = createSemanticTokens(doc);
        metric("semanticTokens", start, doc.uri, tokens.data.length / 5);
        return Promise.resolve(tokens);
    }
    catch (e) {
        logErr("semanticTokens", e);
        return Promise.resolve(emptySemanticTokens());
    }
});
/* -------------------------------- Diagnostics ----------------------------- */
const lintTimers = new Map();
function runLint(doc) {
    try {
        if (tooLarge(doc)) {
            void connection.sendDiagnostics({ uri: doc.uri, diagnostics: [] });
            return;
        }
        const text = doc.getText();
        const uri = doc.uri;
        const t0 = now();
        const diags = (0, lint_js_1.lintToPublishable)(text, uri) ?? [];
        void connection.sendDiagnostics({ uri, diagnostics: diags });
        metric("lint", t0, uri, diags.length);
    }
    catch (e) {
        logErr("lint", e);
    }
}
function scheduleLint(doc) {
    const key = doc.uri;
    const delay = Math.max(0, globalSettings.lintDebounceMs | 0);
    const prev = lintTimers.get(key);
    if (prev)
        clearTimeout(prev);
    lintTimers.set(key, setTimeout(() => runLint(doc), delay));
}
/* --------------------------------- Events -------------------------------- */
documents.onDidOpen((e) => { (0, indexer_js_1.indexDocument)(e.document); scheduleLint(e.document); });
documents.onDidChangeContent((e) => { (0, indexer_js_1.indexDocument)(e.document); scheduleLint(e.document); });
documents.onDidClose((e) => {
    (0, indexer_js_1.removeDocument)(e.document.uri);
    void connection.sendDiagnostics({ uri: e.document.uri, diagnostics: [] });
    lintTimers.delete(e.document.uri);
});
/* --------------------------------- Launch -------------------------------- */
documents.listen(connection);
connection.listen();
/* --------------------------------- Utils --------------------------------- */
function now() {
    if (typeof process !== "undefined" && typeof process.hrtime === "function") {
        const [sec, nanosec] = process.hrtime();
        return sec * 1000 + nanosec / 1e6;
    }
    return Date.now();
}
function metric(what, startMs, uri, n) {
    const elapsed = now() - startMs;
    const entry = metricStore.get(what) ?? {
        count: 0,
        totalMs: 0,
        maxMs: 0,
        lastMs: 0,
        lastAt: 0,
        lastUri: "",
    };
    entry.count += 1;
    entry.totalMs += elapsed;
    entry.maxMs = Math.max(entry.maxMs, elapsed);
    entry.lastMs = elapsed;
    entry.lastAt = Date.now();
    entry.lastUri = uri;
    entry.lastCount = typeof n === "number" ? n : undefined;
    metricStore.set(what, entry);
    if (globalSettings.trace === "verbose") {
        connection.console.log(`[metric] ${what} ${elapsed.toFixed(1)}ms uri=${uri}${typeof n === "number" ? ` n=${n}` : ""}`);
    }
}
function logErr(ctx, err) {
    const msg = err instanceof Error ? `${err.name}: ${err.message}` : String(err);
    if (globalSettings.trace === "verbose" && err instanceof Error && err.stack) {
        connection.console.error(`[${ctx}] ${msg}\n${err.stack}`);
    }
    else {
        connection.console.error(`[${ctx}] ${msg}`);
    }
}
//# sourceMappingURL=server.js.map