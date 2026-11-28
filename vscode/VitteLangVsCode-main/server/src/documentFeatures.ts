import { FoldingRange, FoldingRangeKind, Range, SelectionRange } from "vscode-languageserver/node";
import type { Position } from "vscode-languageserver/node";
import type { TextDocument } from "vscode-languageserver-textdocument";
import { scanLexical } from "./lexical.js";
import { sourceIndex } from "./sourceSymbols.js";

export function provideFoldingRanges(doc: TextDocument): FoldingRange[] {
  const text = doc.getText();
  const { mask, spans } = scanLexical(text);
  const lineStarts = [0];
  for (let index = 0; index < text.length; index++) if (text[index] === "\n") lineStarts.push(index + 1);
  const lineAt = (offset: number): number => {
    let low = 0, high = lineStarts.length;
    while (low < high) {
      const middle = (low + high) >>> 1;
      if ((lineStarts[middle] ?? 0) <= offset) low = middle + 1;
      else high = middle;
    }
    return Math.max(0, low - 1);
  };
  const ranges: FoldingRange[] = [];
  const add = (start: number, end: number, kind?: FoldingRangeKind): void => {
    const startLine = lineAt(start);
    const endLine = lineAt(Math.max(start, end - 1));
    if (endLine > startLine) ranges.push(FoldingRange.create(startLine, endLine, undefined, undefined, kind));
  };

  const stack: number[] = [];
  for (let index = 0; index < text.length; index++) {
    if (!mask[index]) continue;
    if (text[index] === "{") stack.push(index);
    else if (text[index] === "}") {
      const open = stack.pop();
      if (open !== undefined) add(open, index + 1);
    }
  }

  const commentLineRanges: { start: number; end: number }[] = [];
  for (const span of spans) {
    if (span.kind !== "comment") continue;
    const startLine = lineAt(span.start);
    const endLine = lineAt(Math.max(span.start, span.end - 1));
    if (endLine > startLine) add(span.start, span.end, FoldingRangeKind.Comment);
    else if (text.slice(span.start, span.end).startsWith("//")) commentLineRanges.push({ start: startLine, end: endLine });
  }
  commentLineRanges.sort((a, b) => a.start - b.start);
  for (let index = 0; index < commentLineRanges.length;) {
    const first = commentLineRanges[index];
    if (!first) { index++; continue; }
    let end = first.end;
    let next = index + 1;
    while (next < commentLineRanges.length) {
      const nextRange = commentLineRanges[next];
      if (!nextRange || nextRange.start > end + 1) break;
      end = nextRange.end;
      next++;
    }
    if (end > first.start) ranges.push(FoldingRange.create(first.start, end, undefined, undefined, FoldingRangeKind.Comment));
    index = next;
  }
  const unique = new Map(ranges.map((range) => [`${range.startLine}:${range.endLine}:${range.kind ?? ""}`, range]));
  return [...unique.values()].sort((a, b) => a.startLine - b.startLine || b.endLine - a.endLine);
}

export function provideSelectionRanges(doc: TextDocument, positions: readonly Position[]): SelectionRange[] {
  const text = doc.getText();
  const lexical = scanLexical(text);
  const symbols = sourceIndex(doc).symbols;
  const braceScopes: { start: number; end: number }[] = [];
  const stack: number[] = [];
  for (let index = 0; index < text.length; index++) {
    if (!lexical.mask[index]) continue;
    if (text[index] === "{") stack.push(index);
    else if (text[index] === "}") {
      const open = stack.pop();
      if (open !== undefined) braceScopes.push({ start: open, end: index + 1 });
    }
  }
  return positions.map((position) => {
    const offset = doc.offsetAt(position);
    const candidates: { start: number; end: number }[] = [];
    const tokenStart = offset < text.length && /[\p{L}\p{N}_]/u.test(text[offset] ?? "") ? offset
      : offset > 0 && /[\p{L}\p{N}_]/u.test(text[offset - 1] ?? "") ? offset - 1 : -1;
    if (tokenStart >= 0 && lexical.mask[tokenStart]) {
      let start = tokenStart, end = tokenStart + 1;
      while (start > 0 && /[\p{L}\p{N}_]/u.test(text[start - 1] ?? "")) start--;
      while (end < text.length && /[\p{L}\p{N}_]/u.test(text[end] ?? "")) end++;
      candidates.push({ start, end });
    }
    for (const span of lexical.spans) if (span.start <= offset && offset <= span.end) candidates.push({ start: span.start, end: span.end });
    for (const symbol of symbols) {
      const declaration = { start: doc.offsetAt(symbol.range.start), end: doc.offsetAt(symbol.range.end) };
      if (declaration.start <= offset && offset <= declaration.end) candidates.push(declaration);
      if (symbol.bodyStart !== undefined && symbol.bodyEnd !== undefined) {
        const body = { start: symbol.bodyStart, end: symbol.bodyEnd + 1 };
        if (body.start <= offset && offset <= body.end) candidates.push(body);
      }
    }
    for (const scope of braceScopes) if (scope.start <= offset && offset <= scope.end) candidates.push(scope);
    const line = doc.getText({ start: { line: position.line, character: 0 }, end: { line: position.line + 1, character: 0 } });
    const lineStart = doc.offsetAt({ line: position.line, character: 0 });
    candidates.push({ start: lineStart, end: Math.min(text.length, lineStart + line.length) });
    candidates.push({ start: 0, end: text.length });

    const valid = [...new Map(candidates
      .filter((range) => range.start <= offset && offset <= range.end && range.end > range.start)
      .map((range) => [`${range.start}:${range.end}`, range])).values()]
      .sort((a, b) => (b.end - b.start) - (a.end - a.start) || a.start - b.start);
    let chain: SelectionRange | undefined;
    let parentOffsets: { start: number; end: number } | undefined;
    for (const candidate of valid) {
      if (parentOffsets && !(parentOffsets.start <= candidate.start && candidate.end <= parentOffsets.end)) continue;
      const range = Range.create(doc.positionAt(candidate.start), doc.positionAt(candidate.end));
      chain = SelectionRange.create(range, chain);
      parentOffsets = candidate;
    }
    return chain ?? SelectionRange.create(Range.create(position, position));
  });
}
