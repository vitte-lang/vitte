/**
 * Vitte LSP — Language Service (complete minimal)
 * -----------------------------------------------
 * Diagnostics & completion without external parser, with configurable rules.
 * Pure TypeScript. API remains compatible with lsp.ts (doValidation, doComplete, doResolve).
 */
import type { Diagnostic, CompletionItem, TextDocumentPositionParams } from 'vscode-languageserver/node';
export interface LspTextDocument {
    uri: string;
    getText(range?: {
        start: {
            line: number;
            character: number;
        };
        end: {
            line: number;
            character: number;
        };
    }): string;
}
export interface VitteLintSettings {
    maxLineLength: number;
    warnTrailingWhitespace: boolean;
    hintTodoFixme: boolean;
    warnMixedIndent: boolean;
}
export declare class VitteLanguageService {
    private settings;
    constructor(opts?: Partial<VitteLintSettings>);
    /** Update settings at runtime without recreating the service. */
    setSettings(partial: Partial<VitteLintSettings>): void;
    /** Basic diagnostics: TODO/FIXME/XXX, long lines, trailing spaces, mixed indentation. */
    doValidation(doc: LspTextDocument): Promise<Diagnostic[]>;
    /**
     * Completions: keywords + simple snippets for declarations.
     * We avoid context parsing; snippets are provided for productivity.
     */
    doComplete(_params: TextDocumentPositionParams): CompletionItem[];
    /** Attach a bit more detail on resolve. */
    doResolve(item: CompletionItem): CompletionItem;
    private _docFor;
}
export default VitteLanguageService;
