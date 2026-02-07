
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

export const LF = '\n' as const;
export const CRLF = '\r\n' as const;
export const CR = '\r' as const; // Rare; tracked for completeness.

// ---------------------------------------------------------------------------
// Stats & detection
// ---------------------------------------------------------------------------

export interface EolStats {
  lf: number;          // Count of LF characters that are not part of CRLF.
  crlf: number;        // Count of CRLF sequences.
  cr: number;          // Count of lone CR characters (old Mac style).
  lastLineHasTerminator: boolean; // Whether the last line ends with an EOL.
}

/** Returns counts of CRLF sequences, standalone LF/CR, and final‑newline flag. */
export function measureEol(text: string): EolStats {
  if (text.length === 0) return { lf: 0, crlf: 0, cr: 0, lastLineHasTerminator: false };

  let lf = 0;
  let crlf = 0;
  let cr = 0;

  // Fast path: short‑circuit when there is no CR at all.
  if (!text.includes('\r')) {
    for (let i = 0; i < text.length; i++) if (text.charCodeAt(i) === 10) lf++;
    return { lf, crlf, cr, lastLineHasTerminator: text.endsWith('\n') };
  }

  for (let i = 0; i < text.length; i++) {
    const ch = text.charCodeAt(i);
    if (ch === 13 /*\r*/) {
      if (i + 1 < text.length && text.charCodeAt(i + 1) === 10 /*\n*/) {
        crlf++; i++; // Skip the \n of CRLF.
      } else {
        cr++;
      }
    } else if (ch === 10 /*\n*/) {
      lf++;
    }
  }
  const lastLineHasTerminator = text.endsWith('\n') || text.endsWith('\r');
  return { lf, crlf, cr, lastLineHasTerminator };
}

/** True if CRLF and bare LF/CR coexist (mixed file endings). */
export function hasMixedEol(text: string): boolean {
  const s = measureEol(text);
  const hasBare = s.lf > 0 || s.cr > 0;
  return s.crlf > 0 && hasBare;
}

/** Detects dominant EOL with simple majority. Ties favor LF. */
export function detectEol(text: string): EolKind {
  const s = measureEol(text);
  const bareTotal = s.lf + s.cr;
  if (s.crlf === 0 && bareTotal === 0) return 'lf';
  return s.crlf > bareTotal ? 'crlf' : 'lf';
}

/**
 * Detects dominant EOL using a bias and tiebreaker.
 * @param text Source text.
 * @param bias When absolute difference < `bias`, returns `tiebreak`.
 * @param tiebreak Winner when counts are equal or within `bias` (default 'lf').
 */
export function detectEolThreshold(text: string, bias = 0, tiebreak: Exclude<EolKind,'auto'> = 'lf'): Exclude<EolKind,'auto'> {
  const s = measureEol(text);
  const lfLike = s.lf + s.cr;
  const diff = Math.abs(s.crlf - lfLike);
  if (diff <= bias) return tiebreak;
  return s.crlf > lfLike ? 'crlf' : 'lf';
}

/** Counts logical lines regardless of EOL kind. */
export function countLines(text: string): number {
  if (text.length === 0) return 1;
  // Normalize CRLF/CR to LF for counting; do not allocate per line.
  let lines = 1;
  for (let i = 0; i < text.length; i++) {
    const c = text.charCodeAt(i);
    if (c === 13 /*\r*/) {
      if (i + 1 < text.length && text.charCodeAt(i + 1) === 10 /*\n*/) i++;
      lines++;
    } else if (c === 10 /*\n*/) {
      lines++;
    }
  }
  return lines;
}

/** Whether the last line ends with an EOL terminator. */
export function isTerminated(text: string): boolean {
  return text.endsWith('\n') || text.endsWith('\r');
}

// ---------------------------------------------------------------------------
// Normalization & policies
// ---------------------------------------------------------------------------

/**
 * Normalizes EOL sequences to target `eol` (CRLF and CR → LF, then LF → CRLF if needed).
 * If `eol === 'auto'`, returns the input unchanged.
 */
export function normalizeEol(text: string, eol: EolKind): string {
  if (eol === 'auto') return text;
  const work = text.replace(/\r\n/g, '\n').replace(/\r/g, '\n');
  if (eol === 'lf') return work;
  return work.replace(/\n/g, CRLF);
}

/** Ensures a single final newline if requested. */
export function ensureFinalNewline(text: string, eol: EolKind = 'lf'): string {
  const terminator = eol === 'crlf' ? CRLF : LF;
  return text.endsWith(terminator) ? text : text + terminator;
}

/** Removes any trailing newline at the end of the text (LF, CRLF or CR). */
export function stripFinalNewline(text: string): string {
  if (text.endsWith(CRLF)) return text.slice(0, -2);
  if (text.endsWith(LF) || text.endsWith(CR)) return text.slice(0, -1);
  return text;
}

/** Splits text into lines, accepting CRLF, LF and CR. */
export function splitLines(text: string): string[] {
  const endsWithEol = /\r?\n|\r$/.test(text);
  const parts = text.replace(/\r\n/g, '\n').replace(/\r/g, '\n').split('\n');
  if (!endsWithEol && parts.length > 0 && parts[parts.length - 1] === '') parts.pop();
  return parts;
}

/** Joins lines with the given EOL kind. For 'auto', defaults to LF. */
export function joinLines(lines: string[], eol: EolKind): string {
  const sep = eol === 'crlf' ? CRLF : LF; // auto → LF
  return lines.join(sep);
}

/** Quick helper to decide output EOL based on user choice and input text. */
export function chooseOutputEol(preferred: EolKind, sourceText: string, fallback: EolKind = 'lf'): EolKind {
  if (preferred === 'auto') return detectEol(sourceText) || fallback;
  return preferred;
}

export type EolPolicyMode = 'preserve' | 'lf' | 'crlf' | 'auto';

export interface EolPolicy {
  mode: EolPolicyMode;            // How to decide output EOL.
  ensureFinalNewline?: boolean;   // Ensure exactly one final newline.
  stripFinalNewline?: boolean;    // Force no trailing newline.
}

/** Applies an EOL policy in a single pass (ensure wins over strip when both). */
export function applyEolPolicy(text: string, policy: EolPolicy): string {
  const target: EolKind = policy.mode === 'preserve' ? 'auto' : (policy.mode as EolKind);
  let out = normalizeEol(text, target === 'auto' ? detectEol(text) : target);
  if (policy.stripFinalNewline) out = stripFinalNewline(out);
  if (policy.ensureFinalNewline) out = ensureFinalNewline(out, target === 'auto' ? detectEol(text) : target);
  return out;
}

// ---------------------------------------------------------------------------
// Streaming normalization (for large texts/chunks)
// ---------------------------------------------------------------------------

export interface EolTransformState {
  /** True if previous chunk ended with a bare CR ("\r"). */
  carryCR?: boolean;
}

/**
 * Normalizes a chunk to LF (CRLF/CR → LF). Maintains state for boundaries.
 * Returns normalized output and updated state.
 */
export function normalizeChunkToLf(chunk: string, state: EolTransformState = {}): { output: string; state: EolTransformState } {
  let out = '';
  let i = 0;
  if (state.carryCR) {
    // If this chunk starts with \n, the boundary was a CRLF.
    if (chunk.charCodeAt(0) === 10 /*\n*/) {
      // Drop the CR from previous chunk; emit a single LF.
      out += '\n';
      i = 1;
    } else {
      // Lone CR at boundary → convert to LF.
      out += '\n';
    }
    state.carryCR = false;
  }

  for (; i < chunk.length; i++) {
    const c = chunk.charCodeAt(i);
    if (c === 13 /*\r*/) {
      if (i + 1 < chunk.length && chunk.charCodeAt(i + 1) === 10 /*\n*/) {
        out += '\n';
        i++; // Skip the \n.
      } else {
        // May be CRLF across boundary; carry.
        state.carryCR = true;
      }
    } else if (c === 10 /*\n*/) {
      out += '\n';
    } else {
      out += chunk[i];
    }
  }

  return { output: out, state };
}

/** Converts LF‑normalized chunk to target EOL. */
export function expandLfChunk(chunkWithLf: string, target: Exclude<EolKind,'auto'>): string {
  if (target === 'lf') return chunkWithLf;
  // Convert LF → CRLF.
  return chunkWithLf.replace(/\n/g, CRLF);
}

// ---------------------------------------------------------------------------
// Offsets, positions, and range remapping
// ---------------------------------------------------------------------------

export interface Position { line: number; character: number }
export interface Range { start: Position; end: Position }

/** Returns an array of line start offsets for fast position mapping. */
export function indexLineStarts(text: string): number[] {
  const starts: number[] = [0];
  for (let i = 0; i < text.length; i++) {
    const ch = text.charCodeAt(i);
    if (ch === 13 /*\r*/) {
      if (i + 1 < text.length && text.charCodeAt(i + 1) === 10 /*\n*/) { i++; }
      starts.push(i + 1);
    } else if (ch === 10 /*\n*/) {
      starts.push(i + 1);
    }
  }
  return starts;
}

/** Maps a character offset to a Position using a precomputed line index. */
export function positionAt(offset: number, lineStarts: number[]): Position {
  offset = Math.max(0, offset);
  let lo = 0, hi = lineStarts.length - 1;
  while (lo <= hi) {
    const mid = (lo + hi) >>> 1;
    const lineStart = lineStarts[mid];
    if (lineStart === offset) return { line: mid, character: 0 };
    if (lineStart < offset) lo = mid + 1; else hi = mid - 1;
  }
  const line = Math.max(0, lo - 1);
  return { line, character: offset - lineStarts[line] };
}

/** Maps a Position to a character offset using a precomputed line index. */
export function offsetAt(pos: Position, lineStarts: number[]): number {
  const line = Math.min(Math.max(0, pos.line), Math.max(0, lineStarts.length - 1));
  return lineStarts[line] + Math.max(0, pos.character);
}

/**
 * Builds a mapping of CRLF→LF normalization for a given text.
 * Returns the set of offsets (in LF‑normalized text) where an original CR preceded an LF.
 * Useful to remap LF columns back to CRLF columns (add +1 per occurrence before offset).
 */
export function buildCrlfToLfMap(text: string): Uint32Array {
  const marks: number[] = [];
  for (let i = 0; i + 1 < text.length; i++) {
    if (text.charCodeAt(i) === 13 /*\r*/ && text.charCodeAt(i + 1) === 10 /*\n*/) {
      // After normalization, only the \n remains at index i.
      marks.push(i + 1); // Index of the LF in the normalized text.
      i++; // Skip LF.
    }
  }
  return Uint32Array.from(marks);
}

/** Remaps an offset from LF‑normalized back to CRLF view using a mark array. */
export function remapLfOffsetToCrlfWithMap(lfOffset: number, crlfMarks: Uint32Array): number {
  // Binary‑search upper bound of marks < lfOffset.
  let lo = 0, hi = crlfMarks.length;
  while (lo < hi) {
    const mid = (lo + hi) >>> 1;
    if (crlfMarks[mid] < lfOffset) lo = mid + 1; else hi = mid;
  }
  return lfOffset + lo; // Add one extra char per CRLF before the offset.
}

/** Remaps a Range from LF‑normalized view to CRLF view. */
export function remapRangeLfToCrlf(range: Range, crlfMarks: Uint32Array, lineStartsLf: number[]): Range {
  const startOff = offsetAt(range.start, lineStartsLf);
  const endOff = offsetAt(range.end, lineStartsLf);
  const start2 = remapLfOffsetToCrlfWithMap(startOff, crlfMarks);
  const end2 = remapLfOffsetToCrlfWithMap(endOff, crlfMarks);
  return {
    start: positionAt(start2, lineStartsLf),
    end: positionAt(end2, lineStartsLf),
  };
}

/** Normalizes a range to have start <= end and non‑negative values. */
export function normalizeRange(a: Range): Range {
  const s = (a.start.line > a.end.line || (a.start.line === a.end.line && a.start.character > a.end.character))
    ? { start: a.end, end: a.start }
    : a;
  return {
    start: { line: Math.max(0, s.start.line), character: Math.max(0, s.start.character) },
    end:   { line: Math.max(0, s.end.line),   character: Math.max(0, s.end.character) },
  };
}

// ---------------------------------------------------------------------------
// Extra helpers
// ---------------------------------------------------------------------------

/** Splits text and returns pairs [line, delimiter] where delimiter is one of "\n", "\r\n", "\r" or "" for the last line. */
export function splitLinesWithDelimiters(text: string): { line: string; delim: '' | typeof LF | typeof CRLF | typeof CR }[] {
  const out: { line: string; delim: '' | typeof LF | typeof CRLF | typeof CR }[] = [];
  let buf = '';
  for (let i = 0; i < text.length; i++) {
    const c = text.charCodeAt(i);
    if (c === 13 /*\r*/) {
      if (i + 1 < text.length && text.charCodeAt(i + 1) === 10 /*\n*/) {
        out.push({ line: buf, delim: CRLF }); buf = ''; i++; continue;
      }
      out.push({ line: buf, delim: CR }); buf = ''; continue;
    }
    if (c === 10 /*\n*/) { out.push({ line: buf, delim: LF }); buf = ''; continue; }
    buf += text[i];
  }
  out.push({ line: buf, delim: '' });
  return out;
}

/** Joins result of splitLinesWithDelimiters, optionally overriding delimiter kind. */
export function joinLinesPreserveLastTerminator(parts: { line: string; delim: '' | typeof LF | typeof CRLF | typeof CR }[], eol: EolKind): string {
  const sep = eol === 'crlf' ? CRLF : LF; // auto → LF
  let out = '';
  for (const part of parts) {
    const { line, delim } = part;
    const useDelim = delim === '' ? '' : sep; // Preserve presence of last terminator, but convert kind.
    out += line + useDelim;
  }
  return out;
}

/** Clamps a position inside [0, maxLine] and [0, +∞) for character. */
export function clampPosition(pos: Position, maxLine: number): Position {
  const l = Math.max(0, Math.min(maxLine, pos.line));
  const c = Math.max(0, pos.character);
  return { line: l, character: c };
}

/** Compares positions; returns -1,0,1. */
export function comparePosition(a: Position, b: Position): -1 | 0 | 1 {
  if (a.line < b.line) return -1; if (a.line > b.line) return 1;
  if (a.character < b.character) return -1; if (a.character > b.character) return 1;
  return 0;
}

/** Merges overlapping or contiguous ranges (LF view). */
export function mergeRanges(ranges: Range[]): Range[] {
  if (ranges.length <= 1) return ranges.slice();
  const rs = ranges.map(normalizeRange).sort((x, y) => comparePosition(x.start, y.start));
  const out: Range[] = [];
  let cur = rs[0];
  for (let i = 1; i < rs.length; i++) {
    const r = rs[i];
    if (comparePosition(r.start, cur.end) <= 0) {
      // Overlap/adjacent → extend end.
      if (comparePosition(r.end, cur.end) > 0) cur.end = r.end;
    } else {
      out.push(cur); cur = r;
    }
  }
  out.push(cur);
  return out;
}
