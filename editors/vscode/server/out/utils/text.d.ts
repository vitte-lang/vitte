/**
 * Vitte LSP Utils — Text helpers (ultra complete)
 * ------------------------------------------------
 * Pure utilities for text/position/range manipulation with LSP (UTF‑16) semantics.
 * No I/O, no side effects, dependency‑free.
 *
 * IMPORTANT: JavaScript strings are UTF‑16. All offsets and lengths here are in
 * UTF‑16 code units, matching LSP expectations.
 */
export interface Position {
    line: number;
    character: number;
}
export interface Range {
    start: Position;
    end: Position;
}
export interface TextEdit {
    range: Range;
    newText: string;
}
/** Builds an array of line start offsets (UTF‑16 code units). */
export declare function indexLineStarts(text: string): number[];
/** Clamp helpers. */
export declare function clamp(v: number, lo: number, hi: number): number;
export declare function clampPosition(pos: Position, maxLine: number): Position;
/** Compare helpers. */
export declare function comparePosition(a: Position, b: Position): -1 | 0 | 1;
export declare function normalizeRange(r: Range): Range;
export declare function mergeRanges(ranges: Range[]): Range[];
/** Offset/position mapping (UTF‑16). */
export declare function positionAt(offset: number, lineStarts: number[]): Position;
export declare function offsetAt(pos: Position, lineStarts: number[]): number;
/** Full document range for a given text. */
export declare function fullDocumentRange(text: string): Range;
/** Returns text slice by range using a precomputed line index (safer & fast). */
export declare function sliceByRange(text: string, range: Range, lineStarts?: number[]): string;
/** Replaces the content at a given range. */
export declare function replaceRange(text: string, range: Range, replacement: string, lineStarts?: number[]): string;
/** Builds a TextEdit struct. */
export declare function edit(range: Range, newText: string): TextEdit;
/** Applies a set of non‑overlapping edits. If overlapping, sorts & compacts first. */
export declare function applyEdits(text: string, edits: TextEdit[]): string;
/**
 * Extracts the word at a given position using a customizable word regex.
 * Returns null if no word is found at the cursor.
 */
export interface WordAtResult {
    word: string;
    range: Range;
}
export declare function wordAtPosition(text: string, pos: Position, lineStarts?: number[], re?: RegExp): WordAtResult | null;
/** Returns the text of a given line (without its EOL). */
export declare function lineTextAt(text: string, line: number, lineStarts?: number[]): string;
/** Returns the line length (UTF‑16 units) without EOL. */
export declare function lineLengthAt(text: string, line: number, lineStarts?: number[]): number;
/** Creates a single full‑document replace edit (useful for formatters). */
export declare function fullReplaceEdit(text: string, newText: string): TextEdit;
/** Splits text into lines quickly (LF/CRLF tolerated). */
export declare function fastSplitLines(text: string): string[];
/** Counts lines without allocating per line. */
export declare function countLines(text: string): number;
/** Quick predicate: is range empty? */
export declare function isEmptyRange(r: Range): boolean;
/** Make a zero‑length range at a given position. */
export declare function cursorAt(pos: Position): Range;
/** Expand a range by delta characters on both ends (line‑local). */
export declare function expandRangeOnLine(r: Range, startDelta: number, endDelta: number): Range;
/** Check if a position is within a given range (inclusive of start, exclusive of end). */
export declare function rangeContains(range: Range, pos: Position): boolean;
/** Compute a minimal set of TextEdits to replace `oldText` by `newText` (simple fallback). */
export declare function computeMinimalEdits(oldText: string, newText: string): TextEdit[];
/** Try to create a line‑aware replace (only if both share same number of lines & prefixes/suffixes). */
export declare function computeSmartLineEdit(oldText: string, newText: string): TextEdit[] | null;
/** Factory helpers. */
export declare function makePosition(line: number, character: number): Position;
export declare function makeRange(sline: number, schar: number, eline: number, echar: number): Range;
export declare function positionEquals(a: Position, b: Position): boolean;
export declare function rangeEquals(a: Range, b: Range): boolean;
/** Convert to/from offsets (UTF‑16). */
export declare function rangeToOffsets(text: string, r: Range, starts?: number[]): [number, number];
export declare function offsetsToRange(text: string, start: number, end: number, starts?: number[]): Range;
/** Clamp a range to the text bounds. */
export declare function clampRangeToText(text: string, r: Range, starts?: number[]): Range;
/** Line helpers. */
export declare function lineRange(text: string, line: number, starts?: number[]): Range;
export declare function isBlankLine(text: string, line: number, starts?: number[]): boolean;
export declare function indentationLevelOf(lineText: string, tabSize?: number): number;
/** Normalize indentation (leading whitespace only). */
export declare function normalizeIndentation(text: string, insertSpaces: boolean, tabSize?: number): string;
/** Whitespace scanning within a line. */
export declare function nextNonWhitespaceColumn(lineText: string, from?: number): number;
export declare function prevNonWhitespaceColumn(lineText: string, from: number): number;
/** Word boundaries on a line. */
export declare function wordBoundaries(lineText: string, col: number, re?: RegExp): [number, number] | null;
export declare function findMatchingBracket(text: string, pos: Position, starts?: number[]): Range | null;
export declare function expandSelectionToEnclosingBrackets(text: string, pos: Position, starts?: number[]): Range | null;
/** Lightweight line‑based diff to produce coarse TextEdits. */
export declare function computeEditsByLine(oldText: string, newText: string): TextEdit[];
/** Strict edit application: throws on overlapping ranges. */
export declare function applyEditsStrict(text: string, edits: TextEdit[]): string;
/** Smart minimal edits: prefer line‑block replace, else full replace. */
export declare function computeMinimalSmartEdits(oldText: string, newText: string): TextEdit[];
