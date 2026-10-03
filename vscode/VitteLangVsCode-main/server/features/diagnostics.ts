

import {
  type Connection,
  TextDocuments,
  type TextDocument,
  Diagnostic,
  DiagnosticSeverity,
  Range,
  Position
} from 'vscode-languageserver/node';

/**
 * Diagnostics subsystem for Vitte LSP — strict-safe (no implicit undefined),
 * minimal heuristics, debounced publication, and configuration hooks.
 */

export interface DiagnosticsConfig {
  maxProblems: number;            // hard cap to avoid flooding the client
  longLine: number;               // warn on lines longer than this (0 = disabled)
  allowTabs: boolean;             // flag usage of tabs
  todoSeverity: DiagnosticSeverity; // severity for TODO markers
}

const DEFAULT_CONFIG: DiagnosticsConfig = {
  maxProblems: 200,
  longLine: 160,
  allowTabs: false,
  todoSeverity: DiagnosticSeverity.Hint,
};

// Internal state
let currentConfig: DiagnosticsConfig = { ...DEFAULT_CONFIG };
const debounceMs = 200; // light debounce between rapid edits
const timers = new Map<string, NodeJS.Timeout>();

/** Register diagnostics on connection & documents */
export function registerDiagnostics(connection: Connection, documents: TextDocuments<TextDocument>): void {
  documents.onDidChangeContent((change) => scheduleValidate(connection, change.document));
  documents.onDidOpen((open) => scheduleValidate(connection, open.document));
  documents.onDidClose((close) => clearDiagnostics(connection, close.document));

  // Optional: react to config changes from client (if supported)
  connection.onDidChangeConfiguration((params) => {
    try {
      const cfg = (params.settings as any)?.vitte?.diagnostics as Partial<DiagnosticsConfig> | undefined;
      if (cfg && typeof cfg === 'object') {
        updateConfig(cfg);
        // revalidate all open docs after config change
        for (const doc of documents.all()) scheduleValidate(connection, doc);
      }
    } catch {
      // ignore invalid settings payloads
    }
  });
}

export function updateConfig(partial: Partial<DiagnosticsConfig>): void {
  const next: DiagnosticsConfig = { ...currentConfig };
  if (typeof partial.maxProblems === 'number' && Number.isFinite(partial.maxProblems) && partial.maxProblems >= 0) {
    next.maxProblems = partial.maxProblems;
  }
  if (typeof partial.longLine === 'number' && Number.isFinite(partial.longLine) && partial.longLine >= 0) {
    next.longLine = partial.longLine;
  }
  if (typeof partial.allowTabs === 'boolean') {
    next.allowTabs = partial.allowTabs;
  }
  if (typeof partial.todoSeverity === 'number') {
    next.todoSeverity = partial.todoSeverity as DiagnosticSeverity;
  }
  currentConfig = next;
}

function scheduleValidate(connection: Connection, doc: TextDocument): void {
  const uri = doc.uri;
  const existing = timers.get(uri);
  if (existing) clearTimeout(existing);
  const t = setTimeout(() => {
    timers.delete(uri);
    void validateAndPublish(connection, doc);
  }, debounceMs);
  timers.set(uri, t);
}

function clearDiagnostics(connection: Connection, doc: TextDocument): void {
  // Publish an empty list to clear diagnostics on close
  connection.sendDiagnostics({ uri: doc.uri, diagnostics: [] });
  const timer = timers.get(doc.uri);
  if (timer) {
    clearTimeout(timer);
    timers.delete(doc.uri);
  }
}

async function validateAndPublish(connection: Connection, doc: TextDocument): Promise<void> {
  const diagnostics = computeDiagnostics(doc, currentConfig);
  // Respect cap
  const limited = diagnostics.length > currentConfig.maxProblems
    ? diagnostics.slice(0, currentConfig.maxProblems)
    : diagnostics;
  // Publish
  // In LSP, diagnostics are pushed by server to client
  try { connection.sendDiagnostics({ uri: doc.uri, diagnostics: limited }); } catch { /* ignore */ }
}

/** Compute diagnostics with lightweight, syntax-agnostic heuristics. */
function computeDiagnostics(doc: TextDocument, cfg: DiagnosticsConfig): Diagnostic[] {
  const text = doc.getText();
  const lines = splitLines(text);
  const out: Diagnostic[] = [];

  // Simple brace balance check (pairs of {} [] ()) — heuristic
  const braceStack: Array<{ ch: string; line: number; col: number }> = [];
  const openers = new Set(['{', '(', '[']);
  const closers: Record<string, string> = { '}': '{', ')': '(', ']': '[' };

  for (let i = 0; i < lines.length; i++) {
    const line = lines[i];

    // 1) Trailing whitespace
    const tw = /\s+$/.exec(line);
    if (tw) {
      out.push(diagnostic(
        range(i, Math.max(0, line.length - (tw[0]?.length ?? 0)), i, line.length),
        'Espace(s) de fin de ligne inutile(s)',
        DiagnosticSeverity.Hint,
      ));
    }

    // 2) Tabs when not allowed
    if (!cfg.allowTabs && line.includes('\t')) {
      const col = line.indexOf('\t');
      out.push(diagnostic(range(i, col, i, col + 1), 'Tabulation trouvée (préférer des espaces)', DiagnosticSeverity.Information));
    }

    // 3) Long lines
    if (cfg.longLine > 0 && line.length > cfg.longLine) {
      out.push(diagnostic(range(i, cfg.longLine, i, line.length), `Ligne trop longue (${line.length} > ${cfg.longLine})`, DiagnosticSeverity.Hint));
    }

    // 4) TODO markers
    const todoIdx = line.indexOf('TODO');
    if (todoIdx >= 0) {
      out.push(diagnostic(range(i, todoIdx, i, Math.min(line.length, todoIdx + 4)), 'TODO trouvé', cfg.todoSeverity));
    }

    // 5) Brace tracking
    for (let j = 0; j < line.length; j++) {
      const ch = line[j];
      if (openers.has(ch)) {
        braceStack.push({ ch, line: i, col: j });
      } else if (ch in closers) {
        const need = closers[ch];
        const top = braceStack.pop();
        if (!top || top.ch !== need) {
          out.push(diagnostic(range(i, j, i, j + 1), `Fermeture inattendue « ${ch} »`, DiagnosticSeverity.Warning));
        }
      }
    }
  }

  // Unclosed openers
  for (const b of braceStack) {
    out.push(diagnostic(range(b.line, b.col, b.line, Math.max(b.col + 1, 0)), `Délimiteur « ${b.ch} » non refermé`, DiagnosticSeverity.Warning));
  }

  return out;
}

// ----------------- helpers -----------------

function diagnostic(r: Range, message: string, severity: DiagnosticSeverity): Diagnostic {
  return { range: r, message, severity, source: 'vitte' };
}

function range(sl: number, sc: number, el: number, ec: number): Range {
  return Range.create(Position.create(sl, sc), Position.create(el, ec));
}

function splitLines(text: string): string[] {
  // Keep it simple and fast; LSP Document provides all text
  // Normalize to \n and split, preserving last empty line if any
  return text.replace(/\r\n?/g, '\n').split('\n');
}
