/**
 * Vitte Language Server — Hover Helpers (Extended)
 * ------------------------------------------------
 * Pure helpers with no LSP side effects.
 * - Lightweight symbol indexer (functions, types, variables).
* - Doc comment extraction (/// and /** ... *\/).
 * - Heuristic signature preview.
 * - Rich Markdown payload with fenced code and sections.
 * - Accurate hover range (word under cursor).
 *
 * Performance: O(N) single pass over the text (line-by-line).
 */
export interface Position {
    line: number;
    character: number;
}
export interface Range {
    start: Position;
    end: Position;
}
export interface HoverContext {
    uri: string;
    text: string;
    line: number;
    character: number;
}
export interface HoverPayload {
    contents: {
        kind: 'markdown';
        value: string;
    };
    range?: Range;
}
/**
 * Provides hover payload for a position.
 * Returns null when nothing meaningful is found.
 */
export declare function provideHover(ctx: HoverContext): HoverPayload | null;
