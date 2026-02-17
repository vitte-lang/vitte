/**
 * Vitte LSP Utils — EOL helpers (ultra complete +++)
 * --------------------------------------------------
 * Pure utilities to detect, normalize, and manage end‑of‑line sequences.
 * Includes stats, policies, streaming transforms, offset/position helpers,
 * and range remapping across LF/CRLF conversions.
 *
 * All functions are dependency‑free and side‑effect‑free.
 */
export type EolKind = 'lf' | 'crlf' | 'auto';
export declare const LF: "\n";
export declare const CRLF: "\r\n";
export declare const CR: "\r";
export interface EolStats {
    lf: number;
    crlf: number;
    cr: number;
    lastLineHasTerminator: boolean;
}
/** Returns counts of CRLF sequences, standalone LF/CR, and final‑newline flag. */
export declare function measureEol(text: string): EolStats;
/** True if CRLF and bare LF/CR coexist (mixed file endings). */
export declare function hasMixedEol(text: string): boolean;
/** Detects dominant EOL with simple majority. Ties favor LF. */
export declare function detectEol(text: string): EolKind;
/**
 * Detects dominant EOL using a bias and tiebreaker.
 * @param text Source text.
 * @param bias When absolute difference < `bias`, returns `tiebreak`.
 * @param tiebreak Winner when counts are equal or within `bias` (default 'lf').
 */
export declare function detectEolThreshold(text: string, bias?: number, tiebreak?: Exclude<EolKind, 'auto'>): Exclude<EolKind, 'auto'>;
/** Counts logical lines regardless of EOL kind. */
export declare function countLines(text: string): number;
/** Whether the last line ends with an EOL terminator. */
export declare function isTerminated(text: string): boolean;
/**
 * Normalizes EOL sequences to target `eol` (CRLF and CR → LF, then LF → CRLF if needed).
 * If `eol === 'auto'`, returns the input unchanged.
 */
export declare function normalizeEol(text: string, eol: EolKind): string;
/** Ensures a single final newline if requested. */
export declare function ensureFinalNewline(text: string, eol?: EolKind): string;
/** Removes any trailing newline at the end of the text (LF, CRLF or CR). */
export declare function stripFinalNewline(text: string): string;
/** Splits text into lines, accepting CRLF, LF and CR. */
export declare function splitLines(text: string): string[];
/** Joins lines with the given EOL kind. For 'auto', defaults to LF. */
export declare function joinLines(lines: string[], eol: EolKind): string;
/** Quick helper to decide output EOL based on user choice and input text. */
export declare function chooseOutputEol(preferred: EolKind, sourceText: string, fallback?: EolKind): EolKind;
export type EolPolicyMode = 'preserve' | 'lf' | 'crlf' | 'auto';
export interface EolPolicy {
    mode: EolPolicyMode;
    ensureFinalNewline?: boolean;
    stripFinalNewline?: boolean;
}
/** Applies an EOL policy in a single pass (ensure wins over strip when both). */
export declare function applyEolPolicy(text: string, policy: EolPolicy): string;
export interface EolTransformState {
    /** True if previous chunk ended with a bare CR ("\r"). */
    carryCR?: boolean;
}
/**
 * Normalizes a chunk to LF (CRLF/CR → LF). Maintains state for boundaries.
 * Returns normalized output and updated state.
 */
export declare function normalizeChunkToLf(chunk: string, state?: EolTransformState): {
    output: string;
    state: EolTransformState;
};
/** Converts LF‑normalized chunk to target EOL. */
export declare function expandLfChunk(chunkWithLf: string, target: Exclude<EolKind, 'auto'>): string;
export interface Position {
    line: number;
    character: number;
}
export interface Range {
    start: Position;
    end: Position;
}
/** Returns an array of line start offsets for fast position mapping. */
export declare function indexLineStarts(text: string): number[];
/** Maps a character offset to a Position using a precomputed line index. */
export declare function positionAt(offset: number, lineStarts: number[]): Position;
/** Maps a Position to a character offset using a precomputed line index. */
export declare function offsetAt(pos: Position, lineStarts: number[]): number;
/**
 * Builds a mapping of CRLF→LF normalization for a given text.
 * Returns the set of offsets (in LF‑normalized text) where an original CR preceded an LF.
 * Useful to remap LF columns back to CRLF columns (add +1 per occurrence before offset).
 */
export declare function buildCrlfToLfMap(text: string): Uint32Array;
/** Remaps an offset from LF‑normalized back to CRLF view using a mark array. */
export declare function remapLfOffsetToCrlfWithMap(lfOffset: number, crlfMarks: Uint32Array): number;
/** Remaps a Range from LF‑normalized view to CRLF view. */
export declare function remapRangeLfToCrlf(range: Range, crlfMarks: Uint32Array, lineStartsLf: number[]): Range;
/** Normalizes a range to have start <= end and non‑negative values. */
export declare function normalizeRange(a: Range): Range;
/** Splits text and returns pairs [line, delimiter] where delimiter is one of "\n", "\r\n", "\r" or "" for the last line. */
export declare function splitLinesWithDelimiters(text: string): {
    line: string;
    delim: '' | typeof LF | typeof CRLF | typeof CR;
}[];
/** Joins result of splitLinesWithDelimiters, optionally overriding delimiter kind. */
export declare function joinLinesPreserveLastTerminator(parts: {
    line: string;
    delim: '' | typeof LF | typeof CRLF | typeof CR;
}[], eol: EolKind): string;
/** Clamps a position inside [0, maxLine] and [0, +∞) for character. */
export declare function clampPosition(pos: Position, maxLine: number): Position;
/** Compares positions; returns -1,0,1. */
export declare function comparePosition(a: Position, b: Position): -1 | 0 | 1;
/** Merges overlapping or contiguous ranges (LF view). */
export declare function mergeRanges(ranges: Range[]): Range[];
