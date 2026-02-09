import { type Connection, TextDocuments, type TextDocument } from 'vscode-languageserver/node';
/**
 * Completion subsystem for Vitte LSP — strict-safe, keyword + snippet items,
 * minimal context, and resolve support. No implicit undefineds.
 */
export interface CompletionConfig {
    enableSnippets: boolean;
    maxItems: number;
}
/** Register completion feature */
export declare function registerCompletion(connection: Connection, documents: TextDocuments<TextDocument>): void;
export declare function updateConfig(partial: Partial<CompletionConfig>): void;
