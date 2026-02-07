// lint.ts — Linter minimal pour Vitte/Vit (LSP), robuste, configurable, directives inline

import {
  DiagnosticSeverity,
  Range,
  Position,
} from "vscode-languageserver/node";
import type { Diagnostic } from "vscode-languageserver/node";
import { RESERVED_WORDS } from "./languageFacts.js";

/* ============================== Config ==================================== */

export interface LintOptions {
  maxLineLength?: number;                // 120 par défaut
  allowTabs?: boolean;                   // false par défaut
  allowTrailingWhitespace?: boolean;     // false par défaut
}

const DEFAULTS: Required<LintOptions> = {
  maxLineLength: 120,
  allowTabs: false,
  allowTrailingWhitespace: false,
};

/* =============================== Regex ==================================== */

const rxTrailingWS = /[ \t]+$/;
const rxTab = /\t/;
const rxTodo = /(^|\s)(TODO|FIXME|XXX)(:|\b)/;
const rxLineEndSemicolon = /;\s*$/;

const rxIdent = /[A-Za-z_][A-Za-z0-9_]*/g;
const rxNumber =
  /(?:0x[0-9A-Fa-f](?:[0-9A-Fa-f_])*)|(?:0b[01](?:[01_])*)|(?:\d(?:[\d_])*(?:\.(?:\d(?:[\d_])*))?(?:[eE][+-]?\d(?:[\d_])*)?)/g;

const rxString = /"(?:\\.|[^"\\])*"/g;
const rxChar = /'(?:\\.|[^'\\])'/g;

const rxCommentLine = /\/\/[^\n]*/g;
const rxCommentDoc = /\/\/![^\n]*/g;
const rxCommentBlock = /\/\*[\s\S]*?\*\//g;

/* ============================ Directives ================================== */
/**
 * Directives supportées:
 *  - Ligne:  // vitte-lint:disable-next-line rule1,rule2
 *  - Bloc:   /* vitte-lint:disable rule1,rule2 *\/ ... /* vitte-lint:enable *\/
 *  - Pragma: // vitte-lint maxLineLength=100 allowTabs=true allowTrailingWhitespace=true
 */

const RULES = {
  Tabs: "format.tabs",
  TrailingWhitespace: "format.trailingWhitespace",
  LineLength: "format.lineLength",
  Todo: "note.todo",
  Brackets: "syntax.brackets",
  Number: "lex.number",
  IdentKeywordShadow: "style.ident.keywordShadow",
  Semicolon: "style.semicolon",
} as const;

type RuleId = typeof RULES[keyof typeof RULES];

/** Fix TS2345: disposer d’un garde de type pour convertir un `string` en `RuleId`. */
const ALL_RULES: readonly RuleId[] = Object.values(RULES) as readonly RuleId[];
function isRuleId(x: string): x is RuleId {
  // cast en readonly string[] uniquement pour l’opération includes, tout en conservant RuleId côté retour
  return (ALL_RULES as readonly string[]).includes(x);
}

type DisableMap = Map<number, Set<RuleId>>;
interface DisableRanges { byRule: Map<RuleId, { start: number; end: number }[]>; }

interface Pragmas {
  overrides: Partial<LintOptions>;
}

/* =============================== Core ===================================== */

export function lintText(
  text: string,
  uri = "file://unknown",
  opts: LintOptions = {}
): Diagnostic[] {
  const pragmas = parsePragmas(text);
  const cfg: Required<LintOptions> = { ...DEFAULTS, ...pragmas.overrides, ...opts };

  const { lineDisables, blockDisables } = collectDisables(text);
  const diags: Diagnostic[] = [];

  const lines = text.split(/\r?\n/);

  for (let i = 0; i < lines.length; i++) {
    const line = lines[i];

    if (!isDisabled(i, RULES.Tabs, lineDisables, blockDisables)) {
      const m = rxTab.exec(line);
      if (!cfg.allowTabs && m) {
        diags.push(diag(i, m.index, i, m.index + 1,
          "Tabulation détectée. Utiliser des espaces.",
          DiagnosticSeverity.Warning, uri, RULES.Tabs));
      }
    }

    if (!isDisabled(i, RULES.TrailingWhitespace, lineDisables, blockDisables)) {
      const trailing = rxTrailingWS.exec(line);
      if (!cfg.allowTrailingWhitespace && trailing) {
        const start = trailing.index ?? (line.length - trailing[0].length);
        diags.push(diag(i, start, i, line.length,
          "Espaces en fin de ligne.",
          DiagnosticSeverity.Hint, uri, RULES.TrailingWhitespace));
      }
    }

    if (!isDisabled(i, RULES.LineLength, lineDisables, blockDisables)) {
      if (line.length > cfg.maxLineLength) {
        diags.push(diag(i, cfg.maxLineLength, i, line.length,
          `Ligne trop longue (${line.length} > ${cfg.maxLineLength}).`,
          DiagnosticSeverity.Information, uri, RULES.LineLength));
      }
    }

    if (!isDisabled(i, RULES.Todo, lineDisables, blockDisables)) {
      const todo = rxTodo.exec(line);
      if (todo) {
        const idx = todo.index ?? 0;
        diags.push(diag(i, idx, i, idx + todo[0].length,
          "Marqueur de tâche détecté.",
          DiagnosticSeverity.Hint, uri, RULES.Todo));
      }
    }
  }

  const stripped = stripNonCode(text);

  diags.push(...checkBrackets(stripped, uri, lineDisables, blockDisables));
  diags.push(...checkIdentifiersAndKeywords(stripped, uri, lineDisables, blockDisables));
  diags.push(...checkSemicolonHeuristics(stripped, uri, lineDisables, blockDisables));

  return diags;
}

/* ============================= Helpers ==================================== */

function diag(
  line: number, chStart: number, lineEnd: number, chEnd: number,
  message: string, severity: DiagnosticSeverity, uri: string, code: RuleId
): Diagnostic {
  return {
    range: Range.create(Position.create(line, chStart), Position.create(lineEnd, chEnd)),
    message,
    severity,
    source: "vitte-lint",
    code,
  };
}

function stripNonCode(src: string): string {
  return src
    .replace(rxCommentBlock, (m) => " ".repeat(m.length))
    .replace(rxCommentDoc, (m) => " ".repeat(m.length))
    .replace(rxCommentLine, (m) => " ".repeat(m.length))
    .replace(rxString, (m) => " ".repeat(m.length))
    .replace(rxChar, (m) => " ".repeat(m.length));
}

/* ============================ Brackets ==================================== */

function checkBrackets(
  text: string,
  uri: string,
  lineDisables: DisableMap,
  blockDisables: DisableRanges
): Diagnostic[] {
  const diags: Diagnostic[] = [];
  const pairs: Record<string, string> = { ")": "(", "]": "[", "}": "{" };
  const openers = new Set(["(", "[", "{"]);
  const stack: { ch: string; line: number; col: number }[] = [];

  const lines = text.split(/\r?\n/);
  for (let i = 0; i < lines.length; i++) {
    const L = lines[i];
    for (let j = 0; j < L.length; j++) {
      const c = L[j];
      if (openers.has(c)) stack.push({ ch: c, line: i, col: j });
      else if (c in pairs) {
        if (stack.length === 0) {
          if (!isDisabled(i, RULES.Brackets, lineDisables, blockDisables)) {
            diags.push(diag(i, j, i, j + 1,
              `Fermeture inattendue '${c}'.`,
              DiagnosticSeverity.Error, uri, RULES.Brackets));
          }
        } else {
          const top = stack.pop()!;
          if (pairs[c] !== top.ch) {
            if (!isDisabled(i, RULES.Brackets, lineDisables, blockDisables)) {
              diags.push(diag(i, j, i, j + 1,
                `Attendu '${matchingCloser(top.ch)}' avant '${c}'.`,
                DiagnosticSeverity.Error, uri, RULES.Brackets));
            }
          }
        }
      }
    }
  }
  for (const unclosed of stack) {
    if (!isDisabled(unclosed.line, RULES.Brackets, lineDisables, blockDisables)) {
      diags.push(diag(unclosed.line, unclosed.col, unclosed.line, unclosed.col + 1,
        `Délimiteur non fermé '${unclosed.ch}'.`,
        DiagnosticSeverity.Error, uri, RULES.Brackets));
    }
  }
  return diags;
}

function matchingCloser(open: string): string {
  if (open === "(") return ")";
  if (open === "[") return "]";
  return "}";
}

/* ===================== Idents, nombres, mots-clés ========================= */

function checkIdentifiersAndKeywords(
  text: string,
  uri: string,
  lineDisables: DisableMap,
  blockDisables: DisableRanges
): Diagnostic[] {
  const diags: Diagnostic[] = [];
  const lines = text.split(/\r?\n/);

  for (let i = 0; i < lines.length; i++) {
    const L = lines[i];

    if (!isDisabled(i, RULES.Number, lineDisables, blockDisables)) {
      for (const m of L.matchAll(rxNumber)) {
        const tok = m[0];
        if (tok.includes("__") || tok.startsWith("_") || tok.endsWith("_")) {
          const s = m.index ?? 0;
          diags.push(diag(i, s, i, s + tok.length,
            "Littéral numérique invalide (underscore mal placé).",
            DiagnosticSeverity.Warning, uri, RULES.Number));
        }
      }
    }

    if (!isDisabled(i, RULES.IdentKeywordShadow, lineDisables, blockDisables)) {
      for (const m of L.matchAll(rxIdent)) {
        const tok = m[0];
        const idx = m.index ?? 0;
        if (RESERVED_WORDS.has(tok)) continue;

        for (const kw of RESERVED_WORDS) {
          if (tok === kw) continue;
          if (tok.startsWith(kw) && tok.length > kw.length && /[A-Za-z0-9_]/.test(tok[kw.length])) continue;
          if (tok.endsWith(kw) && tok.length > kw.length) {
            diags.push(diag(i, idx, i, idx + tok.length,
              `Éviter d'incorporer le mot-clé '${kw}' dans un identifiant.`,
              DiagnosticSeverity.Hint, uri, RULES.IdentKeywordShadow));
            break;
          }
        }
      }
    }
  }
  return diags;
}

/* ======================== Heuristique point-virgule ======================= */

function checkSemicolonHeuristics(
  text: string,
  uri: string,
  lineDisables: DisableMap,
  blockDisables: DisableRanges
): Diagnostic[] {
  const diags: Diagnostic[] = [];
  const lines = text.split(/\r?\n/);

  const starters = /^(?:\s*)(?:let|const|return|break|continue|type)\b/;
  const blockEnders = /^(?:\s*)[}\])]\s*$/;

  for (let i = 0; i < lines.length; i++) {
    if (isDisabled(i, RULES.Semicolon, lineDisables, blockDisables)) continue;

    const L = lines[i];
    if (blockEnders.test(L)) continue;
    if (/{\s*$/.test(L)) continue;
    if (/\b(if|while|for|match)\b/.test(L)) continue;

    if (starters.test(L)) {
      if (!rxLineEndSemicolon.test(L)) {
        const col = Math.max(0, L.length - 1);
        diags.push(diag(i, col, i, L.length,
          "Point-virgule probablement attendu ici.",
          DiagnosticSeverity.Hint, uri, RULES.Semicolon));
      }
    }
  }
  return diags;
}

/* ============================ Directives utils ============================ */

function parsePragmas(text: string): Pragmas {
  const lines = text.split(/\r?\n/);
  const overrides: Partial<LintOptions> = {};
  for (let i = 0; i < Math.min(20, lines.length); i++) {
    const L = lines[i].trim();
    if (!L.startsWith("//")) continue;
    const m = /\/\/\s*vitte-lint\s+(.*)$/i.exec(L);
    if (!m) continue;
    const kvs = m[1].split(/\s+/);
    for (const kv of kvs) {
      const [k, v] = kv.split("=");
      if (!k || v === undefined) continue;
      if (k === "maxLineLength") {
        const n = Number(v);
        if (Number.isFinite(n) && n > 0) overrides.maxLineLength = n;
      } else if (k === "allowTabs") {
        overrides.allowTabs = v === "true";
      } else if (k === "allowTrailingWhitespace") {
        overrides.allowTrailingWhitespace = v === "true";
      }
    }
  }
  return { overrides };
}

function collectDisables(text: string): { lineDisables: DisableMap; blockDisables: DisableRanges } {
  const lines = text.split(/\r?\n/);
  const lineDisables: DisableMap = new Map();
  const block: DisableRanges = { byRule: new Map() };

  // Ligne: // vitte-lint:disable-next-line ruleA,ruleB
  for (let i = 0; i < lines.length; i++) {
    const m = /\/\/\s*vitte-lint:disable-next-line\s+([A-Za-z0-9_.,-\s]+)/i.exec(lines[i]);
    if (!m) continue;
    const rules = splitRules(m[1]);
    const target = i + 1;
    if (!lineDisables.has(target)) lineDisables.set(target, new Set<RuleId>());
    const set = lineDisables.get(target)!;
    for (const r of rules) set.add(mapRuleAlias(r));
  }

  // Bloc: /* vitte-lint:disable rule */ ... /* vitte-lint:enable */
  const blockRx = /\/\*\s*vitte-lint:(disable|enable)\s*([^*]*)\*\//gi;
  const events: { line: number; kind: "disable" | "enable"; rules: string[] }[] = [];

  for (let i = 0; i < lines.length; i++) {
    let m: RegExpExecArray | null;
    const L = lines[i];
    blockRx.lastIndex = 0;
    while ((m = blockRx.exec(L))) {
      const kind = m[1].toLowerCase() as "disable" | "enable";
      const rules = splitRules(m[2] ?? "");
      events.push({ line: i, kind, rules });
    }
  }

  const openByRule = new Map<RuleId, number>();
  for (const ev of events) {
    for (const r of ev.rules) {
      const rule = mapRuleAlias(r);
      if (ev.kind === "disable") {
        openByRule.set(rule, ev.line);
      } else {
        const start = openByRule.get(rule);
        if (start !== undefined) {
          addRange(block, rule, start, ev.line);
          openByRule.delete(rule);
        }
      }
    }
  }
  for (const [rule, start] of openByRule) addRange(block, rule, start, lines.length - 1);

  return { lineDisables, blockDisables: block };
}

function splitRules(s: string): string[] {
  return s
    .split(/[,\s]+/)
    .map(x => x.trim())
    .filter(Boolean);
}

function mapRuleAlias(name: string): RuleId {
  switch (name) {
    case "tabs": return RULES.Tabs;
    case "trailing-ws":
    case "trailingWhitespace": return RULES.TrailingWhitespace;
    case "line-length":
    case "lineLength": return RULES.LineLength;
    case "todo": return RULES.Todo;
    case "brackets": return RULES.Brackets;
    case "number": return RULES.Number;
    case "ident-shadow":
    case "keyword-shadow": return RULES.IdentKeywordShadow;
    case "semicolon": return RULES.Semicolon;
    default:
      return isRuleId(name) ? name : RULES.LineLength;
  }
}

function addRange(store: DisableRanges, rule: RuleId, start: number, end: number) {
  if (!store.byRule.has(rule)) store.byRule.set(rule, []);
  store.byRule.get(rule)!.push({ start, end });
}

function isDisabled(
  line: number,
  rule: RuleId,
  lineDisables: DisableMap,
  blockDisables: DisableRanges
): boolean {
  if (lineDisables.get(line)?.has(rule)) return true;
  const ranges = blockDisables.byRule.get(rule);
  if (!ranges) return false;
  for (const r of ranges) if (line >= r.start && line <= r.end) return true;
  return false;
}

/* ============================= LSP Bridge ================================= */

export function lintToPublishable(text: string, uri: string, options?: LintOptions): Diagnostic[] {
  return lintText(text, uri, options);
}
