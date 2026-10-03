import { type Connection, TextDocuments, type TextDocument, DiagnosticSeverity } from 'vscode-languageserver/node';
/**
 * Diagnostics subsystem for Vitte LSP — strict-safe (no implicit undefined),
 * minimal heuristics, debounced publication, and configuration hooks.
 */
export interface DiagnosticsConfig {
    maxProblems: number;
    longLine: number;
    allowTabs: boolean;
    todoSeverity: DiagnosticSeverity;
}
/** Register diagnostics on connection & documents */
export declare function registerDiagnostics(connection: Connection, documents: TextDocuments<TextDocument>): void;
export declare function updateConfig(partial: Partial<DiagnosticsConfig>): void;
