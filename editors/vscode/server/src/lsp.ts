/**
 * Vitte LSP — Main Server (synchronized)
 * --------------------------------------
 * Unified LSP server aligned with the updated config/logger/utils/languageService modules.
 */

import {
  createConnection,
  ProposedFeatures,
  TextDocuments,
  TextDocumentSyncKind,
  DidChangeConfigurationNotification,
  DidChangeWatchedFilesNotification,
  FileChangeType,
  CompletionItemKind,
  MarkupKind,
  Location,
} from 'vscode-languageserver/node';
import type {
  InitializeParams,
  InitializeResult,
  CompletionItem,
  TextDocumentPositionParams,
  Hover,
  Range,
} from 'vscode-languageserver/node';

import { TextDocument } from 'vscode-languageserver-textdocument';

import Config, { defaultFormatting } from './config';
import type { FormattingSettings } from './config';
import { logLsp, attachConnection } from './logger';
import { provideCompletions, resolveCompletion, triggerCharacters } from './completion';
import { getSemanticTokensLegend, buildSemanticTokens, provideHover as keywordHover } from './semantic';
import { lintToPublishable } from './lint';
import { definitionAtPosition, referencesAtPosition, documentSymbols } from './navigation';
import { indexDocument, updateDocument, removeDocument, searchWorkspaceSymbols, toWorkspaceSymbols, getDocumentIndex, getIndex, indexText } from './indexer';
import { getDocstringAtLine } from './docstrings';
import { provideFormattingEdits } from './formatting';
import type { ExtraFormattingOptions } from './formatting';
import fs from 'node:fs/promises';
import path from 'node:path';
import { fileURLToPath, pathToFileURL } from 'node:url';

const fileCache = new Map<string, { mtimeMs: number; size: number; text: string }>();
import {
  normalizeIndentation,
  computeMinimalSmartEdits,
  expandSelectionToEnclosingBrackets,
} from './utils/text';

// ---------------------------------------------------------------------------
// Connection & documents
// ---------------------------------------------------------------------------

export const connection = createConnection(ProposedFeatures.all);
attachConnection(connection);
logLsp.info('Vitte LSP: connection created');

const documents = new TextDocuments<TextDocument>(TextDocument);

let hasConfigurationCapability = false;
let hasWorkspaceFolderCapability = false;
const semanticLegend = getSemanticTokensLegend();

interface FormatDocumentParams {
  textDocument?: { uri?: string };
}

interface FormatRangeParams extends FormatDocumentParams {
  range?: Range;
}

interface ExpandSelectionParams extends FormatDocumentParams {
  position?: { line: number; character: number };
}

function wordAt(doc: TextDocument, pos: { line: number; character: number }): string | null {
  const text = doc.getText();
  const off = doc.offsetAt(pos);
  let s = off;
  let e = off;
  while (s > 0 && /[A-Za-z0-9_]/.test(text.charAt(s - 1))) s--;
  while (e < text.length && /[A-Za-z0-9_]/.test(text.charAt(e))) e++;
  return e > s ? text.slice(s, e) : null;
}

function toExtraFormatting(opts: FormattingSettings): ExtraFormattingOptions {
  const normalizeEOL = opts.eol === 'auto' ? undefined : opts.eol;
  return {
    tabSize: opts.tabSize,
    insertSpaces: opts.insertSpaces,
    trimTrailingWhitespace: opts.trimTrailingWhitespace,
    insertFinalNewline: opts.insertFinalNewline,
    normalizeEOL,
  };
}

function isIdentChar(ch: string): boolean {
  return /[A-Za-z0-9_]/.test(ch);
}

function buildCodeMask(text: string): Uint8Array {
  const n = text.length;
  const mask = new Uint8Array(n);
  let i = 0;
  const mark = (from: number, to: number) => { for (let k = from; k < to; k++) mask[k] = 1; };
  while (i < n) {
    const c = text.charCodeAt(i);
    const c2 = i + 1 < n ? text.charCodeAt(i + 1) : 0;

    if (c === 0x23 /* # */ && c2 !== 0x5b /* [ */) {
      i += 1;
      while (i < n && text.charCodeAt(i) !== 0x0a) i++;
      continue;
    }
    if (c === 0x2f && c2 === 0x2f) {
      i += 2;
      while (i < n && text.charCodeAt(i) !== 0x0a) i++;
      continue;
    }
    if (c === 0x2f && c2 === 0x2a) {
      i += 2;
      while (i < n) {
        if (text.charCodeAt(i) === 0x2a && i + 1 < n && text.charCodeAt(i + 1) === 0x2f) { i += 2; break; }
        i++;
      }
      continue;
    }
    if (c === 0x72 /* r */) {
      let j = i + 1;
      let hashes = 0;
      while (j < n && text.charCodeAt(j) === 0x23) { hashes++; j++; }
      if (j < n && text.charCodeAt(j) === 0x22) {
        j++;
        for (; j < n; j++) {
          if (text.charCodeAt(j) === 0x22) {
            let k = j + 1;
            let ok = true;
            for (let h = 0; h < hashes; h++) {
              if (k >= n || text.charCodeAt(k) !== 0x23) { ok = false; break; }
              k++;
            }
            if (ok) { j = k; break; }
          }
        }
        i = j;
        continue;
      }
    }
    if (c === 0x22 || c === 0x27) {
      const quote = c;
      i++;
      while (i < n) {
        if (text.charCodeAt(i) === 0x5c) { i += 2; continue; }
        if (text.charCodeAt(i) === quote) { i++; break; }
        i++;
      }
      continue;
    }
    const start = i;
    while (i < n) {
      const a = text.charCodeAt(i);
      const b = i + 1 < n ? text.charCodeAt(i + 1) : 0;
      if (
        (a === 0x2f && (b === 0x2f || b === 0x2a)) ||
        a === 0x22 ||
        a === 0x27 ||
        a === 0x72 ||
        (a === 0x23 && b !== 0x5b)
      ) break;
      i++;
    }
    mark(start, i);
  }
  return mask;
}

function buildLineIndex(text: string): number[] {
  const idx: number[] = [0];
  for (let i = 0; i < text.length; i++) if (text.charCodeAt(i) === 10) idx.push(i + 1);
  return idx;
}

function offsetToPos(nlIdx: number[], off: number): { line: number; character: number } {
  let lo = 0;
  let hi = nlIdx.length - 1;
  while (lo <= hi) {
    const mid = (lo + hi) >> 1;
    const v = nlIdx[mid];
    if (v === off) return { line: mid, character: 0 };
    if (v < off) lo = mid + 1; else hi = mid - 1;
  }
  const line = Math.max(0, lo - 1);
  return { line, character: off - nlIdx[line] };
}

function findWordLocationsInText(text: string, uri: string, word: string): Location[] {
  if (!word) return [];
  const mask = buildCodeMask(text);
  const nlIdx = buildLineIndex(text);
  const out: Location[] = [];
  let idx = text.indexOf(word);
  while (idx !== -1) {
    const before = idx === 0 ? "" : text.charAt(idx - 1);
    const after = idx + word.length >= text.length ? "" : text.charAt(idx + word.length);
    if (!isIdentChar(before) && !isIdentChar(after) && mask[idx]) {
      const start = offsetToPos(nlIdx, idx);
      const end = offsetToPos(nlIdx, idx + word.length);
      out.push(Location.create(uri, { start, end }));
    }
    idx = text.indexOf(word, idx + word.length);
  }
  return out;
}

async function readFileCached(filePath: string): Promise<string | null> {
  try {
    const stat = await fs.stat(filePath);
    const cached = fileCache.get(filePath);
    if (cached && cached.mtimeMs === stat.mtimeMs && cached.size === stat.size) {
      return cached.text;
    }
    const text = await fs.readFile(filePath, "utf8");
    fileCache.set(filePath, { mtimeMs: stat.mtimeMs, size: stat.size, text });
    return text;
  } catch {
    return null;
  }
}

interface IndexerSettings {
  exclude: string[];
  maxFiles: number;
  maxFileSizeKB: number;
}

const defaultIndexerSettings: IndexerSettings = {
  exclude: ["**/node_modules/**", "**/.git/**", "**/target/**", "**/build/**", "**/dist/**", "**/out/**", "**/.vscode/**"],
  maxFiles: 3000,
  maxFileSizeKB: 1000,
};

function compileExcludeMatchers(patterns: string[]): ((p: string) => boolean)[] {
  return (patterns ?? []).map((p) => {
    const esc = p.replace(/[.+^${}()|[\]\\]/g, "\\$&");
    const rx = new RegExp("^" + esc.replace(/\*/g, ".*").replace(/\?/g, ".") + "$");
    return (pathStr: string) => rx.test(pathStr);
  });
}

function isExcluded(pathStr: string, matchers: ((p: string) => boolean)[], roots: string[]): boolean {
  const norm = pathStr.replace(/\\/g, "/");
  for (const root of roots) {
    const rel = norm.startsWith(root) ? norm.slice(root.length) : norm;
    const relNorm = rel.startsWith("/") ? rel.slice(1) : rel;
    for (const m of matchers) {
      if (m(relNorm) || m(norm)) return true;
    }
  }
  return false;
}

async function getIndexerSettings(): Promise<IndexerSettings> {
  if (!hasConfigurationCapability) return { ...defaultIndexerSettings };
  try {
    const raw = await connection.workspace.getConfiguration({ section: 'vitte.indexer' }) as Partial<IndexerSettings> | null | undefined;
    const out: IndexerSettings = { ...defaultIndexerSettings, ...(raw ?? {}) };
    if (!Array.isArray(out.exclude)) out.exclude = defaultIndexerSettings.exclude;
    out.maxFiles = Math.max(100, Math.min(20000, Number(out.maxFiles) || defaultIndexerSettings.maxFiles));
    out.maxFileSizeKB = Math.max(64, Math.min(10000, Number(out.maxFileSizeKB) || defaultIndexerSettings.maxFileSizeKB));
    return out;
  } catch {
    return { ...defaultIndexerSettings };
  }
}

async function indexWorkspaceFolders(): Promise<void> {
  const folders = await connection.workspace.getWorkspaceFolders();
  if (!folders || folders.length === 0) return;

  const exts = new Set([".vit", ".vitte"]);
  const settings = await getIndexerSettings();
  const maxFiles = settings.maxFiles;
  const maxSize = settings.maxFileSizeKB * 1024;

  let indexed = 0;
  const stack: string[] = [];
  const roots: string[] = [];
  for (const f of folders) {
    if (f.uri.startsWith("file://")) {
      const root = fileURLToPath(f.uri);
      roots.push(root.replace(/\\/g, "/"));
      stack.push(root);
    }
  }
  const excludeMatchers = compileExcludeMatchers(settings.exclude);

  while (stack.length > 0 && indexed < maxFiles) {
    const dir = stack.pop()!;
    if (isExcluded(dir, excludeMatchers, roots)) continue;
    let entries;
    try {
      entries = await fs.readdir(dir, { withFileTypes: true });
    } catch {
      continue;
    }
    for (const ent of entries) {
      if (indexed >= maxFiles) break;
      const full = path.join(dir, ent.name);
      if (ent.isDirectory()) {
        if (!ent.name.startsWith(".") && !isExcluded(full, excludeMatchers, roots)) {
          stack.push(full);
        }
        continue;
      }
      if (!ent.isFile()) continue;
      const ext = path.extname(ent.name).toLowerCase();
      if (!exts.has(ext)) continue;
      const text = await readFileCached(full);
      if (!text) continue;
      if (text.length > maxSize) continue;
      const uri = pathToFileURL(full).toString();
      indexText(uri, text);
      indexed++;
    }
  }

  logLsp.info("Workspace index built", { files: indexed });
}

// ---------------------------------------------------------------------------
// Initialize / Initialized
// ---------------------------------------------------------------------------

connection.onInitialize((params: InitializeParams): InitializeResult => {
  const caps = Config.initConfigFromInitialize(params);
  hasConfigurationCapability = caps.hasConfigurationCapability;
  hasWorkspaceFolderCapability = caps.hasWorkspaceFolderCapability;

  logLsp.info('Vitte LSP: initializing', caps);

  const result: InitializeResult = {
    capabilities: {
      textDocumentSync: TextDocumentSyncKind.Incremental,
      // Completion via completion.ts (with resolve)
      completionProvider: { resolveProvider: true, triggerCharacters: triggerCharacters() },
      hoverProvider: true,
      definitionProvider: true,
      referencesProvider: true,
      documentSymbolProvider: true,
      workspaceSymbolProvider: true,
      semanticTokensProvider: { legend: semanticLegend, full: true, range: false },
      // Formatting endpoints are exposed via custom requests below
      documentFormattingProvider: true,
      documentRangeFormattingProvider: true,
    },
  };

  if (hasWorkspaceFolderCapability) {
    result.capabilities.workspace = {
      workspaceFolders: { supported: true, changeNotifications: true },
    };
  }

  return result;
});

connection.onInitialized(() => {
  logLsp.info('Vitte LSP: initialized');
  if (hasConfigurationCapability) {
    void connection.client.register(DidChangeConfigurationNotification.type, undefined);
    logLsp.debug('Registered DidChangeConfiguration');
    void connection.client.register(DidChangeWatchedFilesNotification.type, {
      watchers: [
        { globPattern: '**/*.vit' },
        { globPattern: '**/*.vitte' },
      ],
    });
  }
  void indexWorkspaceFolders();
});

// ---------------------------------------------------------------------------
// Configuration changes → clear settings & revalidate
// ---------------------------------------------------------------------------

const handleConfigChange = Config.makeOnDidChangeConfigurationHandler<TextDocument>(connection, {
  getOpenDocuments: () => documents.all(),
  validateDocument: async (doc) => {
    try {
      const diagnostics = lintToPublishable(doc.getText(), doc.uri);
      void connection.sendDiagnostics({ uri: doc.uri, diagnostics });
    } catch (e) {
      logLsp.warn('Validation error after config change', { uri: doc.uri, error: String(e) });
    }
  },
});

connection.onDidChangeConfiguration(() => {
  void handleConfigChange();
});

connection.onDidChangeWatchedFiles(async (e) => {
  for (const change of e.changes) {
    const uri = change.uri;
    if (!uri.startsWith("file://")) continue;
    const filePath = fileURLToPath(uri);
    if (change.type === FileChangeType.Deleted) {
      removeDocument(uri);
      fileCache.delete(filePath);
      continue;
    }
    if (change.type === FileChangeType.Changed || change.type === FileChangeType.Created) {
      const text = await readFileCached(filePath);
      if (text) indexText(uri, text);
    }
  }
});

// ---------------------------------------------------------------------------
// Diagnostics lifecycle
// ---------------------------------------------------------------------------

documents.onDidOpen((e) => { void handleValidate(e.document); });
documents.onDidChangeContent((e) => { void handleValidate(e.document); });
documents.onDidClose((e) => {
  // Clear diagnostics when closing
  void connection.sendDiagnostics({ uri: e.document.uri, diagnostics: [] });
  removeDocument(e.document.uri);
});

async function handleValidate(doc: TextDocument) {
  try {
    const diags = lintToPublishable(doc.getText(), doc.uri);
    void connection.sendDiagnostics({ uri: doc.uri, diagnostics: diags });
    logLsp.debug('Diagnostics sent', { uri: doc.uri, count: diags.length });
  } catch (err) {
    logLsp.error('Validation failed', { uri: doc.uri, error: String(err) });
  }
}

// Indexation pour navigation/complétions workspace
documents.onDidOpen((e) => { indexDocument(e.document); });
documents.onDidChangeContent((e) => { updateDocument(e.document); });

// ---------------------------------------------------------------------------
// Completion
// ---------------------------------------------------------------------------

connection.onCompletion((params: TextDocumentPositionParams): CompletionItem[] => {
  try {
    const doc = documents.get(params.textDocument.uri);
    if (!doc) return [];
    return provideCompletions(doc, params.position);
  } catch (err) {
    logLsp.error('Completion failed', { error: String(err) });
    return [
      { label: 'error', kind: CompletionItemKind.Text, detail: 'Error during completion' },
    ];
  }
});

connection.onCompletionResolve((item: CompletionItem): CompletionItem => {
  try { return resolveCompletion(item); } catch { return item; }
});

// ---------------------------------------------------------------------------
// Hover
// ---------------------------------------------------------------------------

connection.onHover((params): Hover | null => {
  const doc = documents.get(params.textDocument.uri);
  if (!doc) return null;

  const kw = keywordHover(doc, params.position);
  if (kw) return kw;

  const word = wordAt(doc, params.position);
  if (!word) return null;

  const defs = getDocumentIndex(doc.uri).filter(s => s.name === word);
  if (defs.length === 0) return null;

  const closest = defs.sort((a, b) =>
    Math.abs(a.line - params.position.line) - Math.abs(b.line - params.position.line)
  )[0];
  const docstring = getDocstringAtLine(doc, closest.line);
  if (!docstring) return null;

  return {
    contents: {
      kind: MarkupKind.Markdown,
      value: `**${word}**\n\n${docstring}`,
    },
  };
});

// ---------------------------------------------------------------------------
// Definition / References
// ---------------------------------------------------------------------------

connection.onDefinition((params) => {
  const doc = documents.get(params.textDocument.uri);
  if (!doc) return [];
  return definitionAtPosition(doc, params.position, params.textDocument.uri);
});

connection.onReferences(async (params) => {
  const doc = documents.get(params.textDocument.uri);
  if (!doc) return [];
  const word = wordAt(doc, params.position);
  if (!word) return [];

  const out: Location[] = [];
  const uris = new Set<string>();
  for (const uri of getIndex().keys()) uris.add(uri);
  uris.add(doc.uri);

  for (const uri of uris) {
    const open = documents.get(uri);
    if (open) {
      out.push(...findWordLocationsInText(open.getText(), uri, word));
      continue;
    }
    if (!uri.startsWith("file://")) continue;
    const text = await readFileCached(fileURLToPath(uri));
    if (text) out.push(...findWordLocationsInText(text, uri, word));
  }
  return out;
});

// ---------------------------------------------------------------------------
// Symbols
// ---------------------------------------------------------------------------

connection.onDocumentSymbol((params) => {
  const doc = documents.get(params.textDocument.uri);
  if (!doc) return [];
  return documentSymbols(doc);
});

connection.onWorkspaceSymbol((params) => {
  const q = params.query ?? '';
  const symbols = searchWorkspaceSymbols(q, 200);
  return toWorkspaceSymbols(symbols);
});

// ---------------------------------------------------------------------------
// Semantic tokens
// ---------------------------------------------------------------------------

connection.languages.semanticTokens.on((params) => {
  const doc = documents.get(params.textDocument.uri);
  if (!doc) return { data: [] };
  return buildSemanticTokens(doc);
});

// ---------------------------------------------------------------------------
// Formatting (document, range, documentOrRange)
// ---------------------------------------------------------------------------

connection.onDocumentFormatting(async (params) => {
  const uri = params.textDocument.uri;
  const doc = documents.get(uri);
  if (!doc) return [];

  let opts: FormattingSettings;
  try {
    opts = await Config.getFormattingSettings(connection, uri);
  } catch {
    opts = defaultFormatting;
  }
  return provideFormattingEdits(doc, toExtraFormatting(opts));
});

connection.onDocumentRangeFormatting(async (params) => {
  const uri = params.textDocument.uri;
  const doc = documents.get(uri);
  if (!doc) return [];
  const lspRange = params.range;
  if (!lspRange) return [];

  let opts: FormattingSettings;
  try {
    opts = await Config.getFormattingSettings(connection, uri);
  } catch {
    opts = defaultFormatting;
  }

  const start = doc.offsetAt(lspRange.start);
  const end = doc.offsetAt(lspRange.end);
  const text = doc.getText();
  const target = text.slice(start, end);
  const targetNorm = normalizeIndentation(target, opts.insertSpaces, opts.tabSize);
  const formattedTarget = formatText(targetNorm, opts, /*isFragment*/ true);
  if (formattedTarget === target) return [];
  return [{ range: lspRange, newText: formattedTarget }];
});

connection.onRequest('vitte/formatDocument', async (params: FormatDocumentParams) => {
  const uri = params?.textDocument?.uri;
  if (!uri) return [];
  const doc = documents.get(uri);
  if (!doc) return [];

  let opts: FormattingSettings;
  try {
    opts = await Config.getFormattingSettings(connection, uri);
  } catch {
    opts = defaultFormatting;
  }
  const original = doc.getText();

  // Normalize indentation quickly before running formatter logic
  const pre = normalizeIndentation(original, opts.insertSpaces, opts.tabSize);
  const formatted = formatText(pre, opts);

  // Return minimal edits
  const edits = computeMinimalSmartEdits(original, formatted);
  logLsp.info('Formatter produced edit(s)', { uri, edits: edits.length });
  return edits;
});

connection.onRequest('vitte/formatRange', async (params: FormatRangeParams) => {
  const uri = params?.textDocument?.uri;
  if (!uri) return [];
  const doc = documents.get(uri);
  if (!doc) return [];

  const lspRange = params?.range;
  if (!lspRange) return [];

  let opts: FormattingSettings;
  try {
    opts = await Config.getFormattingSettings(connection, uri);
  } catch {
    opts = defaultFormatting;
  }
  const start = doc.offsetAt(lspRange.start);
  const end = doc.offsetAt(lspRange.end);

  const text = doc.getText();
  const target = text.slice(start, end);

  const targetNorm = normalizeIndentation(target, opts.insertSpaces, opts.tabSize);
  const formattedTarget = formatText(targetNorm, opts, /*isFragment*/ true);

  if (formattedTarget === target) return [];
  return [{ range: lspRange, newText: formattedTarget }];
});

connection.onRequest('vitte/formatDocumentOrRange', async (params: FormatRangeParams) => {
  const uri = params?.textDocument?.uri;
  if (!uri) return [];
  const doc = documents.get(uri);
  if (!doc) return [];

  let opts: FormattingSettings;
  try {
    opts = await Config.getFormattingSettings(connection, uri);
  } catch {
    opts = defaultFormatting;
  }
  const lspRange = params?.range;
  const original = doc.getText();

  if (!lspRange) {
    const pre = normalizeIndentation(original, opts.insertSpaces, opts.tabSize);
    const formatted = formatText(pre, opts);
    const edits = computeMinimalSmartEdits(original, formatted);
    return edits;
  } else {
    const start = doc.offsetAt(lspRange.start);
    const end = doc.offsetAt(lspRange.end);
    const fragment = original.slice(start, end);
    const fragmentNorm = normalizeIndentation(fragment, opts.insertSpaces, opts.tabSize);
    const formattedFragment = formatText(fragmentNorm, opts, /*isFragment*/ true);
    if (formattedFragment === fragment) return [];
    return [{ range: lspRange, newText: formattedFragment }];
  }
});

// ---------------------------------------------------------------------------
// Extra: expand selection to enclosing brackets
// ---------------------------------------------------------------------------

connection.onRequest('vitte/expandSelectionToEnclosingBrackets', (params: ExpandSelectionParams) => {
  const uri = params?.textDocument?.uri;
  if (!uri) return null;
  const doc = documents.get(uri);
  if (!doc) return null;
  const text = doc.getText();
  const range = params.position ? expandSelectionToEnclosingBrackets(text, params.position) : null;
  return range ?? null;
});

// ---------------------------------------------------------------------------
// Basic formatter (whitespace + EOL policy) — deterministic & quick
// ---------------------------------------------------------------------------

function normalizeEol(text: string, eol: 'lf' | 'crlf' | 'auto'): string {
  if (eol === 'auto') return text;
  if (eol === 'crlf') return text.replace(/\r\n/g, '\n').replace(/\n/g, '\r\n');
  return text.replace(/\r\n/g, '\n');
}

function applyWhitespacePolicy(lines: string[], opts: { tabSize: number; insertSpaces: boolean; trimTrailingWhitespace: boolean; }): string[] {
  const unit = opts.insertSpaces ? ' '.repeat(Math.max(1, opts.tabSize)) : '\t';
  return lines.map((line) => {
    if (opts.insertSpaces) {
      const leadingMatch = /^\t+/.exec(line);
      const leading = leadingMatch?.[0] ?? '';
      if (leading.length > 0) {
        line = leading
          .split('')
          .map(() => unit)
          .join('') + line.slice(leading.length);
      }
    } else {
      const re = new RegExp(`^(?: {${opts.tabSize}})+`);
      const match = re.exec(line);
      if (match) {
        const spaces = match[0].length;
        const tabs = Math.floor(spaces / opts.tabSize);
        line = '\t'.repeat(tabs) + line.slice(spaces);
      }
    }
    if (opts.trimTrailingWhitespace) line = line.replace(/[ \t]+$/g, '');
    return line;
  });
}

function formatText(text: string, opts: { tabSize: number; insertSpaces: boolean; trimTrailingWhitespace: boolean; insertFinalNewline: boolean; eol: 'lf'|'crlf'|'auto' }, isFragment = false): string {
  let work = text.replace(/\r\n/g, '\n');
  let lines = work.split('\n');
  lines = applyWhitespacePolicy(lines, opts);
  work = lines.join('\n');
  work = normalizeEol(work, opts.eol);
  if (!isFragment && opts.insertFinalNewline) {
    const term = opts.eol === 'crlf' ? '\r\n' : '\n';
    if (!work.endsWith(term)) work += term;
  }
  return work;
}

// ---------------------------------------------------------------------------
// Start server
// ---------------------------------------------------------------------------

documents.listen(connection);
connection.listen();
logLsp.info('Vitte LSP: listening');
