// semantic.ts — simple, robuste, et plus complet
// Améliorations clés:
// - Lexer en un seul passage: collecte des plages commentaires et chaînes + masque code
// - Émission triée et sans chevauchement pour respecter LSP SemanticTokens
// - Coloration ciblée des noms de déclarations (module/fn/struct/enum/type/let|const)
// - Paramètres de fonctions et propriétés de struct
// - Mots-clés et nombres uniquement en zones de code
// - Cache par document/version
// - Hover concis sur mots-clés

import {
  MarkupKind,
  SemanticTokensBuilder,
} from "vscode-languageserver/node";
import type {
  Position,
  Hover,
  SemanticTokensLegend,
  SemanticTokens,
} from "vscode-languageserver/node";
import type { TextDocument } from "vscode-languageserver-textdocument";
import { RESERVED_WORDS, PRIMITIVE_TYPE_SET, BOOL_LITERAL_SET, NIL_LITERAL_SET } from "./languageFacts.js";

/* ------------------------------ Legend stable ----------------------------- */
// Garder l’ordre en phase avec server.ts
const TOKEN_TYPES = [
  "namespace", // 0
  "type",      // 1
  "function",  // 2
  "variable",  // 3
  "parameter", // 4
  "property",  // 5
  "keyword",   // 6
  "number",    // 7
  "string",    // 8
  "comment",   // 9
] as const;

const TOKEN_MODIFIERS: string[] = [];

const TYPE_INDEX = {
  namespace: 0,
  type: 1,
  function: 2,
  variable: 3,
  parameter: 4,
  property: 5,
  keyword: 6,
  number: 7,
  string: 8,
  comment: 9,
} as const;

export function getSemanticTokensLegend(): SemanticTokensLegend {
  return { tokenTypes: Array.from(TOKEN_TYPES), tokenModifiers: TOKEN_MODIFIERS };
}

/* --------------------------------- Hover ---------------------------------- */

const HOVER_DOC: Record<string, string> = {
  module: "Déclare le module courant.",
  import: "Importe un chemin depuis un autre module.",
  use: "Importe un symbole depuis un module.",
  space: "Déclare l’espace courant.",
  as: "Assigne un alias à un import.",
  pub: "Rend le symbole public.",
  struct: "Définit une structure.",
  form: "Définit une structure (form).",
  enum: "Définit une énumération.",
  union: "Définit une union.",
  type: "Déclare un alias de type.",
  fn: "Déclare une fonction.",
  proc: "Déclare une procédure.",
  extern: "Déclare une liaison externe.",
  let: "Déclare une variable locale.",
  mut: "Rend un binding mutable.",
  const: "Déclare une constante.",
  static: "Déclare un symbole statique.",
  where: "Contraintes de type.",
  if: "Instruction conditionnelle.",
  elif: "Branche conditionnelle intermédiaire.",
  else: "Branche alternative.",
  match: "Branches par motifs.",
  while: "Boucle conditionnelle.",
  for: "Boucle itérative.",
  in: "Itération sur une séquence.",
  loop: "Boucle infinie interrompue par break.",
  break: "Interrompt une boucle.",
  continue: "Passe à l’itération suivante.",
  return: "Retourne depuis une fonction.",
  give: "Retourne depuis une procédure.",
  true: "Booléen vrai.",
  false: "Booléen faux.",
  nil: "Valeur nulle.",
  null: "Valeur nulle.",
};

export function provideHover(doc: TextDocument, position: Position): Hover | null {
  const w = wordAt(doc, position);
  if (!w) return null;
  const info = HOVER_DOC[w];
  if (info) {
    return { contents: { kind: MarkupKind.Markdown, value: `**${w}** — ${info}` } };
  }
  if (PRIMITIVE_TYPE_SET.has(w)) {
    return { contents: { kind: MarkupKind.Markdown, value: `**${w}** — Type primitif.` } };
  }
  if (BOOL_LITERAL_SET.has(w)) {
    return { contents: { kind: MarkupKind.Markdown, value: `**${w}** — Littéral booléen.` } };
  }
  if (NIL_LITERAL_SET.has(w)) {
    return { contents: { kind: MarkupKind.Markdown, value: `**${w}** — Littéral nul.` } };
  }
  return null;
}

/* --------------------------- Semantic tokeniser --------------------------- */

const KW = RESERVED_WORDS;

// Cache basique par document/version
const semCache = new WeakMap<TextDocument, { version: number; tokens: SemanticTokens }>();

export function buildSemanticTokens(doc: TextDocument): SemanticTokens {
  const cached = semCache.get(doc);
  if (cached && cached.version === doc.version) return cached.tokens;

  const text = doc.getText();
  const lex = scanLex(text); // { mask, strings, comments }
  const nlIdx = buildLineIndex(text);

  // On collecte d’abord tous les spans, puis on trie, puis on émet
  interface Span { start: number; end: number; type: number; }
  const spans: Span[] = [];

  // 1) commentaires et chaînes (priorité forte)
  for (const [s, e] of lex.comments) insertSpan(spans, s, e, TYPE_INDEX.comment);
  for (const [s, e] of lex.strings) insertSpan(spans, s, e, TYPE_INDEX.string);

  // 2) nombres en zones de code
  for (const m of matchAll(/\b\d(?:_?\d)*(?:\.(?:\d(?:_?\d)*)?)?(?:[eE][+-]?\d+)?\b/g, text)) {
    if (!lex.mask[m.index]) continue;
    insertSpan(spans, m.index, m.index + m[0].length, TYPE_INDEX.number);
  }

  // 3) mots-clés en zones de code
  for (const m of matchAll(/\b[A-Za-z_]\w*\b/g, text)) {
    if (!lex.mask[m.index]) continue;
    if (!KW.has(m[0])) continue;
    insertSpan(spans, m.index, m.index + m[0].length, TYPE_INDEX.keyword);
  }

  // 4) déclarations: colorer uniquement le nom
  addDeclSpans(text, lex.mask, /\bmodule\s+([A-Za-z_][\w./:]*)/g, 1, TYPE_INDEX.namespace, spans);
  addDeclSpans(text, lex.mask, /\bspace\s+([A-Za-z_][\w./:]*)/g, 1, TYPE_INDEX.namespace, spans);
  addDeclSpans(text, lex.mask, /\bstruct\s+([A-Za-z_]\w*)/g, 1, TYPE_INDEX.type, spans);
  addDeclSpans(text, lex.mask, /\bform\s+([A-Za-z_]\w*)/g, 1, TYPE_INDEX.type, spans);
  addDeclSpans(text, lex.mask, /\benum\s+([A-Za-z_]\w*)/g, 1, TYPE_INDEX.type, spans);
  addDeclSpans(text, lex.mask, /\bunion\s+([A-Za-z_]\w*)/g, 1, TYPE_INDEX.type, spans);
  addDeclSpans(text, lex.mask, /\btype\s+([A-Za-z_]\w*)/g, 1, TYPE_INDEX.type, spans);
  addDeclSpans(text, lex.mask, /\b(?:fn|proc)\s+([A-Za-z_]\w*)\s*\(/g, 1, TYPE_INDEX.function, spans);
  addDeclSpans(text, lex.mask, /\b(?:let|const|static)\s+(?:mut\s+)?([A-Za-z_]\w*)/g, 1, TYPE_INDEX.variable, spans);

  // 5) paramètres de fonctions
  for (const m of matchAll(/\b(?:fn|proc)\s+[A-Za-z_]\w*\s*\(([^)]*)\)/g, text)) {
    if (!lex.mask[m.index]) continue; // début du fn
    const params = (m[1] ?? "").split(",");
    const base = m.index + m[0].indexOf("(") + 1;
    let offset = 0;
    for (const p of params) {
      const mm = /\s*([A-Za-z_]\w*)/.exec(p);
      if (!mm) { offset += p.length + 1; continue; }
      const name = mm[1];
      const local = p.indexOf(name);
      const off = base + offset + (local >= 0 ? local : 0);
      if (name && lex.mask[off]) insertSpan(spans, off, off + name.length, TYPE_INDEX.parameter);
      offset += p.length + 1; // +1 pour la virgule
    }
  }

  // 6) propriétés de struct
  for (const m of matchAll(/\b(?:struct|form)\s+[A-Za-z_]\w*\s*\{([\s\S]*?)\}/g, text)) {
    const body = m[1] ?? "";
    const bodyStart = m.index + m[0].indexOf("{") + 1;
    for (const fm of matchAll(/(^|\s)([A-Za-z_]\w*)\s*:\n?\s*[^,\n\r\}]+/g, body)) {
      const name = fm[2];
      const off = bodyStart + fm.index + fm[0].lastIndexOf(name);
      if (lex.mask[off]) insertSpan(spans, off, off + name.length, TYPE_INDEX.property);
    }
  }

  // Tri global et émission
  spans.sort((a, b) => a.start - b.start || a.end - b.end);
  const builder = new SemanticTokensBuilder();
  for (const s of spans) pushMultiline(builder, nlIdx, s.start, s.end, s.type);

  const tokens = builder.build();
  semCache.set(doc, { version: doc.version, tokens });
  return tokens;
}

/* --------------------------------- utils ---------------------------------- */

function wordAt(doc: TextDocument, pos: Position): string | null {
  const text = doc.getText();
  const off = doc.offsetAt(pos);
  let s = off, e = off;
  while (s > 0 && /[A-Za-z0-9_]/.test(text.charAt(s - 1))) s--;
  while (e < text.length && /[A-Za-z0-9_]/.test(text.charAt(e))) e++;
  return e > s ? text.slice(s, e) : null;
}

// Scan lexical: collecte commentaires/chaînes et construit un masque code
function scanLex(text: string): { mask: Uint8Array; strings: [number, number][]; comments: [number, number][] } {
  const n = text.length;
  const mask = new Uint8Array(n); // 1 = code, 0 = non-code
  const strings: [number, number][] = [];
  const comments: [number, number][] = [];

  let i = 0;
  const markCode = (from: number, to: number) => { for (let k = from; k < to; k++) mask[k] = 1; };

  while (i < n) {
    const c = text.charCodeAt(i);
    const c2 = i + 1 < n ? text.charCodeAt(i + 1) : 0;

    // line comment #... (ignore attributes #[...])
    if (c === 0x23 /* # */ && c2 !== 0x5b /* [ */) {
      const start = i;
      i += 1;
      while (i < n && text.charCodeAt(i) !== 0x0a) i++;
      comments.push([start, i]);
      continue;
    }
    // line comment //...
    if (c === 0x2f && c2 === 0x2f) {
      const start = i;
      i += 2;
      while (i < n && text.charCodeAt(i) !== 0x0a) i++;
      comments.push([start, i]);
      continue;
    }
    // block comment /* ... */
    if (c === 0x2f && c2 === 0x2a) {
      const start = i;
      i += 2;
      while (i < n) {
        if (text.charCodeAt(i) === 0x2a && i + 1 < n && text.charCodeAt(i + 1) === 0x2f) { i += 2; break; }
        i++;
      }
      comments.push([start, i]);
      continue;
    }
    // raw string r#*"..."#*
    if (c === 0x72 /* r */) {
      let j = i + 1, hashes = 0;
      while (j < n && text.charCodeAt(j) === 0x23 /* # */) { hashes++; j++; }
      if (j < n && text.charCodeAt(j) === 0x22 /* " */) {
        const start = i; j++;
        for (; j < n; j++) {
          if (text.charCodeAt(j) === 0x22) {
            let k = j + 1, ok = true;
            for (let h = 0; h < hashes; h++) { if (k >= n || text.charCodeAt(k) !== 0x23) { ok = false; break; } k++; }
            if (ok) { j = k; break; }
          }
        }
        strings.push([start, j]);
        i = j;
        continue;
      }
    }
    // normal string "..." ou '...'
    if (c === 0x22 || c === 0x27) {
      const start = i;
      const quote = c; i++;
      while (i < n) {
        if (text.charCodeAt(i) === 0x5c /* \\ */) { i += 2; continue; }
        if (text.charCodeAt(i) === quote) { i++; break; }
        i++;
      }
      strings.push([start, i]);
      continue;
    }

    // code chunk jusqu’au prochain début de com/str
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
    markCode(start, i);
  }

  return { mask, strings, comments };
}

// Index des débuts de lignes pour conversion rapide offset→(line, char)
function buildLineIndex(text: string): number[] {
  const idx: number[] = [0];
  for (let i = 0; i < text.length; i++) if (text.charCodeAt(i) === 10 /*\n*/) idx.push(i + 1);
  return idx;
}

function offsetToLC(nlIdx: number[], off: number): [line: number, char: number] {
  let lo = 0, hi = nlIdx.length - 1;
  while (lo <= hi) {
    const mid = (lo + hi) >> 1;
    const v = nlIdx[mid];
    if (v === off) return [mid, 0];
    if (v < off) lo = mid + 1; else hi = mid - 1;
  }
  const line = Math.max(0, lo - 1);
  return [line, off - nlIdx[line]];
}

function pushMultiline(
  builder: SemanticTokensBuilder,
  nlIdx: number[],
  startOff: number,
  endOff: number,
  tokenType: number,
) {
  const [sLine, sChar] = offsetToLC(nlIdx, startOff);
  const [eLine, eChar] = offsetToLC(nlIdx, endOff);
  if (sLine === eLine) {
    builder.push(sLine, sChar, endOff - startOff, tokenType, 0);
    return;
  }
  // première ligne
  const lineEnd = nlIdx[sLine + 1] ?? endOff;
  builder.push(sLine, sChar, lineEnd - startOff, tokenType, 0);
  // lignes intermédiaires
  for (let ln = sLine + 1; ln < eLine; ln++) {
    const ls = nlIdx[ln];
    const le = (nlIdx[ln + 1] ?? endOff) - 1; // exclure saut de ligne
    if (le > ls) builder.push(ln, 0, le - ls, tokenType, 0);
  }
  // dernière ligne
  builder.push(eLine, 0, eChar, tokenType, 0);
}

function addDeclSpans(
  text: string,
  mask: Uint8Array,
  rx: RegExp,
  group: number,
  tokenTypeIndex: number,
  spans: { start: number; end: number; type: number }[]
) {
  rx.lastIndex = 0;
  let m: RegExpExecArray | null;
  while ((m = rx.exec(text))) {
    const name = m[group];
    if (!name || !/^[A-Za-z_]\w*$/.test(name)) continue;
    const nameOff = (m.index ?? 0) + m[0].indexOf(name);
    if (!mask[nameOff]) continue;
    insertSpan(spans, nameOff, nameOff + name.length, tokenTypeIndex);
  }
}

function insertSpan(spans: { start: number; end: number; type: number }[], start: number, end: number, type: number) {
  if (end <= start) return;
  // éviter les chevauchements: si recouvrement détecté, on ignore la nouvelle plage
  for (const s of spans) {
    if (!(end <= s.start || start >= s.end)) return;
  }
  spans.push({ start, end, type });
}

function* matchAll(rx: RegExp, s: string): Generator<RegExpMatchArray & { index: number }> {
  const r = new RegExp(rx.source, rx.flags.includes("g") ? rx.flags : rx.flags + "g");
  let m: RegExpExecArray | null;
  while ((m = r.exec(s))) {
    const arr = m as RegExpMatchArray & { index: number };
    arr.index = m.index ?? 0;
    if (m[0].length === 0) r.lastIndex++;
    yield arr;
  }
}

