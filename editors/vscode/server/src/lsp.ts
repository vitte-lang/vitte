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
  CompletionItemKind,
} from 'vscode-languageserver/node';
import type {
  InitializeParams,
  InitializeResult,
  CompletionItem,
  TextDocumentPositionParams,
  Range,
} from 'vscode-languageserver/node';

import { TextDocument } from 'vscode-languageserver-textdocument';

import Config, { defaultFormatting } from './config';
import type { FormattingSettings } from './config';
import { logLsp, attachConnection } from './logger';
import { VitteLanguageService } from './languageService';
import type { LspTextDocument } from './languageService';
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
const lang = new VitteLanguageService();

let hasConfigurationCapability = false;
let hasWorkspaceFolderCapability = false;

interface FormatDocumentParams {
  textDocument?: { uri?: string };
}

interface FormatRangeParams extends FormatDocumentParams {
  range?: Range;
}

interface ExpandSelectionParams extends FormatDocumentParams {
  position?: { line: number; character: number };
}

function toLspDocument(doc: TextDocument): LspTextDocument {
  return {
    uri: doc.uri,
    getText: doc.getText.bind(doc),
  };
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
      // We provide completion via VitteLanguageService (with resolve)
      completionProvider: { resolveProvider: true, triggerCharacters: ['.', ':', '>'] },
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
  }
});

// ---------------------------------------------------------------------------
// Configuration changes → clear settings & revalidate
// ---------------------------------------------------------------------------

const handleConfigChange = Config.makeOnDidChangeConfigurationHandler<TextDocument>(connection, {
  getOpenDocuments: () => documents.all(),
  validateDocument: async (doc) => {
    try {
      const diagnostics = await lang.doValidation(toLspDocument(doc));
      void connection.sendDiagnostics({ uri: doc.uri, diagnostics });
    } catch (e) {
      logLsp.warn('Validation error after config change', { uri: doc.uri, error: String(e) });
    }
  },
});

connection.onDidChangeConfiguration(() => {
  void handleConfigChange();
});

// ---------------------------------------------------------------------------
// Diagnostics lifecycle
// ---------------------------------------------------------------------------

documents.onDidOpen((e) => { void handleValidate(e.document); });
documents.onDidChangeContent((e) => { void handleValidate(e.document); });
documents.onDidClose((e) => {
  // Clear diagnostics when closing
  void connection.sendDiagnostics({ uri: e.document.uri, diagnostics: [] });
});

async function handleValidate(doc: TextDocument) {
  try {
    const diags = await lang.doValidation(toLspDocument(doc));
    void connection.sendDiagnostics({ uri: doc.uri, diagnostics: diags });
    logLsp.debug('Diagnostics sent', { uri: doc.uri, count: diags.length });
  } catch (err) {
    logLsp.error('Validation failed', { uri: doc.uri, error: String(err) });
  }
}

// ---------------------------------------------------------------------------
// Completion
// ---------------------------------------------------------------------------

connection.onCompletion((params: TextDocumentPositionParams): CompletionItem[] => {
  try {
    const items = lang.doComplete(params);
    return items;
  } catch (err) {
    logLsp.error('Completion failed', { error: String(err) });
    return [
      { label: 'error', kind: CompletionItemKind.Text, detail: 'Error during completion' },
    ];
  }
});

connection.onCompletionResolve((item: CompletionItem): CompletionItem => {
  try { return lang.doResolve(item); } catch { return item; }
});

// ---------------------------------------------------------------------------
// Formatting (document, range, documentOrRange)
// ---------------------------------------------------------------------------

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
