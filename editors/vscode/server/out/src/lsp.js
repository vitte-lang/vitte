"use strict";
/**
 * Vitte LSP — Main Server (synchronized)
 * --------------------------------------
 * Unified LSP server aligned with the updated config/logger/utils/languageService modules.
 */
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
var __importDefault = (this && this.__importDefault) || function (mod) {
    return (mod && mod.__esModule) ? mod : { "default": mod };
};
Object.defineProperty(exports, "__esModule", { value: true });
exports.connection = void 0;
const node_1 = require("vscode-languageserver/node");
const vscode_languageserver_textdocument_1 = require("vscode-languageserver-textdocument");
const config_1 = __importStar(require("./config"));
const logger_1 = require("./logger");
const completion_1 = require("./completion");
const semantic_1 = require("./semantic");
const lint_1 = require("./lint");
const navigation_1 = require("./navigation");
const indexer_1 = require("./indexer");
const docstrings_1 = require("./docstrings");
const formatting_1 = require("./formatting");
const promises_1 = __importDefault(require("node:fs/promises"));
const node_path_1 = __importDefault(require("node:path"));
const node_url_1 = require("node:url");
const fileCache = new Map();
const text_1 = require("./utils/text");
// ---------------------------------------------------------------------------
// Connection & documents
// ---------------------------------------------------------------------------
exports.connection = (0, node_1.createConnection)(node_1.ProposedFeatures.all);
(0, logger_1.attachConnection)(exports.connection);
logger_1.logLsp.info('Vitte LSP: connection created');
const documents = new node_1.TextDocuments(vscode_languageserver_textdocument_1.TextDocument);
let hasConfigurationCapability = false;
let hasWorkspaceFolderCapability = false;
const semanticLegend = (0, semantic_1.getSemanticTokensLegend)();
function wordAt(doc, pos) {
    const text = doc.getText();
    const off = doc.offsetAt(pos);
    let s = off;
    let e = off;
    while (s > 0 && /[A-Za-z0-9_]/.test(text.charAt(s - 1)))
        s--;
    while (e < text.length && /[A-Za-z0-9_]/.test(text.charAt(e)))
        e++;
    return e > s ? text.slice(s, e) : null;
}
function toExtraFormatting(opts) {
    const normalizeEOL = opts.eol === 'auto' ? undefined : opts.eol;
    return {
        tabSize: opts.tabSize,
        insertSpaces: opts.insertSpaces,
        trimTrailingWhitespace: opts.trimTrailingWhitespace,
        insertFinalNewline: opts.insertFinalNewline,
        normalizeEOL,
    };
}
function isIdentChar(ch) {
    return /[A-Za-z0-9_]/.test(ch);
}
function buildCodeMask(text) {
    const n = text.length;
    const mask = new Uint8Array(n);
    let i = 0;
    const mark = (from, to) => { for (let k = from; k < to; k++)
        mask[k] = 1; };
    while (i < n) {
        const c = text.charCodeAt(i);
        const c2 = i + 1 < n ? text.charCodeAt(i + 1) : 0;
        if (c === 0x23 /* # */ && c2 !== 0x5b /* [ */) {
            i += 1;
            while (i < n && text.charCodeAt(i) !== 0x0a)
                i++;
            continue;
        }
        if (c === 0x2f && c2 === 0x2f) {
            i += 2;
            while (i < n && text.charCodeAt(i) !== 0x0a)
                i++;
            continue;
        }
        if (c === 0x2f && c2 === 0x2a) {
            i += 2;
            while (i < n) {
                if (text.charCodeAt(i) === 0x2a && i + 1 < n && text.charCodeAt(i + 1) === 0x2f) {
                    i += 2;
                    break;
                }
                i++;
            }
            continue;
        }
        if (c === 0x72 /* r */) {
            let j = i + 1;
            let hashes = 0;
            while (j < n && text.charCodeAt(j) === 0x23) {
                hashes++;
                j++;
            }
            if (j < n && text.charCodeAt(j) === 0x22) {
                j++;
                for (; j < n; j++) {
                    if (text.charCodeAt(j) === 0x22) {
                        let k = j + 1;
                        let ok = true;
                        for (let h = 0; h < hashes; h++) {
                            if (k >= n || text.charCodeAt(k) !== 0x23) {
                                ok = false;
                                break;
                            }
                            k++;
                        }
                        if (ok) {
                            j = k;
                            break;
                        }
                    }
                }
                i = j;
                continue;
            }
        }
        if (c === 0x22 || c === 0x27) {
            const quote = c;
            i++;
            while (i < n) {
                if (text.charCodeAt(i) === 0x5c) {
                    i += 2;
                    continue;
                }
                if (text.charCodeAt(i) === quote) {
                    i++;
                    break;
                }
                i++;
            }
            continue;
        }
        const start = i;
        while (i < n) {
            const a = text.charCodeAt(i);
            const b = i + 1 < n ? text.charCodeAt(i + 1) : 0;
            if ((a === 0x2f && (b === 0x2f || b === 0x2a)) ||
                a === 0x22 ||
                a === 0x27 ||
                a === 0x72 ||
                (a === 0x23 && b !== 0x5b))
                break;
            i++;
        }
        mark(start, i);
    }
    return mask;
}
function buildLineIndex(text) {
    const idx = [0];
    for (let i = 0; i < text.length; i++)
        if (text.charCodeAt(i) === 10)
            idx.push(i + 1);
    return idx;
}
function offsetToPos(nlIdx, off) {
    let lo = 0;
    let hi = nlIdx.length - 1;
    while (lo <= hi) {
        const mid = (lo + hi) >> 1;
        const v = nlIdx[mid];
        if (v === off)
            return { line: mid, character: 0 };
        if (v < off)
            lo = mid + 1;
        else
            hi = mid - 1;
    }
    const line = Math.max(0, lo - 1);
    return { line, character: off - nlIdx[line] };
}
function findWordLocationsInText(text, uri, word) {
    if (!word)
        return [];
    const mask = buildCodeMask(text);
    const nlIdx = buildLineIndex(text);
    const out = [];
    let idx = text.indexOf(word);
    while (idx !== -1) {
        const before = idx === 0 ? "" : text.charAt(idx - 1);
        const after = idx + word.length >= text.length ? "" : text.charAt(idx + word.length);
        if (!isIdentChar(before) && !isIdentChar(after) && mask[idx]) {
            const start = offsetToPos(nlIdx, idx);
            const end = offsetToPos(nlIdx, idx + word.length);
            out.push(node_1.Location.create(uri, { start, end }));
        }
        idx = text.indexOf(word, idx + word.length);
    }
    return out;
}
async function readFileCached(filePath) {
    try {
        const stat = await promises_1.default.stat(filePath);
        const cached = fileCache.get(filePath);
        if (cached && cached.mtimeMs === stat.mtimeMs && cached.size === stat.size) {
            return cached.text;
        }
        const text = await promises_1.default.readFile(filePath, "utf8");
        fileCache.set(filePath, { mtimeMs: stat.mtimeMs, size: stat.size, text });
        return text;
    }
    catch {
        return null;
    }
}
const defaultIndexerSettings = {
    exclude: ["**/node_modules/**", "**/.git/**", "**/target/**", "**/build/**", "**/dist/**", "**/out/**", "**/.vscode/**"],
    maxFiles: 3000,
    maxFileSizeKB: 1000,
};
function compileExcludeMatchers(patterns) {
    return (patterns ?? []).map((p) => {
        const esc = p.replace(/[.+^${}()|[\]\\]/g, "\\$&");
        const rx = new RegExp("^" + esc.replace(/\*/g, ".*").replace(/\?/g, ".") + "$");
        return (pathStr) => rx.test(pathStr);
    });
}
function isExcluded(pathStr, matchers, roots) {
    const norm = pathStr.replace(/\\/g, "/");
    for (const root of roots) {
        const rel = norm.startsWith(root) ? norm.slice(root.length) : norm;
        const relNorm = rel.startsWith("/") ? rel.slice(1) : rel;
        for (const m of matchers) {
            if (m(relNorm) || m(norm))
                return true;
        }
    }
    return false;
}
async function getIndexerSettings() {
    if (!hasConfigurationCapability)
        return { ...defaultIndexerSettings };
    try {
        const raw = await exports.connection.workspace.getConfiguration({ section: 'vitte.indexer' });
        const out = { ...defaultIndexerSettings, ...(raw ?? {}) };
        if (!Array.isArray(out.exclude))
            out.exclude = defaultIndexerSettings.exclude;
        out.maxFiles = Math.max(100, Math.min(20000, Number(out.maxFiles) || defaultIndexerSettings.maxFiles));
        out.maxFileSizeKB = Math.max(64, Math.min(10000, Number(out.maxFileSizeKB) || defaultIndexerSettings.maxFileSizeKB));
        return out;
    }
    catch {
        return { ...defaultIndexerSettings };
    }
}
async function indexWorkspaceFolders() {
    const folders = await exports.connection.workspace.getWorkspaceFolders();
    if (!folders || folders.length === 0)
        return;
    const exts = new Set([".vit", ".vitte"]);
    const settings = await getIndexerSettings();
    const maxFiles = settings.maxFiles;
    const maxSize = settings.maxFileSizeKB * 1024;
    let indexed = 0;
    const stack = [];
    const roots = [];
    for (const f of folders) {
        if (f.uri.startsWith("file://")) {
            const root = (0, node_url_1.fileURLToPath)(f.uri);
            roots.push(root.replace(/\\/g, "/"));
            stack.push(root);
        }
    }
    const excludeMatchers = compileExcludeMatchers(settings.exclude);
    while (stack.length > 0 && indexed < maxFiles) {
        const dir = stack.pop();
        if (isExcluded(dir, excludeMatchers, roots))
            continue;
        let entries;
        try {
            entries = await promises_1.default.readdir(dir, { withFileTypes: true });
        }
        catch {
            continue;
        }
        for (const ent of entries) {
            if (indexed >= maxFiles)
                break;
            const full = node_path_1.default.join(dir, ent.name);
            if (ent.isDirectory()) {
                if (!ent.name.startsWith(".") && !isExcluded(full, excludeMatchers, roots)) {
                    stack.push(full);
                }
                continue;
            }
            if (!ent.isFile())
                continue;
            const ext = node_path_1.default.extname(ent.name).toLowerCase();
            if (!exts.has(ext))
                continue;
            const text = await readFileCached(full);
            if (!text)
                continue;
            if (text.length > maxSize)
                continue;
            const uri = (0, node_url_1.pathToFileURL)(full).toString();
            (0, indexer_1.indexText)(uri, text);
            indexed++;
        }
    }
    logger_1.logLsp.info("Workspace index built", { files: indexed });
}
// ---------------------------------------------------------------------------
// Initialize / Initialized
// ---------------------------------------------------------------------------
exports.connection.onInitialize((params) => {
    const caps = config_1.default.initConfigFromInitialize(params);
    hasConfigurationCapability = caps.hasConfigurationCapability;
    hasWorkspaceFolderCapability = caps.hasWorkspaceFolderCapability;
    logger_1.logLsp.info('Vitte LSP: initializing', caps);
    const result = {
        capabilities: {
            textDocumentSync: node_1.TextDocumentSyncKind.Incremental,
            // Completion via completion.ts (with resolve)
            completionProvider: { resolveProvider: true, triggerCharacters: (0, completion_1.triggerCharacters)() },
            hoverProvider: true,
            definitionProvider: true,
            referencesProvider: true,
            documentSymbolProvider: true,
            workspaceSymbolProvider: true,
            semanticTokensProvider: { legend: semanticLegend, full: true, range: false },
            // Formatting endpoints are exposed via custom requests below
            documentFormattingProvider: true,
            documentRangeFormattingProvider: true,
        },
    };
    if (hasWorkspaceFolderCapability) {
        result.capabilities.workspace = {
            workspaceFolders: { supported: true, changeNotifications: true },
        };
    }
    return result;
});
exports.connection.onInitialized(() => {
    logger_1.logLsp.info('Vitte LSP: initialized');
    if (hasConfigurationCapability) {
        void exports.connection.client.register(node_1.DidChangeConfigurationNotification.type, undefined);
        logger_1.logLsp.debug('Registered DidChangeConfiguration');
        void exports.connection.client.register(node_1.DidChangeWatchedFilesNotification.type, {
            watchers: [
                { globPattern: '**/*.vit' },
                { globPattern: '**/*.vitte' },
            ],
        });
    }
    void indexWorkspaceFolders();
});
// ---------------------------------------------------------------------------
// Configuration changes → clear settings & revalidate
// ---------------------------------------------------------------------------
const handleConfigChange = config_1.default.makeOnDidChangeConfigurationHandler(exports.connection, {
    getOpenDocuments: () => documents.all(),
    validateDocument: (doc) => {
        try {
            const diagnostics = (0, lint_1.lintToPublishable)(doc.getText(), doc.uri);
            void exports.connection.sendDiagnostics({ uri: doc.uri, diagnostics });
        }
        catch (e) {
            logger_1.logLsp.warn('Validation error after config change', { uri: doc.uri, error: String(e) });
        }
    },
});
exports.connection.onDidChangeConfiguration(() => {
    void handleConfigChange();
});
async function handleWatchedFiles(e) {
    for (const change of e.changes) {
        const uri = change.uri;
        if (!uri.startsWith("file://"))
            continue;
        const filePath = (0, node_url_1.fileURLToPath)(uri);
        if (change.type === node_1.FileChangeType.Deleted) {
            (0, indexer_1.removeDocument)(uri);
            fileCache.delete(filePath);
            continue;
        }
        if (change.type === node_1.FileChangeType.Changed || change.type === node_1.FileChangeType.Created) {
            const text = await readFileCached(filePath);
            if (text)
                (0, indexer_1.indexText)(uri, text);
        }
    }
}
exports.connection.onDidChangeWatchedFiles((e) => {
    void handleWatchedFiles(e);
});
// ---------------------------------------------------------------------------
// Diagnostics lifecycle
// ---------------------------------------------------------------------------
documents.onDidOpen((e) => { void handleValidate(e.document); });
documents.onDidChangeContent((e) => { void handleValidate(e.document); });
documents.onDidClose((e) => {
    // Clear diagnostics when closing
    void exports.connection.sendDiagnostics({ uri: e.document.uri, diagnostics: [] });
    (0, indexer_1.removeDocument)(e.document.uri);
});
function handleValidate(doc) {
    try {
        const diags = (0, lint_1.lintToPublishable)(doc.getText(), doc.uri);
        void exports.connection.sendDiagnostics({ uri: doc.uri, diagnostics: diags });
        logger_1.logLsp.debug('Diagnostics sent', { uri: doc.uri, count: diags.length });
    }
    catch (err) {
        logger_1.logLsp.error('Validation failed', { uri: doc.uri, error: String(err) });
    }
}
// Indexation pour navigation/complétions workspace
documents.onDidOpen((e) => { (0, indexer_1.indexDocument)(e.document); });
documents.onDidChangeContent((e) => { (0, indexer_1.updateDocument)(e.document); });
// ---------------------------------------------------------------------------
// Completion
// ---------------------------------------------------------------------------
exports.connection.onCompletion((params) => {
    try {
        const doc = documents.get(params.textDocument.uri);
        if (!doc)
            return [];
        return (0, completion_1.provideCompletions)(doc, params.position);
    }
    catch (err) {
        logger_1.logLsp.error('Completion failed', { error: String(err) });
        return [
            { label: 'error', kind: node_1.CompletionItemKind.Text, detail: 'Error during completion' },
        ];
    }
});
exports.connection.onCompletionResolve((item) => {
    try {
        return (0, completion_1.resolveCompletion)(item);
    }
    catch {
        return item;
    }
});
// ---------------------------------------------------------------------------
// Hover
// ---------------------------------------------------------------------------
exports.connection.onHover((params) => {
    const doc = documents.get(params.textDocument.uri);
    if (!doc)
        return null;
    const kw = (0, semantic_1.provideHover)(doc, params.position);
    if (kw)
        return kw;
    const word = wordAt(doc, params.position);
    if (!word)
        return null;
    const defs = (0, indexer_1.getDocumentIndex)(doc.uri).filter(s => s.name === word);
    if (defs.length === 0)
        return null;
    const closest = defs.sort((a, b) => Math.abs(a.line - params.position.line) - Math.abs(b.line - params.position.line))[0];
    const docstring = (0, docstrings_1.getDocstringAtLine)(doc, closest.line);
    if (!docstring)
        return null;
    return {
        contents: {
            kind: node_1.MarkupKind.Markdown,
            value: `**${word}**\n\n${docstring}`,
        },
    };
});
// ---------------------------------------------------------------------------
// Definition / References
// ---------------------------------------------------------------------------
exports.connection.onDefinition((params) => {
    const doc = documents.get(params.textDocument.uri);
    if (!doc)
        return [];
    return (0, navigation_1.definitionAtPosition)(doc, params.position, params.textDocument.uri);
});
exports.connection.onReferences(async (params) => {
    const doc = documents.get(params.textDocument.uri);
    if (!doc)
        return [];
    const word = wordAt(doc, params.position);
    if (!word)
        return [];
    const out = [];
    const uris = new Set();
    for (const uri of (0, indexer_1.getIndex)().keys())
        uris.add(uri);
    uris.add(doc.uri);
    for (const uri of uris) {
        const open = documents.get(uri);
        if (open) {
            out.push(...findWordLocationsInText(open.getText(), uri, word));
            continue;
        }
        if (!uri.startsWith("file://"))
            continue;
        const text = await readFileCached((0, node_url_1.fileURLToPath)(uri));
        if (text)
            out.push(...findWordLocationsInText(text, uri, word));
    }
    return out;
});
// ---------------------------------------------------------------------------
// Symbols
// ---------------------------------------------------------------------------
exports.connection.onDocumentSymbol((params) => {
    const doc = documents.get(params.textDocument.uri);
    if (!doc)
        return [];
    return (0, navigation_1.documentSymbols)(doc);
});
exports.connection.onWorkspaceSymbol((params) => {
    const q = params.query ?? '';
    const symbols = (0, indexer_1.searchWorkspaceSymbols)(q, 200);
    return (0, indexer_1.toWorkspaceSymbols)(symbols);
});
// ---------------------------------------------------------------------------
// Semantic tokens
// ---------------------------------------------------------------------------
exports.connection.languages.semanticTokens.on((params) => {
    const doc = documents.get(params.textDocument.uri);
    if (!doc)
        return { data: [] };
    return (0, semantic_1.buildSemanticTokens)(doc);
});
// ---------------------------------------------------------------------------
// Formatting (document, range, documentOrRange)
// ---------------------------------------------------------------------------
exports.connection.onDocumentFormatting(async (params) => {
    const uri = params.textDocument.uri;
    const doc = documents.get(uri);
    if (!doc)
        return [];
    let opts;
    try {
        opts = await config_1.default.getFormattingSettings(exports.connection, uri);
    }
    catch {
        opts = config_1.defaultFormatting;
    }
    return (0, formatting_1.provideFormattingEdits)(doc, toExtraFormatting(opts));
});
exports.connection.onDocumentRangeFormatting(async (params) => {
    const uri = params.textDocument.uri;
    const doc = documents.get(uri);
    if (!doc)
        return [];
    const lspRange = params.range;
    if (!lspRange)
        return [];
    let opts;
    try {
        opts = await config_1.default.getFormattingSettings(exports.connection, uri);
    }
    catch {
        opts = config_1.defaultFormatting;
    }
    const start = doc.offsetAt(lspRange.start);
    const end = doc.offsetAt(lspRange.end);
    const text = doc.getText();
    const target = text.slice(start, end);
    const targetNorm = (0, text_1.normalizeIndentation)(target, opts.insertSpaces, opts.tabSize);
    const formattedTarget = formatText(targetNorm, opts, /*isFragment*/ true);
    if (formattedTarget === target)
        return [];
    return [{ range: lspRange, newText: formattedTarget }];
});
exports.connection.onRequest('vitte/formatDocument', async (params) => {
    const uri = params?.textDocument?.uri;
    if (!uri)
        return [];
    const doc = documents.get(uri);
    if (!doc)
        return [];
    let opts;
    try {
        opts = await config_1.default.getFormattingSettings(exports.connection, uri);
    }
    catch {
        opts = config_1.defaultFormatting;
    }
    const original = doc.getText();
    // Normalize indentation quickly before running formatter logic
    const pre = (0, text_1.normalizeIndentation)(original, opts.insertSpaces, opts.tabSize);
    const formatted = formatText(pre, opts);
    // Return minimal edits
    const edits = (0, text_1.computeMinimalSmartEdits)(original, formatted);
    logger_1.logLsp.info('Formatter produced edit(s)', { uri, edits: edits.length });
    return edits;
});
exports.connection.onRequest('vitte/formatRange', async (params) => {
    const uri = params?.textDocument?.uri;
    if (!uri)
        return [];
    const doc = documents.get(uri);
    if (!doc)
        return [];
    const lspRange = params?.range;
    if (!lspRange)
        return [];
    let opts;
    try {
        opts = await config_1.default.getFormattingSettings(exports.connection, uri);
    }
    catch {
        opts = config_1.defaultFormatting;
    }
    const start = doc.offsetAt(lspRange.start);
    const end = doc.offsetAt(lspRange.end);
    const text = doc.getText();
    const target = text.slice(start, end);
    const targetNorm = (0, text_1.normalizeIndentation)(target, opts.insertSpaces, opts.tabSize);
    const formattedTarget = formatText(targetNorm, opts, /*isFragment*/ true);
    if (formattedTarget === target)
        return [];
    return [{ range: lspRange, newText: formattedTarget }];
});
exports.connection.onRequest('vitte/formatDocumentOrRange', async (params) => {
    const uri = params?.textDocument?.uri;
    if (!uri)
        return [];
    const doc = documents.get(uri);
    if (!doc)
        return [];
    let opts;
    try {
        opts = await config_1.default.getFormattingSettings(exports.connection, uri);
    }
    catch {
        opts = config_1.defaultFormatting;
    }
    const lspRange = params?.range;
    const original = doc.getText();
    if (!lspRange) {
        const pre = (0, text_1.normalizeIndentation)(original, opts.insertSpaces, opts.tabSize);
        const formatted = formatText(pre, opts);
        const edits = (0, text_1.computeMinimalSmartEdits)(original, formatted);
        return edits;
    }
    else {
        const start = doc.offsetAt(lspRange.start);
        const end = doc.offsetAt(lspRange.end);
        const fragment = original.slice(start, end);
        const fragmentNorm = (0, text_1.normalizeIndentation)(fragment, opts.insertSpaces, opts.tabSize);
        const formattedFragment = formatText(fragmentNorm, opts, /*isFragment*/ true);
        if (formattedFragment === fragment)
            return [];
        return [{ range: lspRange, newText: formattedFragment }];
    }
});
// ---------------------------------------------------------------------------
// Extra: expand selection to enclosing brackets
// ---------------------------------------------------------------------------
exports.connection.onRequest('vitte/expandSelectionToEnclosingBrackets', (params) => {
    const uri = params?.textDocument?.uri;
    if (!uri)
        return null;
    const doc = documents.get(uri);
    if (!doc)
        return null;
    const text = doc.getText();
    const range = params.position ? (0, text_1.expandSelectionToEnclosingBrackets)(text, params.position) : null;
    return range ?? null;
});
// ---------------------------------------------------------------------------
// Basic formatter (whitespace + EOL policy) — deterministic & quick
// ---------------------------------------------------------------------------
function normalizeEol(text, eol) {
    if (eol === 'auto')
        return text;
    if (eol === 'crlf')
        return text.replace(/\r\n/g, '\n').replace(/\n/g, '\r\n');
    return text.replace(/\r\n/g, '\n');
}
function applyWhitespacePolicy(lines, opts) {
    const unit = opts.insertSpaces ? ' '.repeat(Math.max(1, opts.tabSize)) : '\t';
    return lines.map((line) => {
        if (opts.insertSpaces) {
            const leadingMatch = /^\t+/.exec(line);
            const leading = leadingMatch?.[0] ?? '';
            if (leading.length > 0) {
                line = leading
                    .split('')
                    .map(() => unit)
                    .join('') + line.slice(leading.length);
            }
        }
        else {
            const re = new RegExp(`^(?: {${opts.tabSize}})+`);
            const match = re.exec(line);
            if (match) {
                const spaces = match[0].length;
                const tabs = Math.floor(spaces / opts.tabSize);
                line = '\t'.repeat(tabs) + line.slice(spaces);
            }
        }
        if (opts.trimTrailingWhitespace)
            line = line.replace(/[ \t]+$/g, '');
        return line;
    });
}
function formatText(text, opts, isFragment = false) {
    let work = text.replace(/\r\n/g, '\n');
    let lines = work.split('\n');
    lines = applyWhitespacePolicy(lines, opts);
    work = lines.join('\n');
    work = normalizeEol(work, opts.eol);
    if (!isFragment && opts.insertFinalNewline) {
        const term = opts.eol === 'crlf' ? '\r\n' : '\n';
        if (!work.endsWith(term))
            work += term;
    }
    return work;
}
// ---------------------------------------------------------------------------
// Start server
// ---------------------------------------------------------------------------
documents.listen(exports.connection);
exports.connection.listen();
logger_1.logLsp.info('Vitte LSP: listening');
//# sourceMappingURL=lsp.js.map