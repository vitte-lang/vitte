"use strict";
/**
 * Vitte LSP — Configuration module (ultra complete)
 * -------------------------------------------------
 * Centralizes client capability detection, per-document settings cache,
 * validation/normalization of user configuration, and helpers to wire
 * configuration change events. Dependency-free and side-effect free
 * (except for functions that explicitly take a `Connection`).
 */
Object.defineProperty(exports, "__esModule", { value: true });
exports.Config = exports.formattingSchema = exports.defaultFormatting = void 0;
exports.initConfigFromInitialize = initConfigFromInitialize;
exports.getConfigState = getConfigState;
exports.getFormattingSettings = getFormattingSettings;
exports.clearDocumentSettings = clearDocumentSettings;
exports.validateAndNormalizeFormatting = validateAndNormalizeFormatting;
exports.makeOnDidChangeConfigurationHandler = makeOnDidChangeConfigurationHandler;
exports.defaultFormatting = {
    tabSize: 2,
    insertSpaces: true,
    trimTrailingWhitespace: true,
    insertFinalNewline: true,
    eol: 'lf',
    detectMixedIndentation: true,
};
// Internal cache for per-document settings acquired via `workspace/configuration`.
const documentSettings = new Map();
// Global state derived from client capabilities.
const state = {
    hasConfigurationCapability: false,
    hasWorkspaceFolderCapability: false,
};
// ---------------------------------------------------------------------------
// Capability detection
// ---------------------------------------------------------------------------
function initConfigFromInitialize(params) {
    const capabilities = params.capabilities;
    state.hasConfigurationCapability = Boolean(capabilities.workspace?.configuration);
    state.hasWorkspaceFolderCapability = Boolean(capabilities.workspace?.workspaceFolders);
    return { ...state };
}
function getConfigState() {
    return { ...state };
}
// ---------------------------------------------------------------------------
// Settings retrieval & cache
// ---------------------------------------------------------------------------
/**
 * Returns formatting settings for a given resource URI. If the client does not
 * support workspace configuration, returns the default settings.
 */
function getFormattingSettings(connection, resourceUri) {
    if (!state.hasConfigurationCapability) {
        return Promise.resolve(exports.defaultFormatting);
    }
    let result = documentSettings.get(resourceUri);
    if (!result) {
        const request = connection.workspace.getConfiguration({
            scopeUri: resourceUri,
            section: 'vitte.formatting',
        });
        // Wrap with validation/normalization before caching result to callers.
        result = request.then(raw => validateAndNormalizeFormatting(raw ?? undefined));
        documentSettings.set(resourceUri, result);
    }
    return result;
}
/** Clears all cached per-document settings or a single document when `uri` is provided. */
function clearDocumentSettings(uri) {
    if (uri)
        documentSettings.delete(uri);
    else
        documentSettings.clear();
}
// ---------------------------------------------------------------------------
// Validation & normalization
// ---------------------------------------------------------------------------
function validateAndNormalizeFormatting(raw) {
    const out = { ...exports.defaultFormatting };
    if (!raw || typeof raw !== 'object')
        return out;
    // tabSize
    if (typeof raw.tabSize === 'number' && Number.isFinite(raw.tabSize)) {
        const v = Math.max(1, Math.min(16, Math.floor(raw.tabSize)));
        out.tabSize = v;
    }
    // insertSpaces
    if (typeof raw.insertSpaces === 'boolean')
        out.insertSpaces = raw.insertSpaces;
    // trimTrailingWhitespace
    if (typeof raw.trimTrailingWhitespace === 'boolean')
        out.trimTrailingWhitespace = raw.trimTrailingWhitespace;
    // insertFinalNewline
    if (typeof raw.insertFinalNewline === 'boolean')
        out.insertFinalNewline = raw.insertFinalNewline;
    // eol
    const eol = raw.eol;
    if (eol === 'lf' || eol === 'crlf' || eol === 'auto')
        out.eol = eol;
    // detectMixedIndentation
    if (typeof raw.detectMixedIndentation === 'boolean')
        out.detectMixedIndentation = raw.detectMixedIndentation;
    return out;
}
// ---------------------------------------------------------------------------
// Wiring helpers for onDidChangeConfiguration
// ---------------------------------------------------------------------------
/**
 * Create a handler for `connection.onDidChangeConfiguration`.
 * You can pass an optional callback to revalidate open documents.
 */
function makeOnDidChangeConfigurationHandler(connection, options) {
    return async function onDidChangeConfiguration() {
        clearDocumentSettings();
        if (options?.getOpenDocuments && options?.validateDocument) {
            for (const doc of options.getOpenDocuments()) {
                try {
                    await options.validateDocument(doc);
                }
                catch (e) {
                    connection.console.warn(`validateDocument error: ${String(e)}`);
                }
            }
        }
    };
}
// ---------------------------------------------------------------------------
// Optional: JSON schema (informational only; not enforced here)
// ---------------------------------------------------------------------------
/**
 * Informational JSON schema-like description for `vitte.formatting`.
 * Can be exposed by clients or used in docs; this module does not enforce it.
 */
exports.formattingSchema = {
    type: 'object',
    properties: {
        tabSize: { type: 'integer', minimum: 1, maximum: 16, default: 2 },
        insertSpaces: { type: 'boolean', default: true },
        trimTrailingWhitespace: { type: 'boolean', default: true },
        insertFinalNewline: { type: 'boolean', default: true },
        eol: { enum: ['lf', 'crlf', 'auto'], default: 'lf' },
        detectMixedIndentation: { type: 'boolean', default: true },
    },
    additionalProperties: false,
};
// ---------------------------------------------------------------------------
// Convenience helpers for server wiring
// ---------------------------------------------------------------------------
/** Expose a small facade so `lsp.ts` can import a stable API. */
exports.Config = {
    state,
    initConfigFromInitialize,
    getConfigState,
    getFormattingSettings,
    clearDocumentSettings,
    validateAndNormalizeFormatting,
    makeOnDidChangeConfigurationHandler,
    defaultFormatting: exports.defaultFormatting,
    formattingSchema: exports.formattingSchema,
};
exports.default = exports.Config;
//# sourceMappingURL=config.js.map