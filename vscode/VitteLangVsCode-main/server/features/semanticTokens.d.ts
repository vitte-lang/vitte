/**
 * Vitte Language Server — Semantic Tokens Feature
 * ------------------------------------------------
 * Provides a minimal, deterministic tokenizer for Vitte source files.
 * No LSP side effects — pure computation only.
 *
 * Supported token types:
 * - keyword
 * - string
 * - number
 * - function
 * - variable
 * - type
 * - comment
 */
/**
 * Vitte Language Server — Semantic Tokens Feature (Extended)
 * ---------------------------------------------------------
 * Deterministic, side‑effect‑free tokenizer for Vitte source files.
 *
 * Coverage:
 * - Keywords, identifiers with context (function/type/variable).
 * - Numbers (decimal, hex 0x, bin 0b, octal 0o, floats with exponent).
 * - Strings with escapes (single‑line) and block/line comments.
 * - Multiline block comments `/* ... *\/` supported via state.
 * - Token modifiers: declaration (after fn/struct/enum/trait/type/let/const),
 *   readonly (const), async (async keyword before fn).
 */
import type { SemanticTokens, SemanticTokensLegend } from 'vscode-languageserver';
export declare const legend: SemanticTokensLegend;
/** Tokenizes the whole document into `SemanticTokens`. */
export declare function tokenize(text: string): SemanticTokens;
/** Provides an empty SemanticTokens result (for fallback or invalid documents). */
export declare function emptyTokens(): SemanticTokens;
