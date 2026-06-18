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

export interface Position { line: number; character: number }    // 0-based
export interface Range { start: Position; end: Position }

export interface HoverContext {
  uri: string;
  text: string;
  line: number;       // 0-based
  character: number;  // 0-based
}

export interface HoverPayload {
  contents: { kind: 'markdown'; value: string };
  range?: Range;
}

type SymbolKind = 'function' | 'struct' | 'enum' | 'trait' | 'type' | 'variable';

interface SymbolInfo {
  name: string;
  kind: SymbolKind;
  range: Range;           // Name range.
  signature?: string;     // Single-line signature preview.
  doc?: string;           // Extracted doc.
  defLine?: number;       // Definition line.
  defCol?: number;        // Definition column.
}

// Minimal keyword set for classification fallback.
const KEYWORDS = new Set([
  'fn','struct','enum','union','trait','impl','let','const','mut','pub','use','mod','return','if','else','while','for','match','break','continue','async','await','yield','where','type','alias'
]);

/** Extracts the full word at a given position and its [start,end) columns. */
function wordAt(lineText: string, col: number): { word: string; start: number; end: number } | null {
  if (col < 0) return null;
  const re = /[A-Za-z0-9_]+/g;
  let m: RegExpExecArray | null;
  while ((m = re.exec(lineText))) {
    if (m.index <= col && re.lastIndex >= col) {
      return { word: m[0], start: m.index, end: re.lastIndex };
    }
  }
  return null;
}

/** Escapes Markdown fenced code content safely. */
function fence(lang: string, content: string): string {
  return '```' + lang + '\n' + content.replace(/```/g, '``\u0060') + '\n```';
}

/** Trims leading doc comment prefixes and joins lines. */
function normalizeDoc(lines: string[]): string {
  const cleaned = lines.map(l =>
    l
      .replace(/^\s*\/\/\/\s?/, '')
      .replace(/^\s*\*\s?/, '')
      .replace(/^\s*\/\*\*?\s?/, '')
      .replace(/\s*\*\/\s*$/, '')
  );
  // Collapse multiple blank lines.
  return cleaned.join('\n').replace(/\n{3,}/g, '\n\n').trim();
}

/**
 * Extracts contiguous doc comment block immediately above a given line index.
 * Supports `///` style and `/** ... *\/` style.
 */
function extractLeadingDoc(sourceLines: string[], defLine: number): string | undefined {
  const doc: string[] = [];
  // 1) Try `///` blocks directly above.
  let i = defLine - 1;
  let sawSlashDoc = false;
  while (i >= 0) {
    const line = sourceLines[i];
    if (/^\s*\/\/\//.test(line)) {
      doc.unshift(line);
      sawSlashDoc = true;
      i--;
      continue;
    }
    if (/^\s*$/.test(line)) {
      // Allow one blank line between `///` lines (still part of doc).
      if (sawSlashDoc) { i--; continue; }
    }
    break;
  }
  if (doc.length > 0) return normalizeDoc(doc);

  // 2) Try a JSDoc-style block `/** ... */` immediately above (ignoring blank line).
  i = defLine - 1;
  // Skip one blank line max.
  if (i >= 0 && /^\s*$/.test(sourceLines[i])) i--;
  if (i >= 0 && /\*\/\s*$/.test(sourceLines[i])) {
    const block: string[] = [];
    let j = i;
    while (j >= 0) {
      const L = sourceLines[j];
      block.unshift(L);
      if (/^\s*\/\*\*/.test(L)) break;
      j--;
    }
    if (block.length > 0 && /^\s*\/\*\*/.test(block[0])) {
      return normalizeDoc(block);
    }
  }
  return undefined;
}

/** Converts [startCol,endCol) to a Range for the given line. */
function rangeForLine(line: number, start: number, end: number): Range {
  return { start: { line, character: start }, end: { line, character: end } };
}

/**
 * Builds a lightweight symbol index from source lines.
 * Keeps it intentionally simple and deterministic.
 */
function indexSymbols(text: string): SymbolInfo[] {
  const symbols: SymbolInfo[] = [];
  const lines = text.split(/\r?\n/);
  const cap = Math.min(lines.length, 50_000); // Safety cap.

  for (let ln = 0; ln < cap; ln++) {
    const line = lines[ln];

    // --- Functions: fn name(args).
    {
      const m = line.match(/^\s*fn\s+([A-Za-z_]\w*)\s*\(([^)]*)\)?/);
      if (m) {
        const name = m[1];
        const nameStart = line.indexOf(name);
        const sig = `fn ${name}(${m[2] ?? ''})`;
        symbols.push({
          name,
          kind: 'function',
          range: rangeForLine(ln, nameStart, nameStart + name.length),
          signature: sig,
          doc: extractLeadingDoc(lines, ln),
          defLine: ln,
          defCol: nameStart,
        });
        continue; // One symbol per line preference.
      }
    }

    // --- Structs.
    {
      const m = line.match(/^\s*struct\s+([A-Za-z_]\w*)\b/);
      if (m) {
        const name = m[1];
        const nameStart = line.indexOf(name);
        symbols.push({
          name,
          kind: 'struct',
          range: rangeForLine(ln, nameStart, nameStart + name.length),
          signature: `struct ${name} { … }`,
          doc: extractLeadingDoc(lines, ln),
          defLine: ln, defCol: nameStart,
        });
        continue;
      }
    }

    // --- Enums.
    {
      const m = line.match(/^\s*enum\s+([A-Za-z_]\w*)\b/);
      if (m) {
        const name = m[1];
        const nameStart = line.indexOf(name);
        symbols.push({
          name,
          kind: 'enum',
          range: rangeForLine(ln, nameStart, nameStart + name.length),
          signature: `enum ${name} { … }`,
          doc: extractLeadingDoc(lines, ln),
          defLine: ln, defCol: nameStart,
        });
        continue;
      }
    }

    // --- Traits.
    {
      const m = line.match(/^\s*trait\s+([A-Za-z_]\w*)\b/);
      if (m) {
        const name = m[1];
        const nameStart = line.indexOf(name);
        symbols.push({
          name,
          kind: 'trait',
          range: rangeForLine(ln, nameStart, nameStart + name.length),
          signature: `trait ${name} { … }`,
          doc: extractLeadingDoc(lines, ln),
          defLine: ln, defCol: nameStart,
        });
        continue;
      }
    }

    // --- Type alias.
    {
      const m = line.match(/^\s*type\s+([A-Za-z_]\w*)\b/);
      if (m) {
        const name = m[1];
        const nameStart = line.indexOf(name);
        symbols.push({
          name,
          kind: 'type',
          range: rangeForLine(ln, nameStart, nameStart + name.length),
          signature: `type ${name} = …`,
          doc: extractLeadingDoc(lines, ln),
          defLine: ln, defCol: nameStart,
        });
        continue;
      }
    }

    // --- Variables (let [mut] name [: Ty] = …).
    {
      const m = line.match(/^\s*let\s+(?:mut\s+)?([A-Za-z_]\w*)(\s*:\s*([^=;]+))?/);
      if (m) {
        const name = m[1];
        const nameStart = line.indexOf(name);
        const ty = (m[3] ?? '').trim();
        const sig = ty ? `let ${name}: ${ty}` : `let ${name}: _`;
        symbols.push({
          name,
          kind: 'variable',
          range: rangeForLine(ln, nameStart, nameStart + name.length),
          signature: sig,
          doc: extractLeadingDoc(lines, ln),
          defLine: ln, defCol: nameStart,
        });
        continue;
      }
    }
  }

  return symbols;
}

/** Basic classifier for non-indexed tokens (fallback). */
function classifyFallback(word: string, before: string, after: string): SymbolKind | 'keyword' {
  if (KEYWORDS.has(word)) return 'keyword' as any;
  if (/^[A-Z][A-Za-z0-9_]*$/.test(word)) return 'type';
  if (/^\s*\(/.test(after) || /(^|\W)fn\s+$/.test(before)) return 'function';
  return 'variable';
}

/** Builds nice Markdown hover from symbol info (or fallback token). */
function buildMarkdown(sym: SymbolInfo | null, fallbackKind?: string): string {
  if (sym) {
    const header = `**${sym.name}** — ${sym.kind}`;
    const sig = sym.signature ? '\n\n' + fence('vitte', sym.signature) : '';
    const doc = sym.doc ? '\n\n' + sym.doc : '';
    const def = typeof sym.defLine === 'number'
      ? `\n\n_Definition: line ${sym.defLine + 1}, col ${ (sym.defCol ?? 0) + 1 }_`
      : '';
    return header + sig + doc + def;
  }
  // Fallback.
  return `**${fallbackKind ?? 'symbol'}**`;
}

/**
 * Provides hover payload for a position.
 * Returns null when nothing meaningful is found.
 */
export function provideHover(ctx: HoverContext): HoverPayload | null {
  const lines = ctx.text.split(/\r?\n/);
  if (ctx.line < 0 || ctx.line >= lines.length) return null;
  const lineText = lines[ctx.line];

  const w = wordAt(lineText, ctx.character);
  if (!w || !w.word) return null;

  // Build local range for the hovered word.
  const hoverRange: Range = rangeForLine(ctx.line, w.start, w.end);

  // Try indexed symbols first.
  const symbols = indexSymbols(ctx.text);

  // Prefer exact name matches on the same line first, then global.
  let match: SymbolInfo | null = null;
  for (const s of symbols) {
    if (s.name === w.word && s.defLine === ctx.line) { match = s; break; }
  }
  if (!match) {
    for (const s of symbols) {
      if (s.name === w.word) { match = s; break; }
    }
  }

  if (match) {
    return {
      contents: { kind: 'markdown', value: buildMarkdown(match) },
      range: hoverRange,
    };
  }

  // Fallback classification (keyword/type/function/variable).
  const before = lineText.slice(0, w.start);
  const after = lineText.slice(w.end);
  const kind = classifyFallback(w.word, before, after);
  const md = `**${w.word}** — ${kind}` + (kind === 'function'
    ? '\n\n' + fence('vitte', `fn ${w.word}(…)`)
    : '');
  return { contents: { kind: 'markdown', value: md }, range: hoverRange };
}
