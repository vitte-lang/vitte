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
Object.defineProperty(exports, "__esModule", { value: true });
exports.connection = void 0;
const node_1 = require("vscode-languageserver/node");
const vscode_languageserver_textdocument_1 = require("vscode-languageserver-textdocument");
const config_1 = __importStar(require("./config"));
const logger_1 = require("./logger");
const languageService_1 = require("./languageService");
const text_1 = require("./utils/text");
// ---------------------------------------------------------------------------
// Connection & documents
// ---------------------------------------------------------------------------
exports.connection = (0, node_1.createConnection)(node_1.ProposedFeatures.all);
(0, logger_1.attachConnection)(exports.connection);
logger_1.logLsp.info('Vitte LSP: connection created');
const documents = new node_1.TextDocuments(vscode_languageserver_textdocument_1.TextDocument);
const lang = new languageService_1.VitteLanguageService();
let hasConfigurationCapability = false;
let hasWorkspaceFolderCapability = false;
function toLspDocument(doc) {
    return {
        uri: doc.uri,
        getText: doc.getText.bind(doc),
    };
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
            // We provide completion via VitteLanguageService (with resolve)
            completionProvider: { resolveProvider: true, triggerCharacters: ['.', ':', '>'] },
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
    }
});
// ---------------------------------------------------------------------------
// Configuration changes → clear settings & revalidate
// ---------------------------------------------------------------------------
const handleConfigChange = config_1.default.makeOnDidChangeConfigurationHandler(exports.connection, {
    getOpenDocuments: () => documents.all(),
    validateDocument: async (doc) => {
        try {
            const diagnostics = await lang.doValidation(toLspDocument(doc));
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
// ---------------------------------------------------------------------------
// Diagnostics lifecycle
// ---------------------------------------------------------------------------
documents.onDidOpen((e) => { void handleValidate(e.document); });
documents.onDidChangeContent((e) => { void handleValidate(e.document); });
documents.onDidClose((e) => {
    // Clear diagnostics when closing
    void exports.connection.sendDiagnostics({ uri: e.document.uri, diagnostics: [] });
});
async function handleValidate(doc) {
    try {
        const diags = await lang.doValidation(toLspDocument(doc));
        void exports.connection.sendDiagnostics({ uri: doc.uri, diagnostics: diags });
        logger_1.logLsp.debug('Diagnostics sent', { uri: doc.uri, count: diags.length });
    }
    catch (err) {
        logger_1.logLsp.error('Validation failed', { uri: doc.uri, error: String(err) });
    }
}
// ---------------------------------------------------------------------------
// Completion
// ---------------------------------------------------------------------------
exports.connection.onCompletion((params) => {
    try {
        const items = lang.doComplete(params);
        return items;
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
        return lang.doResolve(item);
    }
    catch {
        return item;
    }
});
// ---------------------------------------------------------------------------
// Formatting (document, range, documentOrRange)
// ---------------------------------------------------------------------------
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