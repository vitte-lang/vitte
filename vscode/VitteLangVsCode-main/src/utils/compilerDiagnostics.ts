import * as path from 'node:path';
import { fileURLToPath } from 'node:url';

export interface CompilerSpan {
  source?: string | undefined;
  start: number;
  end: number;
}

export interface CompilerLabel extends CompilerSpan {
  style?: string | undefined;
  offsetUnit?: "utf8-byte" | "utf16" | undefined;
  lineRange?: { startLine: number; startCharacter: number; endLine: number; endCharacter: number } | undefined;
  message: string;
}

export interface CompilerRelatedLocation extends CompilerSpan {
  message: string;
  offsetUnit?: "utf8-byte" | "utf16" | undefined;
  lineRange?: { startLine: number; startCharacter: number; endLine: number; endCharacter: number } | undefined;
}

export interface CompilerSuggestion extends CompilerSpan {
  message: string;
  replacement?: string | undefined;
  applicability: string;
  kind?: string | undefined;
  candidate?: string | undefined;
  destination?: string | undefined;
  candidateCount?: number | undefined;
  destinationCount?: number | undefined;
}

export interface CompilerDiagnostic extends CompilerSpan {
  severity?: string | number | undefined;
  code?: string | number | undefined;
  message?: string | undefined;
  title?: string | undefined;
  help?: string | undefined;
  labels: CompilerLabel[];
  related: CompilerRelatedLocation[];
  suggestions: CompilerSuggestion[];
  expected?: string | undefined;
  found?: string | undefined;
  expectedType?: string | undefined;
  actualType?: string | undefined;
  phase?: string | undefined;
  category?: string | undefined;
  origin?: string | undefined;
}

type JsonRecord = Record<string, unknown>;

function record(value: unknown): JsonRecord {
  return value && typeof value === 'object' && !Array.isArray(value) ? value as JsonRecord : {};
}

function string(value: unknown): string | undefined {
  return typeof value === 'string' && value.trim().length > 0 ? value : undefined;
}

function number(value: unknown): number | undefined {
  return typeof value === 'number' && Number.isFinite(value) ? value : undefined;
}

function uriToPath(source: string, cwd: string): string {
  if (source.startsWith('file://')) {
    try { return path.resolve(fileURLToPath(source)); } catch { /* keep the original path */ }
  }
  return path.resolve(cwd, source);
}

function utf16Offset(bytes: Buffer, byte: number): number {
  return bytes.subarray(0, Math.max(0, Math.min(byte, bytes.length))).toString('utf8').length;
}

function offsetAt(text: string, line: number, character: number): number {
  if (line < 0 || character < 0) return 0;
  let offset = 0;
  for (let current = 0; current < line; current += 1) {
    const next = text.indexOf('\n', offset);
    if (next < 0) return text.length;
    offset = next + 1;
  }
  const lineEnd = text.indexOf('\n', offset);
  return Math.min(lineEnd < 0 ? text.length : lineEnd, offset + character);
}

function spanOffsets(raw: unknown, text: string, bytes: Buffer, defaultSource?: string): CompilerSpan | undefined {
  const value = record(raw);
  const source = string(value.source) ?? string(value.uri) ?? defaultSource;
  const startByte = number(value.start_byte) ?? number(value.startByte);
  const endByte = number(value.end_byte) ?? number(value.endByte);
  if (startByte !== undefined && endByte !== undefined) {
    return { source, start: utf16Offset(bytes, startByte), end: utf16Offset(bytes, endByte) };
  }
  const startOffset = number(value.start);
  const endOffset = number(value.end);
  if (startOffset !== undefined && endOffset !== undefined) return { source, start: startOffset, end: endOffset };

  const range = record(value.range);
  const lspStart = record(range.start);
  const lspEnd = record(range.end);
  if (number(lspStart.line) !== undefined && number(lspStart.character) !== undefined
      && number(lspEnd.line) !== undefined && number(lspEnd.character) !== undefined) {
    return {
      source,
      start: offsetAt(text, number(lspStart.line)!, number(lspStart.character)!),
      end: offsetAt(text, number(lspEnd.line)!, number(lspEnd.character)!),
    };
  }

  const start = record(value.start);
  const end = record(value.end);
  const lineStart = number(value.start_line) ?? number(value.startLine) ?? number(start.line);
  const colStart = number(value.start_column) ?? number(value.startColumn) ?? number(start.column);
  const lineEnd = number(value.end_line) ?? number(value.endLine) ?? number(end.line);
  const colEnd = number(value.end_column) ?? number(value.endColumn) ?? number(end.column);
  if (lineStart !== undefined && colStart !== undefined && lineEnd !== undefined && colEnd !== undefined) {
    // Compiler JSON coordinates are zero-based; legacy `location` coordinates are one-based.
    const legacy = value.start !== undefined && value.start_byte === undefined && value.startByte === undefined;
    const shift = legacy ? 1 : 0;
    return {
      source,
      start: offsetAt(text, Math.max(0, lineStart - shift), Math.max(0, colStart - shift)),
      end: offsetAt(text, Math.max(0, lineEnd - shift), Math.max(0, colEnd - shift)),
    };
  }
  return undefined;
}

function sourceMatches(source: string | undefined, file: string, cwd: string): boolean {
  if (!source) return true;
  return uriToPath(source, cwd) === path.resolve(file);
}

function helpFrom(item: JsonRecord, data: JsonRecord): string | undefined {
  const direct = string(item.help) ?? string(data.help);
  if (direct) return direct;
  const annotations = Array.isArray(item.annotations) ? item.annotations : Array.isArray(data.annotations) ? data.annotations : [];
  const help = annotations
    .map(record)
    .filter(annotation => string(annotation.kind)?.toLowerCase() === 'help')
    .map(annotation => string(annotation.message))
    .filter((value): value is string => value !== undefined);
  return help.length > 0 ? Array.from(new Set(help)).join('\n') : undefined;
}

function labelsFrom(item: JsonRecord, data: JsonRecord, text: string, bytes: Buffer, fallbackSource: string, cwd: string): CompilerLabel[] {
  const rawLabels = Array.isArray(item.labels) ? item.labels : Array.isArray(data.labels) ? data.labels : [];
  return rawLabels.flatMap((raw): CompilerLabel[] => {
    const label = record(raw);
    const rawSpan = record(label.span ?? label);
    const source = string(rawSpan.source) ?? string(rawSpan.uri) ?? fallbackSource;
    const sameFile = sourceMatches(source, fallbackSource, cwd);
    const byteStart = number(rawSpan.start_byte) ?? number(rawSpan.startByte);
    const byteEnd = number(rawSpan.end_byte) ?? number(rawSpan.endByte);
    const span = !sameFile && byteStart !== undefined && byteEnd !== undefined
      ? { source, start: byteStart, end: byteEnd }
      : spanOffsets(rawSpan, text, bytes, source);
    const message = string(label.message);
    const range = record(rawSpan.range);
    const rangeStart = record(range.start);
    const rangeEnd = record(range.end);
    const lineRange = number(rangeStart.line) !== undefined && number(rangeStart.character) !== undefined
        && number(rangeEnd.line) !== undefined && number(rangeEnd.character) !== undefined
      ? {
          startLine: number(rangeStart.line)!,
          startCharacter: number(rangeStart.character)!,
          endLine: number(rangeEnd.line)!,
          endCharacter: number(rangeEnd.character)!,
        }
      : undefined;
    return span && message ? [{
      ...span,
      style: string(label.style),
      message,
      offsetUnit: sameFile ? undefined : byteStart !== undefined && byteEnd !== undefined ? "utf8-byte" : "utf16",
      lineRange,
    }] : [];
  });
}

function relatedFrom(item: JsonRecord, data: JsonRecord, text: string, bytes: Buffer, fallbackSource: string, cwd: string): CompilerRelatedLocation[] {
  const result: CompilerRelatedLocation[] = [];
  const rawRelated = Array.isArray(item.relatedInformation)
    ? item.relatedInformation
    : Array.isArray(item.related_locations)
      ? item.related_locations
      : Array.isArray(data.related_locations) ? data.related_locations : [];
  for (const raw of rawRelated) {
    const related = record(raw);
    const location = record(related.location);
    const lspRange = record(location.range);
    const lspStart = record(lspRange.start);
    const lspEnd = record(lspRange.end);
    const lspSource = string(location.uri) ?? string(related.source);
    const rawSpan = record(location.range ? { ...location, range: location.range } : related.span ?? related);
    const source = lspSource ?? string(rawSpan.source) ?? fallbackSource;
    const sameFile = sourceMatches(source, fallbackSource, cwd);
    const byteStart = number(rawSpan.start_byte) ?? number(rawSpan.startByte);
    const byteEnd = number(rawSpan.end_byte) ?? number(rawSpan.endByte);
    const span = !sameFile && byteStart !== undefined && byteEnd !== undefined
      ? { source, start: byteStart, end: byteEnd }
      : spanOffsets(rawSpan, text, bytes, source);
    const message = string(related.message) ?? string(related.label);
    if (message && number(lspStart.line) !== undefined && number(lspStart.character) !== undefined
        && number(lspEnd.line) !== undefined && number(lspEnd.character) !== undefined) {
      result.push({
        source: lspSource ?? fallbackSource,
        start: span?.start ?? 0,
        end: span?.end ?? 0,
        message,
        lineRange: {
          startLine: number(lspStart.line)!,
          startCharacter: number(lspStart.character)!,
          endLine: number(lspEnd.line)!,
          endCharacter: number(lspEnd.character)!,
        },
      });
    } else if (span && message) {
      result.push({
        ...span,
        message,
        offsetUnit: sameFile ? undefined : byteStart !== undefined && byteEnd !== undefined ? "utf8-byte" : "utf16",
      });
    }
  }

  // Secondary compiler labels carry the same useful source context when an LSP
  // server did not also produce standard `relatedInformation` entries.
  if (rawRelated.length === 0) {
    result.push(...labelsFrom(item, data, text, bytes, fallbackSource, cwd)
      .filter(label => label.style !== 'primary')
      .map(({ style: _style, ...label }) => label));
  }
  return result;
}

function suggestionsFrom(item: JsonRecord, data: JsonRecord, text: string, bytes: Buffer, fallbackSource: string): CompilerSuggestion[] {
  const rawSuggestions = Array.isArray(item.suggestions) ? item.suggestions : Array.isArray(data.suggestions) ? data.suggestions : [];
  return rawSuggestions.flatMap((raw): CompilerSuggestion[] => {
    const suggestion = record(raw);
    const span = spanOffsets(suggestion.span ?? suggestion, text, bytes,
      string(suggestion.source) ?? fallbackSource);
    const applicability = string(suggestion.applicability)?.toLowerCase();
    if (!span || !applicability) return [];
    const candidateRecord = record(suggestion.candidate);
    const destinationRecord = record(suggestion.destination);
    return [{
      ...span,
      message: string(suggestion.message) ?? 'Apply compiler suggestion',
      replacement: typeof suggestion.replacement === 'string' ? suggestion.replacement : undefined,
      applicability,
      kind: string(suggestion.kind) ?? string(suggestion.action),
      candidate: string(suggestion.candidate) ?? string(candidateRecord.name),
      destination: string(suggestion.destination) ?? string(destinationRecord.path) ?? string(destinationRecord.module),
      candidateCount: number(suggestion.candidateCount) ?? number(suggestion.candidate_count),
      destinationCount: number(suggestion.destinationCount) ?? number(suggestion.destination_count),
    }];
  });
}

/** Native diagnostic spans are UTF-8 byte offsets; VS Code uses UTF-16 offsets. */
export function normalizeCompilerDiagnostics(payload: unknown, text: string, file: string, cwd: string): CompilerDiagnostic[] {
  const envelope = record(payload);
  const diagnostics = Array.isArray(payload) ? payload : envelope.diagnostics;
  if (!Array.isArray(diagnostics)) return [];
  const bytes = Buffer.from(text, 'utf8');
  const fallbackSource = file;
  const result: CompilerDiagnostic[] = [];

  for (const raw of diagnostics) {
    const item = record(raw);
    const data = record(item.data);
    const position = record(data.position ?? item.position ?? item.location);
    const source = string(position.source) ?? string(position.uri) ?? string(item.sourcePath);
    if (!sourceMatches(source, file, cwd)) continue;

    const hasByteSpan = (number(position.start_byte) ?? number(position.startByte)) !== undefined
      && (number(position.end_byte) ?? number(position.endByte)) !== undefined;
    let explicitSpan = spanOffsets(position, text, bytes, fallbackSource)
      ?? spanOffsets(item, text, bytes, fallbackSource);
    const sourceLines = text.split(/\r\n|\r|\n/);
    const sourceLineCount = sourceLines.length;
    const startLine = number(position.startLine) ?? number(position.start_line);
    const endLine = number(position.endLine) ?? number(position.end_line);
    const startColumn = number(position.startColumn) ?? number(position.start_column);
    const endColumn = number(position.endColumn) ?? number(position.end_column);
    // Some CLI builds serialize an out-of-range, one-based line while the
    // byte offsets are also one byte late. Recover from the UTF-16 columns
    // only when that reported line cannot exist in this source document.
    const startCoordinateOutsideSource = startLine !== undefined && startColumn !== undefined
      && (startLine >= sourceLineCount || startColumn > (sourceLines[startLine]?.length ?? -1));
    const endCoordinateOutsideSource = endLine !== undefined && endColumn !== undefined
      && (endLine >= sourceLineCount || endColumn > (sourceLines[endLine]?.length ?? -1));
    if (hasByteSpan && startLine !== undefined && endLine !== undefined
        && startColumn !== undefined && endColumn !== undefined
        && startCoordinateOutsideSource && endCoordinateOutsideSource) {
      explicitSpan = {
        source: string(position.source) ?? fallbackSource,
        start: offsetAt(text, startLine - 1, startColumn),
        end: offsetAt(text, endLine - 1, endColumn),
      };
    }
    const lspSpan = !hasByteSpan && item.range
      ? spanOffsets({ source: source ?? fallbackSource, range: item.range }, text, bytes, fallbackSource)
      : undefined;
    const span = hasByteSpan ? explicitSpan : lspSpan ?? explicitSpan;
    if (!span) continue;
    const start = Math.max(0, Math.min(span.start, text.length));
    const end = Math.max(start, Math.min(span.end, text.length));
    const reportedByteStart = number(position.start_byte) ?? number(position.startByte);
    const byteDelta = lspSpan && reportedByteStart !== undefined
      ? lspSpan.start - utf16Offset(bytes, reportedByteStart)
      : 0;
    const shiftSameFile = <T extends CompilerSpan>(entries: T[]): T[] => entries.map(entry => {
      if (!byteDelta || !sourceMatches(entry.source, file, cwd)) return entry;
      return { ...entry, start: entry.start + byteDelta, end: entry.end + byteDelta };
    });
    const labels = shiftSameFile(labelsFrom(item, data, text, bytes, source ?? fallbackSource, cwd));
    result.push({
      start,
      end,
      source: span.source,
      severity: typeof data.severity === 'string' || typeof data.severity === 'number'
        ? data.severity
        : typeof item.severity === 'string' || typeof item.severity === 'number' ? item.severity : undefined,
      code: typeof item.code === 'string' || typeof item.code === 'number' ? item.code : undefined,
      message: string(item.message) ?? string(item.title),
      title: string(item.title) ?? string(item.message),
      help: helpFrom(item, data),
      labels: labels.map(label => ({ ...label, source: label.source && sourceMatches(label.source, file, cwd) ? file : label.source })),
      related: shiftSameFile(relatedFrom(item, data, text, bytes, source ?? fallbackSource, cwd)),
      suggestions: shiftSameFile(suggestionsFrom(item, data, text, bytes, source ?? fallbackSource))
        .map(suggestion => ({ ...suggestion, source: suggestion.source && sourceMatches(suggestion.source, file, cwd) ? file : suggestion.source })),
      expected: string(data.expected) ?? string(record(item.context).expected),
      found: string(data.found) ?? string(record(item.context).found),
      expectedType: string(data.expectedType) ?? string(data.expected_type),
      actualType: string(data.actualType) ?? string(data.actual_type),
      phase: string(data.phase) ?? string(item.phase),
      category: string(data.category) ?? string(item.category),
      origin: string(data.origin) ?? string(item.origin),
    });
  }
  return result;
}

function canonicalDiagnosticCode(code: string | number | undefined): string {
  const value = code === undefined ? '' : String(code).trim().toUpperCase();
  return value.includes(':') ? value.slice(value.lastIndexOf(':') + 1) : value;
}

interface SuggestionEntry { start: number; end: number; code: string; suggestions: CompilerSuggestion[]; }
const suggestionsByDocument = new Map<string, SuggestionEntry[]>();

/** Retain structured fixes beside VS Code diagnostics (the VS Code API drops LSP `data`). */
export function storeCompilerSuggestions(uri: string, diagnostics: CompilerDiagnostic[]): void {
  const entries = diagnostics
    .filter(diagnostic => diagnostic.suggestions.length > 0 && diagnostic.code !== undefined)
    .map(diagnostic => ({
      start: diagnostic.start,
      end: diagnostic.end,
      code: canonicalDiagnosticCode(diagnostic.code),
      suggestions: diagnostic.suggestions,
    }));
  if (entries.length === 0) suggestionsByDocument.delete(uri);
  else suggestionsByDocument.set(uri, entries);
}

export function clearCompilerSuggestions(uri: string): void {
  suggestionsByDocument.delete(uri);
}

export function compilerSuggestionsFor(uri: string, code: string | number | undefined, start: number, end: number): CompilerSuggestion[] {
  const canonical = canonicalDiagnosticCode(code);
  return suggestionsByDocument.get(uri)?.find(item => item.code === canonical && item.start === start && item.end === end)?.suggestions ?? [];
}
