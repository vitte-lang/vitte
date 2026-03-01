/**
 * Vitte LSP — Language Service (complete minimal)
 * -----------------------------------------------
 * Diagnostics & completion without external parser, with configurable rules.
 * Pure TypeScript. API remains compatible with lsp.ts (doValidation, doComplete, doResolve).
 */

import {
  CompletionItemKind,
  DiagnosticSeverity,
  InsertTextFormat,
  MarkupKind,
} from 'vscode-languageserver/node';
import type {
  Diagnostic,
  CompletionItem,
  TextDocumentPositionParams,
  MarkupContent,
} from 'vscode-languageserver/node';

// Lightweight TextDocument facade (subset used by TextDocuments())
export interface LspTextDocument {
  uri: string;
  getText(range?: { start: { line: number; character: number }; end: { line: number; character: number } }): string;
}

// ---------------------------------------------------------------------------
// Settings (lint/heuristics)
// ---------------------------------------------------------------------------

export interface VitteLintSettings {
  maxLineLength: number;            // warn if a line exceeds this many chars (0 to disable)
  warnTrailingWhitespace: boolean;  // highlight spaces/tabs at end of line
  hintTodoFixme: boolean;           // show TODO/FIXME/XXX as Hint
  warnMixedIndent: boolean;         // warn if both tabs and spaces are used for leading indent in file
}

const DEFAULT_SETTINGS: VitteLintSettings = {
  maxLineLength: 140,
  warnTrailingWhitespace: true,
  hintTodoFixme: true,
  warnMixedIndent: true,
};

// Small helpers
function hasTabsOnly(s: string): boolean { return /^\t+$/.test(s); }
function hasSpacesOnly(s: string): boolean { return /^ +$/.test(s); }

// ---------------------------------------------------------------------------
// Language Service
// ---------------------------------------------------------------------------

export class VitteLanguageService {
  private settings: VitteLintSettings;

  constructor(opts: Partial<VitteLintSettings> = {}) {
    this.settings = { ...DEFAULT_SETTINGS, ...opts };
  }

  /** Update settings at runtime without recreating the service. */
  setSettings(partial: Partial<VitteLintSettings>) {
    this.settings = { ...this.settings, ...partial };
  }

  /** Basic diagnostics: TODO/FIXME/XXX, long lines, trailing spaces, mixed indentation. */
  doValidation(doc: LspTextDocument): Promise<Diagnostic[]> {
    const cfg = this.settings;
    const text = doc.getText();
    const diagnostics: Diagnostic[] = [];

    const lines = text.replace(/\r\n/g, '\n').split('\n');
    let sawTabIndent = false;
    let sawSpaceIndent = false;

    for (let i = 0; i < lines.length; i++) {
      const line = lines[i];

      // Leading indentation sampling for mixed‑indent detection
      const mIndent = /^(\s*)/.exec(line);
      if (mIndent) {
        const lead = mIndent[1];
        if (lead.length > 0) {
          // Only consider pure tab or pure space runs at the beginning
          if (hasTabsOnly(lead)) sawTabIndent = true; else if (hasSpacesOnly(lead)) sawSpaceIndent = true;
        }
      }

      // TODO/FIXME/XXX as Hint
      if (cfg.hintTodoFixme) {
        const m = /(TODO|FIXME|XXX)(:?)(.*)/.exec(line);
        if (m) {
          diagnostics.push({
            severity: DiagnosticSeverity.Hint,
            message: `Note: ${m[1]}${m[2]}${m[3] ?? ''}`.trim(),
            range: { start: { line: i, character: m.index }, end: { line: i, character: m.index + m[0].length } },
            source: 'vitte-lsp',
            code: 'vitte.todofixme',
          });
        }
      }

      // Long line
      if (cfg.maxLineLength > 0 && line.length > cfg.maxLineLength) {
        diagnostics.push({
          severity: DiagnosticSeverity.Warning,
          message: `Ligne trop longue (${line.length} > ${cfg.maxLineLength})`,
          range: { start: { line: i, character: cfg.maxLineLength }, end: { line: i, character: line.length } },
          source: 'vitte-lsp',
          code: 'vitte.maxLineLength',
        });
      }

      // Trailing whitespace
      if (cfg.warnTrailingWhitespace) {
        const tw = /(\s+)$/.exec(line);
        if (tw && tw[1].length > 0) {
          const start = line.length - tw[1].length;
          diagnostics.push({
            severity: DiagnosticSeverity.Warning,
            message: 'Espace en fin de ligne',
            range: { start: { line: i, character: start }, end: { line: i, character: line.length } },
            source: 'vitte-lsp',
            code: 'vitte.trailingWhitespace',
          });
        }
      }
    }

    // Mixed indentation (file scope)
    if (cfg.warnMixedIndent && sawTabIndent && sawSpaceIndent) {
      diagnostics.push({
        severity: DiagnosticSeverity.Warning,
        message: 'Indentation mixte détectée (tabs et espaces). Normalisez votre indentation.',
        range: { start: { line: 0, character: 0 }, end: { line: Math.max(0, lines.length - 1), character: 0 } },
        source: 'vitte-lsp',
        code: 'vitte.mixedIndent',
      });
    }

    return Promise.resolve(diagnostics);
  }

  /**
   * Completions: keywords + simple snippets for declarations.
   * We avoid context parsing; snippets are provided for productivity.
   */
  doComplete(_params: TextDocumentPositionParams): CompletionItem[] {
    const items: CompletionItem[] = [
      { label: 'module', kind: CompletionItemKind.Keyword, detail: 'Déclaration de module', insertTextFormat: InsertTextFormat.Snippet, insertText: 'module ${1:my.module};' },
      { label: 'import', kind: CompletionItemKind.Keyword, detail: 'Import', insertTextFormat: InsertTextFormat.Snippet, insertText: 'import ${1:path::to::item};' },
      { label: 'fn', kind: CompletionItemKind.Keyword, detail: 'Déclaration de fonction', insertTextFormat: InsertTextFormat.Snippet, insertText: 'fn ${1:name}(${2:args}) {\n\t$0\n}' },
      { label: 'struct', kind: CompletionItemKind.Keyword, detail: 'Déclaration de structure', insertTextFormat: InsertTextFormat.Snippet, insertText: 'struct ${1:Name} {\n\t$0\n}' },
      { label: 'enum', kind: CompletionItemKind.Keyword, detail: 'Déclaration d’énumération', insertTextFormat: InsertTextFormat.Snippet, insertText: 'enum ${1:Name} {\n\t${2:Variant}\n}' },
      { label: 'union', kind: CompletionItemKind.Keyword, detail: 'Déclaration d’union', insertTextFormat: InsertTextFormat.Snippet, insertText: 'union ${1:Name} {\n\t$0\n}' },
      { label: 'type', kind: CompletionItemKind.Keyword, detail: 'Alias de type', insertTextFormat: InsertTextFormat.Snippet, insertText: 'type ${1:Name} = ${2:Existing};' },
      { label: 'let', kind: CompletionItemKind.Keyword, detail: 'Binding (variable)', insertTextFormat: InsertTextFormat.Snippet, insertText: 'let ${1:name} = ${2:value};' },
      { label: 'const', kind: CompletionItemKind.Keyword, detail: 'Constante', insertTextFormat: InsertTextFormat.Snippet, insertText: 'const ${1:NAME} = ${2:value};' },
      { label: 'static', kind: CompletionItemKind.Keyword, detail: 'Statique', insertTextFormat: InsertTextFormat.Snippet, insertText: 'static ${1:NAME}: ${2:Type} = ${3:value};' },
      { label: 'return', kind: CompletionItemKind.Keyword, detail: 'Retour de fonction', insertTextFormat: InsertTextFormat.Snippet, insertText: 'return ${1:value};' },
      { label: 'match', kind: CompletionItemKind.Keyword, detail: 'Expression de correspondance', insertTextFormat: InsertTextFormat.Snippet, insertText: 'match ${1:expr} {\n\t${2:pattern} => ${3:result},\n}' },
      { label: 'if', kind: CompletionItemKind.Keyword, detail: 'Condition', insertTextFormat: InsertTextFormat.Snippet, insertText: 'if ${1:cond} {\n\t$0\n}' },
      { label: 'else', kind: CompletionItemKind.Keyword, detail: 'Alternative', insertTextFormat: InsertTextFormat.Snippet, insertText: 'else {\n\t$0\n}' },
      { label: 'while', kind: CompletionItemKind.Keyword, detail: 'Boucle while', insertTextFormat: InsertTextFormat.Snippet, insertText: 'while ${1:cond} {\n\t$0\n}' },
      { label: 'for', kind: CompletionItemKind.Keyword, detail: 'Boucle for', insertTextFormat: InsertTextFormat.Snippet, insertText: 'for ${1:item} in ${2:iter} {\n\t$0\n}' },
      { label: 'loop', kind: CompletionItemKind.Keyword, detail: 'Boucle infinie', insertTextFormat: InsertTextFormat.Snippet, insertText: 'loop {\n\t$0\n}' },
    ];
    return items;
  }

  /** Attach a bit more detail on resolve. */
  doResolve(item: CompletionItem): CompletionItem {
    const label = String(item.label);
    const extra = this._docFor(label);
    item.detail ??= extra.title;
    const documentation: MarkupContent = { kind: MarkupKind.Markdown, value: extra.markdown };
    item.documentation = documentation;
    return item;
  }

  private _docFor(label: string): { title: string; markdown: string } {
    switch (label) {
      case 'module': return { title: 'Déclaration de module', markdown: '**module** — Déclare le module courant.\n\n```vitte\nmodule my.module;\n```' };
      case 'import': return { title: 'Import', markdown: '**import** — Ajoute un symbole d’un autre module.\n\n```vitte\nimport std::core;\n```' };
      case 'fn': return { title: 'Déclaration de fonction', markdown: '**fn** — Déclare une fonction.\n\n```vitte\nfn name(args) {\n    // corps\n}\n```' };
      case 'struct': return { title: 'Déclaration de structure', markdown: '**struct** — Regroupe des champs.\n\n```vitte\nstruct Name {\n    field: Type\n}\n```' };
      case 'enum': return { title: 'Déclaration d’énumération', markdown: '**enum** — Variantes nommées.\n\n```vitte\nenum Name {\n    Variant\n}\n```' };
      case 'union': return { title: 'Déclaration d’union', markdown: '**union** — Représentation mémoire partagée.\n\n```vitte\nunion Name {\n    field: Type\n}\n```' };
      case 'type': return { title: 'Alias de type', markdown: '**type** — Alias vers un autre type.\n\n```vitte\ntype Alias = Existing;\n```' };
      case 'let': return { title: 'Binding', markdown: '**let** — Nouveau binding.\n\n```vitte\nlet x = 1;\n```' };
      case 'const': return { title: 'Constante', markdown: '**const** — Valeur immuable.\n\n```vitte\nconst NAME = 1;\n```' };
      case 'static': return { title: 'Statique', markdown: '**static** — Valeur globale statique.\n\n```vitte\nstatic NAME: Type = init();\n```' };
      case 'match': return { title: 'Expression de correspondance', markdown: '**match** — Branches par motif.\n\n```vitte\nmatch x {\n    0 => 1,\n}\n```' };
      default: return { title: String(label), markdown: `**${label}**` };
    }
  }
}

export default VitteLanguageService;
