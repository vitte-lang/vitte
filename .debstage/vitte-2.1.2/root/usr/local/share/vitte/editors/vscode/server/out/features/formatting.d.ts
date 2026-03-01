/**
 * Vitte Language Server — Formatting Helpers
 * ------------------------------------------
 * Pure deterministic text formatter with no LSP side effects.
 * Provides whitespace normalization, indentation policy enforcement,
 * EOL normalization, and final newline management.
 */
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
/**
 * Detects indentation style in a given text sample.
 * Returns true if spaces are dominant, false if tabs are dominant.
 */
export declare function detectIndentationStyle(text: string): boolean;
/**
 * Normalize EOLs in text to desired format.
 */
export declare function normalizeEol(text: string, eol: EolKind): string;
/**
 * Apply indentation and trailing space policy.
 */
export declare function applyWhitespacePolicy(lines: string[], opts: FormattingSettings): string[];
/**
 * Determines appropriate EOL character sequence for a text sample.
 */
export declare function detectEol(text: string): EolKind;
/**
 * Performs deterministic format of provided text according to settings.
 * @param text - Original text
 * @param opts - Formatting settings
 * @param isFragment - Whether text is a fragment or full document
 * @returns Formatted text
 */
export declare function formatText(text: string, opts: FormattingSettings, isFragment?: boolean): string;
/**
 * Compare two formatted texts for equality ignoring EOL differences.
 */
export declare function isSameFormat(a: string, b: string): boolean;
/**
 * Utility to quickly preview what would change by formatting.
 */
export declare function diffPreview(before: string, after: string, contextLines?: number): string;
