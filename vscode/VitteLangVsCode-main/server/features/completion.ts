import {
  type Connection,
  type TextDocumentPositionParams,
  type CompletionItem,
  type CompletionList,
  CompletionItemKind,
  InsertTextFormat,
  TextDocuments,
  type TextDocument,
  Range,
  Position
} from 'vscode-languageserver/node';

/**
 * Completion subsystem for Vitte LSP — strict-safe, keyword + snippet items,
 * minimal context, and resolve support. No implicit undefineds.
 */

export interface CompletionConfig {
  enableSnippets: boolean;
  maxItems: number;
}

const DEFAULT_CONFIG: CompletionConfig = {
  enableSnippets: true,
  maxItems: 200,
};

let currentConfig: CompletionConfig = { ...DEFAULT_CONFIG };

/** Register completion feature */
export function registerCompletion(connection: Connection, documents: TextDocuments<TextDocument>): void {
  connection.onDidChangeConfiguration((params) => {
    try {
      const cfg = (params.settings as any)?.vitte?.completion as Partial<CompletionConfig> | undefined;
      if (cfg && typeof cfg === 'object') {
        updateConfig(cfg);
      }
    } catch {
      // ignore invalid payload
    }
  });

  connection.onCompletion((params): CompletionList => {
    const doc = documents.get(params.textDocument.uri);
    if (!doc) return emptyList();
    return completeAt(doc, params);
  });

  // Provide resolve to enrich details/documentation lazily
  connection.onCompletionResolve((item: CompletionItem): CompletionItem => {
    if (item.data === 'vitte:snippet:for') {
      item.detail = 'Boucle for (snippet)';
      item.documentation = 'Insère une boucle `for` Vitte avec index et bornes incluses.';
    } else if (item.data === 'vitte:kw:fn') {
      item.detail = 'Déclaration de fonction';
    }
    return item;
  });
}

export function updateConfig(partial: Partial<CompletionConfig>): void {
  const next: CompletionConfig = { ...currentConfig };
  if (typeof partial.enableSnippets === 'boolean') next.enableSnippets = partial.enableSnippets;
  if (typeof partial.maxItems === 'number' && Number.isFinite(partial.maxItems) && partial.maxItems > 0) next.maxItems = partial.maxItems;
  currentConfig = next;
}

// ---------------- core logic ----------------

function completeAt(doc: TextDocument, params: TextDocumentPositionParams): CompletionList {
  const { position } = params;
  const prefix = linePrefix(doc, position);

  const items: CompletionItem[] = [];
  // Keywords (subset placeholder — adapt to Vitte real grammar)
  const keywords: Array<{ label: string; kind: CompletionItemKind; detail?: string; data?: string }> = [
    { label: 'fn', kind: CompletionItemKind.Keyword, detail: 'Déclare une fonction', data: 'vitte:kw:fn' },
    { label: 'let', kind: CompletionItemKind.Keyword, detail: 'Bind immuable' },
    { label: 'mut', kind: CompletionItemKind.Keyword, detail: 'Mutabilité' },
    { label: 'struct', kind: CompletionItemKind.Keyword },
    { label: 'enum', kind: CompletionItemKind.Keyword },
    { label: 'use', kind: CompletionItemKind.Keyword },
    { label: 'mod', kind: CompletionItemKind.Keyword },
    { label: 'impl', kind: CompletionItemKind.Keyword },
    { label: 'return', kind: CompletionItemKind.Keyword },
    { label: 'if', kind: CompletionItemKind.Keyword },
    { label: 'else', kind: CompletionItemKind.Keyword },
    { label: 'for', kind: CompletionItemKind.Keyword },
    { label: 'while', kind: CompletionItemKind.Keyword },
  ];

  for (const k of keywords) {
    if (startsWithWord(prefix, k.label)) {
      items.push({
        label: k.label,
        kind: k.kind,
        detail: k.detail,
        data: k.data,
      });
    }
  }

  if (currentConfig.enableSnippets) {
    items.push(...snippetItems(prefix));
  }

  // Cap results
  const limited = items.length > currentConfig.maxItems ? items.slice(0, currentConfig.maxItems) : items;
  return { isIncomplete: false, items: limited };
}

function snippetItems(prefix: string): CompletionItem[] {
  const out: CompletionItem[] = [];

  if (startsWithWord(prefix, 'for')) {
    out.push({
      label: 'for (i in 0..N)',
      kind: CompletionItemKind.Snippet,
      detail: 'Boucle for (0..N)',
      insertTextFormat: InsertTextFormat.Snippet,
      insertText: 'for (let ${1:i} in 0..${2:N}) {\n    ${3:// ...}\n}',
      data: 'vitte:snippet:for',
    });
  }

  // Function template
  if (startsWithWord(prefix, 'fn')) {
    out.push({
      label: 'fn name(args) -> T {}',
      kind: CompletionItemKind.Snippet,
      detail: 'Déclaration de fonction',
      insertTextFormat: InsertTextFormat.Snippet,
      insertText: 'fn ${1:name}(${2:args}) -> ${3:T} {\n    ${4:// body}\n}',
      data: 'vitte:snippet:fn',
    });
  }

  return out;
}

// ---------------- helpers ----------------

function linePrefix(doc: TextDocument, pos: Position): string {
  const start = Position.create(pos.line, 0);
  const r = Range.create(start, pos);
  const text = doc.getText(r);
  // Keep only the last wordish fragment to compare startsWith
  const m = /([A-Za-z_][A-Za-z0-9_]*)$/.exec(text);
  return m ? m[1] : '';
}

function startsWithWord(prefix: string, word: string): boolean {
  if (prefix.length === 0) return true; // offer at BOF
  return word.startsWith(prefix);
}

function emptyList(): CompletionList {
  return { isIncomplete: false, items: [] };
}
