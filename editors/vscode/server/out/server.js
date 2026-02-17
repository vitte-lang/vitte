"use strict";
// server.ts — Vitte/Vit hardened LSP server
// Goals:
// - Full capabilities: completion, hover, symbols, def/refs, rename, formatting, diagnostics, semantic tokens
// - Dynamic config management + watchers + graceful shutdown
// - Lint with debounce, size guards, cancellation
// - Controlled logging, lightweight metrics, pervasive try/catch
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
const fs = __importStar(require("node:fs/promises"));
const path = __importStar(require("node:path"));
const node_url_1 = require("node:url");
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
    traceSampleRate: 0.01,
    lintDebounceMs: 200,
    enableFormatting: true,
    maxFileSizeKB: 1024, // 1 MB
    requestTimeoutMs: 800,
    requestTimeouts: {},
    requestMaxConcurrent: 4,
    indexerMaxRssMB: 1024,
    indexerCacheEnabled: true,
    lint: {},
    features: {
        completion: true,
        hover: true,
        definition: true,
        references: true,
        rename: true,
        documentSymbols: true,
        workspaceSymbols: true,
        semanticTokens: true,
        formatting: true,
        lint: true,
    },
};
let globalSettings = { ...DEFAULT_SETTINGS };
let hasConfigurationCapability = false;
let hasWorkspaceFoldersCapability = false;
let workspaceRoot;
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
    void loadIndexCache();
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
        p99Ms: percentile(data.samples, 99),
        errorCount: data.errorCount,
        lastError: data.lastError ?? null,
    }));
    snapshot.sort((a, b) => b.averageMs - a.averageMs);
    return snapshot;
});
connection.onRequest("vitte/metrics.reset", () => {
    metricStore.clear();
    return { ok: true };
});
connection.onRequest("vitte/ping", () => {
    return { ok: true, ts: Date.now() };
});
connection.onShutdown(() => {
    for (const t of lintTimers.values())
        clearTimeout(t);
    lintTimers.clear();
    void saveIndexCache();
    (0, indexer_js_1.clearIndex)();
});
/* ------------------------------- Config updates --------------------------- */
async function applyConfiguration() {
    const workspace = Reflect.get(connection, "workspace");
    if (hasConfigurationCapability && workspace) {
        try {
            const cfg = await workspace.getConfiguration({ section: "vitte" });
            const lintCfg = await workspace.getConfiguration({ section: "vitte.lint" });
            const merged = { ...DEFAULT_SETTINGS, ...(cfg ?? {}) };
            merged.lint = { ...DEFAULT_SETTINGS.lint, ...(lintCfg ?? {}) };
            merged.features = { ...DEFAULT_SETTINGS.features, ...(cfg?.features ?? {}) };
            merged.lintDebounceMs = clampNumber(merged.lintDebounceMs, 0, 2000, DEFAULT_SETTINGS.lintDebounceMs);
            merged.maxFileSizeKB = clampNumber(merged.maxFileSizeKB, 64, 10000, DEFAULT_SETTINGS.maxFileSizeKB);
            merged.requestTimeoutMs = clampNumber(merged.requestTimeoutMs, 100, 5000, DEFAULT_SETTINGS.requestTimeoutMs);
            merged.traceSampleRate = clampNumber(merged.traceSampleRate, 0, 1, DEFAULT_SETTINGS.traceSampleRate);
            merged.requestMaxConcurrent = clampNumber(merged.requestMaxConcurrent, 1, 32, DEFAULT_SETTINGS.requestMaxConcurrent);
            merged.indexerMaxRssMB = clampNumber(merged.indexerMaxRssMB, 256, 8192, DEFAULT_SETTINGS.indexerMaxRssMB);
            merged.indexerCacheEnabled = cfg?.indexerCacheEnabled ?? DEFAULT_SETTINGS.indexerCacheEnabled;
            merged.requestTimeouts = { ...DEFAULT_SETTINGS.requestTimeouts, ...(cfg?.requestTimeouts ?? {}) };
            globalSettings = merged;
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
async function resolveWorkspaceRoot() {
    if (workspaceRoot)
        return workspaceRoot;
    try {
        const folders = await connection.workspace.getWorkspaceFolders();
        if (!folders || folders.length === 0)
            return undefined;
        const first = folders[0].uri;
        if (first.startsWith("file://")) {
            workspaceRoot = (0, node_url_1.fileURLToPath)(first);
            return workspaceRoot;
        }
    }
    catch {
        return undefined;
    }
    return undefined;
}
async function getIndexCachePath() {
    const root = await resolveWorkspaceRoot();
    if (!root)
        return undefined;
    return path.join(root, ".vitte", "index-cache.json");
}
function isIndexSnapshot(value) {
    if (!value || typeof value !== "object")
        return false;
    const v = value;
    return v.version === 1 && Array.isArray(v.entries);
}
async function loadIndexCache() {
    if (!globalSettings.indexerCacheEnabled)
        return;
    const cachePath = await getIndexCachePath();
    if (!cachePath)
        return;
    try {
        const raw = await fs.readFile(cachePath, "utf8");
        const parsed = JSON.parse(raw);
        const snapshot = isIndexSnapshot(parsed) ? parsed : null;
        const count = (0, indexer_js_1.loadIndexSnapshot)(snapshot);
        connection.console.log(`[index] loaded cache (${count} files)`);
    }
    catch {
        // ignore cache errors
    }
}
async function saveIndexCache() {
    if (!globalSettings.indexerCacheEnabled)
        return;
    const cachePath = await getIndexCachePath();
    if (!cachePath)
        return;
    try {
        await fs.mkdir(path.dirname(cachePath), { recursive: true });
        const snapshot = (0, indexer_js_1.exportIndexSnapshot)();
        await fs.writeFile(cachePath, JSON.stringify(snapshot), "utf8");
    }
    catch {
        // ignore cache errors
    }
}
/* --------------------------------- Handlers ------------------------------- */
const breaker = { consecutiveErrors: 0, until: 0 };
const BREAKER_THRESHOLD = 5;
const BREAKER_COOLDOWN_MS = 30000;
const requestQueue = [];
let activeRequests = 0;
function isBreakerActive() {
    return Date.now() < breaker.until;
}
function recordSuccess() {
    breaker.consecutiveErrors = 0;
}
function recordFailure() {
    breaker.consecutiveErrors += 1;
    if (breaker.consecutiveErrors >= BREAKER_THRESHOLD) {
        breaker.until = Date.now() + BREAKER_COOLDOWN_MS;
    }
}
function featureEnabled(name) {
    return Boolean(globalSettings.features?.[name]);
}
async function withTimeout(task, ms) {
    if (ms <= 0)
        return task;
    let tid;
    const timeout = new Promise((_, reject) => {
        tid = setTimeout(() => reject(new Error("Request timed out")), ms);
    });
    try {
        return await Promise.race([task, timeout]);
    }
    finally {
        if (tid)
            clearTimeout(tid);
    }
}
function timeoutFor(feature) {
    const per = globalSettings.requestTimeouts?.[feature];
    if (typeof per === "number" && Number.isFinite(per) && per > 0)
        return per;
    return globalSettings.requestTimeoutMs;
}
async function withBackpressure(fn) {
    const max = Math.max(1, globalSettings.requestMaxConcurrent | 0);
    if (activeRequests >= max) {
        return new Promise((resolve, reject) => {
            requestQueue.push(() => {
                void withBackpressure(fn).then(resolve, reject);
            });
        });
    }
    activeRequests += 1;
    try {
        return await fn();
    }
    finally {
        activeRequests -= 1;
        if (requestQueue.length && activeRequests < max) {
            const next = requestQueue.shift();
            if (next)
                next();
        }
    }
}
function failFast(feature) {
    if (featureEnabled(feature))
        return false;
    connection.console.warn(`[disabled] ${feature} is disabled by configuration`);
    return true;
}
function memoryExceeded() {
    const rss = process.memoryUsage().rss / (1024 * 1024);
    return rss > globalSettings.indexerMaxRssMB;
}
connection.onCompletion(async (params, token) => {
    if (failFast("completion"))
        return [];
    if (isBreakerActive())
        return [];
    return withBackpressure(async () => {
        const start = now();
        const doc = documents.get(params.textDocument.uri);
        if (!doc || cancelled(token))
            return [];
        try {
            const result = await withTimeout(Promise.resolve((0, completion_js_1.provideCompletions)(doc, params.position)), timeoutFor("completion"));
            metric("completion", start, doc.uri, result.length);
            recordSuccess();
            return result;
        }
        catch (e) {
            metric("completion", start, doc?.uri ?? "unknown", undefined, e);
            recordFailure();
            logErr("completion", e);
            return [];
        }
    });
});
// Allow the client to resolve/enrich completion items on demand (details, docs)
connection.onCompletionResolve(async (item) => {
    if (failFast("completion"))
        return item;
    if (isBreakerActive())
        return item;
    return withBackpressure(async () => {
        const start = now();
        try {
            const resolved = await withTimeout(Promise.resolve((0, completion_js_1.resolveCompletion)(item)), timeoutFor("completion"));
            metric("completionResolve", start, "completionResolve");
            recordSuccess();
            return resolved;
        }
        catch (e) {
            metric("completionResolve", start, "completionResolve", undefined, e);
            recordFailure();
            logErr("completionResolve", e);
            return item;
        }
    });
});
connection.onHover(async (params, token) => {
    if (failFast("hover"))
        return null;
    if (isBreakerActive())
        return null;
    return withBackpressure(async () => {
        const start = now();
        const doc = documents.get(params.textDocument.uri);
        if (!doc || cancelled(token))
            return null;
        try {
            const hover = await withTimeout(Promise.resolve((0, semantic_js_1.provideHover)(doc, params.position)), timeoutFor("hover"));
            metric("hover", start, doc.uri);
            recordSuccess();
            return hover;
        }
        catch (e) {
            metric("hover", start, doc?.uri ?? "unknown", undefined, e);
            recordFailure();
            logErr("hover", e);
            return null;
        }
    });
});
connection.onDocumentSymbol(async (params, token) => {
    if (failFast("documentSymbols"))
        return [];
    if (isBreakerActive())
        return [];
    return withBackpressure(async () => {
        const start = now();
        const doc = documents.get(params.textDocument.uri);
        if (!doc || cancelled(token))
            return [];
        try {
            const symbols = await withTimeout(Promise.resolve((0, navigation_js_1.documentSymbols)(doc)), timeoutFor("documentSymbols"));
            metric("documentSymbols", start, doc.uri, symbols.length);
            recordSuccess();
            return symbols;
        }
        catch (e) {
            metric("documentSymbols", start, doc?.uri ?? "unknown", undefined, e);
            recordFailure();
            logErr("documentSymbols", e);
            return [];
        }
    });
});
connection.onDocumentFormatting(async (params, token) => {
    if (!globalSettings.enableFormatting)
        return [];
    if (failFast("formatting"))
        return [];
    if (isBreakerActive())
        return [];
    return withBackpressure(async () => {
        const start = now();
        const doc = documents.get(params.textDocument.uri);
        if (!doc || cancelled(token))
            return [];
        try {
            const edits = await withTimeout(Promise.resolve((0, formatting_js_1.provideFormattingEdits)(doc, params.options)), timeoutFor("formatting"));
            metric("formatting", start, doc.uri, edits.length);
            recordSuccess();
            return edits;
        }
        catch (e) {
            metric("formatting", start, doc?.uri ?? "unknown", undefined, e);
            recordFailure();
            logErr("formatting", e);
            return [];
        }
    });
});
connection.onDefinition(async (params, token) => {
    if (failFast("definition"))
        return [];
    if (isBreakerActive())
        return [];
    return withBackpressure(async () => {
        const start = now();
        const doc = documents.get(params.textDocument.uri);
        if (!doc || cancelled(token))
            return [];
        try {
            const defs = await withTimeout(Promise.resolve((0, navigation_js_1.definitionAtPosition)(doc, params.position, params.textDocument.uri)), timeoutFor("definition"));
            metric("definition", start, doc.uri, defs.length);
            recordSuccess();
            return defs;
        }
        catch (e) {
            metric("definition", start, doc?.uri ?? "unknown", undefined, e);
            recordFailure();
            logErr("definition", e);
            return [];
        }
    });
});
connection.onReferences(async (params, token) => {
    if (failFast("references"))
        return [];
    if (isBreakerActive())
        return [];
    return withBackpressure(async () => {
        const start = now();
        const doc = documents.get(params.textDocument.uri);
        if (!doc || cancelled(token))
            return [];
        try {
            const refs = await withTimeout(Promise.resolve((0, navigation_js_1.referencesAtPosition)(doc, params.position, params.textDocument.uri)), timeoutFor("references"));
            metric("references", start, doc.uri, refs.length);
            recordSuccess();
            return refs;
        }
        catch (e) {
            metric("references", start, doc?.uri ?? "unknown", undefined, e);
            recordFailure();
            logErr("references", e);
            return [];
        }
    });
});
connection.onPrepareRename(async (params, token) => {
    if (failFast("rename"))
        return null;
    if (isBreakerActive())
        return null;
    return withBackpressure(async () => {
        const start = now();
        const doc = documents.get(params.textDocument.uri);
        if (!doc || cancelled(token))
            return null;
        try {
            const result = await withTimeout(Promise.resolve((0, navigation_js_1.prepareRename)(doc, params.position)), timeoutFor("rename"));
            if (result)
                metric("prepareRename", start, doc.uri);
            recordSuccess();
            return result;
        }
        catch (e) {
            metric("prepareRename", start, doc?.uri ?? "unknown", undefined, e);
            recordFailure();
            logErr("prepareRename", e);
            return null;
        }
    });
});
// Correction de type: renvoie WorkspaceEdit
connection.onRenameRequest(async (params, token) => {
    if (failFast("rename"))
        return null;
    if (isBreakerActive())
        return null;
    return withBackpressure(async () => {
        const start = now();
        const doc = documents.get(params.textDocument.uri);
        if (!doc || cancelled(token))
            return null;
        try {
            const edits = await withTimeout(Promise.resolve((0, navigation_js_1.renameSymbol)(doc, params.position, params.newName)), timeoutFor("rename"));
            const we = { changes: { [doc.uri]: edits.map(e => node_1.TextEdit.replace(e.range, e.newText)) } };
            metric("rename", start, doc.uri, edits.length);
            recordSuccess();
            return we;
        }
        catch (e) {
            metric("rename", start, doc?.uri ?? "unknown", undefined, e);
            recordFailure();
            logErr("rename", e);
            return null;
        }
    });
});
connection.onWorkspaceSymbol(async (params, token) => {
    if (failFast("workspaceSymbols"))
        return [];
    if (isBreakerActive())
        return [];
    return withBackpressure(async () => {
        const start = now();
        if (cancelled(token))
            return [];
        try {
            const openDocs = documents.all().map(d => ({ uri: d.uri, doc: d }));
            const symbols = await withTimeout(Promise.resolve((0, navigation_js_1.workspaceSymbols)(params.query ?? "", openDocs, 200)), timeoutFor("workspaceSymbols"));
            metric("workspaceSymbols", start, params.query ?? "", symbols.length);
            recordSuccess();
            return symbols;
        }
        catch (e) {
            metric("workspaceSymbols", start, params.query ?? "workspaceSymbols", undefined, e);
            recordFailure();
            logErr("workspaceSymbols", e);
            return [];
        }
    });
});
connection.languages.semanticTokens.on(async (params, token) => {
    if (failFast("semanticTokens"))
        return emptySemanticTokens();
    if (isBreakerActive())
        return emptySemanticTokens();
    return withBackpressure(async () => {
        const start = now();
        const doc = documents.get(params.textDocument.uri);
        if (!doc || cancelled(token))
            return emptySemanticTokens();
        if (tooLarge(doc))
            return emptySemanticTokens();
        try {
            const tokens = await withTimeout(Promise.resolve(createSemanticTokens(doc)), timeoutFor("semanticTokens"));
            metric("semanticTokens", start, doc.uri, tokens.data.length / 5);
            recordSuccess();
            return tokens;
        }
        catch (e) {
            metric("semanticTokens", start, doc?.uri ?? "unknown", undefined, e);
            recordFailure();
            logErr("semanticTokens", e);
            return emptySemanticTokens();
        }
    });
});
/* -------------------------------- Diagnostics ----------------------------- */
const lintTimers = new Map();
const lintQueue = [];
const lintQueued = new Set();
let lintWorkerRunning = false;
function runLint(doc) {
    const t0 = now();
    try {
        if (!featureEnabled("lint"))
            return;
        if (isBreakerActive())
            return;
        if (tooLarge(doc)) {
            void connection.sendDiagnostics({ uri: doc.uri, diagnostics: [] });
            return;
        }
        const text = doc.getText();
        const uri = doc.uri;
        const diags = (0, lint_js_1.lintToPublishable)(text, uri, globalSettings.lint) ?? [];
        void connection.sendDiagnostics({ uri, diagnostics: diags });
        metric("lint", t0, uri, diags.length);
    }
    catch (e) {
        metric("lint", t0, doc?.uri ?? "unknown", undefined, e);
        logErr("lint", e);
    }
}
function scheduleLint(doc) {
    if (!featureEnabled("lint"))
        return;
    const key = doc.uri;
    const delay = Math.max(0, globalSettings.lintDebounceMs | 0);
    const prev = lintTimers.get(key);
    if (prev)
        clearTimeout(prev);
    lintTimers.set(key, setTimeout(() => enqueueLint(key), delay));
}
function safeIndexDocument(doc) {
    if (memoryExceeded()) {
        connection.console.warn("[index] skipped due to memory quota");
        return;
    }
    (0, indexer_js_1.indexDocument)(doc);
}
function enqueueLint(uri) {
    if (lintQueued.has(uri))
        return;
    lintQueued.add(uri);
    lintQueue.push(uri);
    void processLintQueue();
}
function processLintQueue() {
    if (lintWorkerRunning)
        return;
    lintWorkerRunning = true;
    try {
        while (lintQueue.length > 0) {
            const uri = lintQueue.shift();
            lintQueued.delete(uri);
            const doc = documents.get(uri);
            if (!doc)
                continue;
            runLint(doc);
        }
    }
    finally {
        lintWorkerRunning = false;
    }
}
/* --------------------------------- Events -------------------------------- */
documents.onDidOpen((e) => { safeIndexDocument(e.document); scheduleLint(e.document); });
documents.onDidChangeContent((e) => { safeIndexDocument(e.document); scheduleLint(e.document); });
documents.onDidClose((e) => {
    (0, indexer_js_1.removeDocument)(e.document.uri);
    void connection.sendDiagnostics({ uri: e.document.uri, diagnostics: [] });
    const timer = lintTimers.get(e.document.uri);
    if (timer)
        clearTimeout(timer);
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
function metric(what, startMs, uri, n, err) {
    const elapsed = now() - startMs;
    recordMetric(what, elapsed, uri, n, err);
}
function recordMetric(what, elapsed, uri, n, err) {
    const entry = metricStore.get(what) ?? {
        count: 0,
        totalMs: 0,
        maxMs: 0,
        lastMs: 0,
        lastAt: 0,
        lastUri: "",
        errorCount: 0,
        samples: [],
    };
    entry.count += 1;
    entry.totalMs += elapsed;
    entry.maxMs = Math.max(entry.maxMs, elapsed);
    entry.lastMs = elapsed;
    entry.lastAt = Date.now();
    entry.lastUri = uri;
    entry.lastCount = typeof n === "number" ? n : undefined;
    if (err) {
        entry.errorCount += 1;
        entry.lastError = formatError(err);
    }
    entry.samples.push(elapsed);
    if (entry.samples.length > 1000)
        entry.samples.shift();
    metricStore.set(what, entry);
    if (globalSettings.trace === "verbose") {
        if (Math.random() < (globalSettings.traceSampleRate || 0)) {
            connection.console.log(`[metric] ${what} ${elapsed.toFixed(1)}ms uri=${uri}${typeof n === "number" ? ` n=${n}` : ""}`);
        }
    }
}
const errorRateLimit = new Map();
const ERROR_LOG_INTERVAL_MS = 2000;
function formatError(err) {
    if (err instanceof Error)
        return `${err.name}: ${err.message}`;
    if (typeof err === "string")
        return err;
    try {
        return JSON.stringify(err);
    }
    catch {
        return "[unknown error]";
    }
}
function logErr(ctx, err) {
    const msg = formatError(err);
    const nowTs = Date.now();
    const last = errorRateLimit.get(ctx) ?? 0;
    if (nowTs - last < ERROR_LOG_INTERVAL_MS)
        return;
    errorRateLimit.set(ctx, nowTs);
    if (globalSettings.trace === "verbose" && err instanceof Error && err.stack) {
        connection.console.error(`[${ctx}] ${msg}\n${err.stack}`);
    }
    else {
        connection.console.error(`[${ctx}] ${msg}`);
    }
}
function clampNumber(value, min, max, fallback) {
    if (!Number.isFinite(value))
        return fallback;
    return Math.min(max, Math.max(min, value));
}
function percentile(samples, p) {
    if (!samples.length)
        return 0;
    const sorted = [...samples].sort((a, b) => a - b);
    const idx = Math.min(sorted.length - 1, Math.max(0, Math.ceil((p / 100) * sorted.length) - 1));
    return sorted[idx];
}
//# sourceMappingURL=server.js.map