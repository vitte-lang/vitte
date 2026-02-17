/**
 * Vitte LSP — Configuration module (ultra complete)
 * -------------------------------------------------
 * Centralizes client capability detection, per-document settings cache,
 * validation/normalization of user configuration, and helpers to wire
 * configuration change events. Dependency-free and side-effect free
 * (except for functions that explicitly take a `Connection`).
 */
import type { Connection, InitializeParams } from 'vscode-languageserver';
export type EolKind = 'lf' | 'crlf' | 'auto';
export interface FormattingSettings {
    tabSize: number;
    insertSpaces: boolean;
    trimTrailingWhitespace: boolean;
    insertFinalNewline: boolean;
    eol: EolKind;
    detectMixedIndentation?: boolean;
}
export declare const defaultFormatting: FormattingSettings;
export interface ConfigState {
    hasConfigurationCapability: boolean;
    hasWorkspaceFolderCapability: boolean;
}
export declare function initConfigFromInitialize(params: InitializeParams): ConfigState;
export declare function getConfigState(): ConfigState;
/**
 * Returns formatting settings for a given resource URI. If the client does not
 * support workspace configuration, returns the default settings.
 */
export declare function getFormattingSettings(connection: Connection, resourceUri: string): Thenable<FormattingSettings>;
/** Clears all cached per-document settings or a single document when `uri` is provided. */
export declare function clearDocumentSettings(uri?: string): void;
export declare function validateAndNormalizeFormatting(raw: Partial<FormattingSettings> | undefined | null): FormattingSettings;
/**
 * Create a handler for `connection.onDidChangeConfiguration`.
 * You can pass an optional callback to revalidate open documents.
 */
export declare function makeOnDidChangeConfigurationHandler<TDoc = {
    uri: string;
}>(connection: Connection, options?: {
    /** List of currently open documents to revalidate. */
    getOpenDocuments?: () => Iterable<TDoc>;
    /** Callback after cache clear to revalidate one document. */
    validateDocument?: (doc: TDoc) => Promise<void> | void;
}): () => Promise<void>;
/**
 * Informational JSON schema-like description for `vitte.formatting`.
 * Can be exposed by clients or used in docs; this module does not enforce it.
 */
export declare const formattingSchema: {
    type: string;
    properties: {
        tabSize: {
            type: string;
            minimum: number;
            maximum: number;
            default: number;
        };
        insertSpaces: {
            type: string;
            default: boolean;
        };
        trimTrailingWhitespace: {
            type: string;
            default: boolean;
        };
        insertFinalNewline: {
            type: string;
            default: boolean;
        };
        eol: {
            enum: string[];
            default: string;
        };
        detectMixedIndentation: {
            type: string;
            default: boolean;
        };
    };
    additionalProperties: boolean;
};
/** Expose a small facade so `lsp.ts` can import a stable API. */
export declare const Config: {
    state: ConfigState;
    initConfigFromInitialize: typeof initConfigFromInitialize;
    getConfigState: typeof getConfigState;
    getFormattingSettings: typeof getFormattingSettings;
    clearDocumentSettings: typeof clearDocumentSettings;
    validateAndNormalizeFormatting: typeof validateAndNormalizeFormatting;
    makeOnDidChangeConfigurationHandler: typeof makeOnDidChangeConfigurationHandler;
    defaultFormatting: FormattingSettings;
    formattingSchema: {
        type: string;
        properties: {
            tabSize: {
                type: string;
                minimum: number;
                maximum: number;
                default: number;
            };
            insertSpaces: {
                type: string;
                default: boolean;
            };
            trimTrailingWhitespace: {
                type: string;
                default: boolean;
            };
            insertFinalNewline: {
                type: string;
                default: boolean;
            };
            eol: {
                enum: string[];
                default: string;
            };
            detectMixedIndentation: {
                type: string;
                default: boolean;
            };
        };
        additionalProperties: boolean;
    };
};
export default Config;
