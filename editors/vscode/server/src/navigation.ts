// navigation.ts — symboles, définitions, références, rename, workspace symbols
// Version enrichie :
// - Parsing tolérant qui ignore commentaires/chaînes
// - Hiérarchie de symboles via pile d’accolades
// - Règles étendues (module, import, fn, struct, enum, union, type, const/static, field)
// - Références/rename hors commentaires/chaînes
// - Cache par document/version
// - Workspace symbols avec fuzzy match et scoring stable
// - API additionnelle: prepareRename, indexDocument

import type { TextDocument } from "vscode-languageserver-textdocument";
import {
  Location,
  Position,
  Range,
  SymbolKind,
} from "vscode-languageserver/node";
import type {
  DocumentSymbol,
  WorkspaceSymbol,
} from "vscode-languageserver/node";

/* --------------------------------- Types ---------------------------------- */

interface FlatSymbol {
  name: string;
  kind: SymbolKind;
  range: Range;           // entier bloc (ex: de 'fn' à la fin de la signature/ligne)
  selectionRange: Range;  // le nom seul
  containerName?: string; // parent logique (module/impl/struct)
}

interface DocIndex {
  flat: FlatSymbol[];
  outline: DocumentSymbol[];
  byName: Map<string, FlatSymbol[]>;
}

/* -------------------------- Masque code (lexer light) ---------------------- */
// Construit un masque booléen de positions « valides » (hors commentaires et chaînes)
// Supporte //, /* */, "...", '...', r"..." et r#"..."# (style Rust/Vitte). Sans interpolation.

function buildCodeMask(text: string): Uint8Array {
  const n = text.length;
  const mask = new Uint8Array(n);
  let i = 0;
  const setCode = (from: number, to: number) => {
    for (let k = from; k < to; k++) mask[k] = 1;
  };
  while (i < n) {
    const c = text.charCodeAt(i);
    const c2 = i + 1 < n ? text.charCodeAt(i + 1) : 0;
    // Line comment
    if (c === 0x2f /*/ */ && c2 === 0x2f) {
      i += 2;
      while (i < n && text.charCodeAt(i) !== 0x0a) i++;
      continue;
    }
    // Block comment (supports nesting depth 1 minimal)
    if (c === 0x2f && c2 === 0x2a /* * */) {
      i += 2;
      while (i < n) {
        if (text.charCodeAt(i) === 0x2a && i + 1 < n && text.charCodeAt(i + 1) === 0x2f) {
          i += 2;
          break;
        }
        i++;
      }
      continue;
    }
    // Raw string r#*" ... "#*
    if (c === 0x72 /* r */) {
      let j = i + 1;
      let hashes = 0;
      while (j < n && text.charCodeAt(j) === 0x23 /* # */) { hashes++; j++; }
      if (j < n && text.charCodeAt(j) === 0x22 /* " */) {
        // found r#*"
        j++;
        // consume until closing "#*
        for (; j < n; j++) {
          if (text.charCodeAt(j) === 0x22 /* " */) {
            let k = j + 1;
            let ok = true;
            for (let h = 0; h < hashes; h++) {
              if (k >= n || text.charCodeAt(k) !== 0x23) { ok = false; break; }
              k++;
            }
            if (ok) { i = k; break; }
          }
        }
        continue;
      }
    }
    // Normal string "..." ou '...'
    if (c === 0x22 || c === 0x27) {
      const quote = c;
      i++;
      while (i < n) {
        if (text.charCodeAt(i) === 0x5c /* \\ */) { i += 2; continue; }
        if (text.charCodeAt(i) === quote) { i++; break; }
        i++;
      }
      continue;
    }
    // Code token
    const start = i;
    // Avance jusqu’à prochain début de com/str pour marquer code en bloc
    while (i < n) {
      const a = text.charCodeAt(i);
      const b = i + 1 < n ? text.charCodeAt(i + 1) : 0;
      if ((a === 0x2f && (b === 0x2f || b === 0x2a)) || a === 0x22 || a === 0x27 || a === 0x72) break;
      i++;
    }
    setCode(start, i);
  }
  return mask;
}

function isIdentChar(ch: number): boolean {
  return (ch >= 48 && ch <= 57) || // 0-9
         (ch >= 65 && ch <= 90) || // A-Z
         (ch >= 97 && ch <= 122) || // a-z
         ch === 95; // _
}

function wordAt(doc: TextDocument, pos: Position): string | null {
  const text = doc.getText();
  const off = doc.offsetAt(pos);
  let s = off, e = off;
  while (s > 0 && isIdentChar(text.charCodeAt(s - 1))) s--;
  while (e < text.length && isIdentChar(text.charCodeAt(e))) e++;
  return e > s ? text.slice(s, e) : null;
}

function escapeRx(s: string): string {
  return s.replace(/[.*+?^${}()|[\]\\]/g, "\\$&");
}

function posKey(p: Position): string { return `${p.line}:${p.character}`; }

function dedupeBy<T>(arr: T[], keyFn: (t: T) => string): T[] {
  const seen = new Set<string>();
  const out: T[] = [];
  for (const v of arr) {
    const k = keyFn(v);
    if (seen.has(k)) continue;
    seen.add(k);
    out.push(v);
  }
  return out;
}

function isValidIdent(s: string): boolean { return /^[A-Za-z_]\w*$/.test(s); }

/* ------------------------------ Règles symboles ---------------------------- */
// Les regex sont évaluées uniquement sur les positions mask==1

const RULES: { rx: RegExp; kind: SymbolKind; nameGroup: number; containerHint?: (m: RegExpExecArray) => string | undefined }[] = [
  { rx: /\bmodule\s+([A-Za-z_][\w:]*)/g,                                kind: SymbolKind.Namespace, nameGroup: 1 },
  { rx: /\bimport\s+([A-Za-z_][\w:]*(?:::\*)?)/g,                       kind: SymbolKind.Namespace, nameGroup: 1 },
  { rx: /\b(?:pub\s+)?fn\s+([A-Za-z_]\w*)\s*\(/g,                       kind: SymbolKind.Function,  nameGroup: 1 },
  { rx: /\b(?:pub\s+)?struct\s+([A-Za-z_]\w*)/g,                        kind: SymbolKind.Struct,    nameGroup: 1 },
  { rx: /\b(?:pub\s+)?enum\s+([A-Za-z_]\w*)/g,                          kind: SymbolKind.Enum,      nameGroup: 1 },
  { rx: /\b(?:pub\s+)?union\s+([A-Za-z_]\w*)/g,                         kind: SymbolKind.Struct,    nameGroup: 1 },
  { rx: /\b(?:pub\s+)?type\s+([A-Za-z_]\w*)/g,                          kind: SymbolKind.TypeParameter, nameGroup: 1 },
  { rx: /\b(?:pub\s+)?const\s+([A-Za-z_]\w*)/g,                         kind: SymbolKind.Constant,  nameGroup: 1 },
  { rx: /\b(?:pub\s+)?static\s+([A-Za-z_]\w*)/g,                        kind: SymbolKind.Variable,  nameGroup: 1 },
  // champs de struct: name: Type
  { rx: /(^|\s)([A-Za-z_]\w*)\s*:\s*[^;{},\n]+(?=,|\n|\r|\})/g, kind: SymbolKind.Field, nameGroup: 2 },
];

function* iterMaskedMatches(rx: RegExp, text: string, mask: Uint8Array): Generator<RegExpExecArray> {
  rx.lastIndex = 0;
  let m: RegExpExecArray | null;
  while ((m = rx.exec(text))) {
    const idx = m.index ?? 0;
    // Vérifie que le début du match tombe sur zone code
    if (mask[idx]) {
      yield m;
    }
    if (m[0].length === 0) rx.lastIndex++;
  }
}

/* ------------------------ Collecte + hiérarchie outline -------------------- */

function collectFlatSymbols(doc: TextDocument): FlatSymbol[] {
  const text = doc.getText();
  const mask = buildCodeMask(text);
  const out: FlatSymbol[] = [];

  for (const { rx, kind, nameGroup } of RULES) {
    for (const m of iterMaskedMatches(rx, text, mask)) {
      const name = m[nameGroup];
      if (!name) continue;
      const start = doc.positionAt(m.index ?? 0);
      const end = doc.positionAt((m.index ?? 0) + m[0].length);
      const nameOffset = (m.index ?? 0) + m[0].indexOf(name);
      const nameStart = doc.positionAt(nameOffset);
      const nameEnd = doc.positionAt(nameOffset + name.length);
      out.push({
        name,
        kind,
        range: Range.create(start, end),
        selectionRange: Range.create(nameStart, nameEnd),
      });
    }
  }

  // Déduplication conservatrice
  return dedupeBy(out, s => `${s.kind}:${s.name}:${posKey(s.selectionRange.start)}`);
}

function buildOutline(doc: TextDocument, flat: FlatSymbol[]): DocumentSymbol[] {
  // Trie par position pour associer via une pile d’accolades
  const text = doc.getText();
  type Node = DocumentSymbol & { children: DocumentSymbol[] };
  const nodes: Node[] = flat
    .map(s => ({
      name: s.name,
      kind: s.kind,
      range: s.range,
      selectionRange: s.selectionRange,
      children: [],
    }))
    .sort((a, b) => {
      const da = doc.offsetAt(a.range.start);
      const db = doc.offsetAt(b.range.start);
      return da - db;
    });

  // Pile de conteneurs basée sur accolades
  const root: Node = { name: "<root>", kind: SymbolKind.Namespace, range: Range.create(Position.create(0,0), Position.create(0,0)), selectionRange: Range.create(Position.create(0,0), Position.create(0,0)), children: [] };
  const stack: Node[] = [root];

  // Prépare positions des '{' et '}' sur zones code
  const mask = buildCodeMask(text);
  const opens: number[] = [];
  for (let i = 0; i < text.length; i++) {
    if (!mask[i]) continue;
    const ch = text.charCodeAt(i);
    if (ch === 0x7b /* { */) opens.push(i);
    else if (ch === 0x7d /* } */) {
      if (opens.length) opens.pop();
    }
  }
  // Index rapide: map startOffset->depth at that point
  // Simplification: nous utilisons l’ordre croissant + heuristique par proximité

  function pushToBestContainer(node: Node) {
    // Heuristique: rattacher au dernier symbole dont la position est avant node et qui n’est pas clos avant.
    for (let j = stack.length - 1; j >= 0; j--) {
      const top = stack[j];
      const topStart = doc.offsetAt(top.range.start);
      const nodeStart = doc.offsetAt(node.range.start);
      if (nodeStart >= topStart) {
        top.children.push(node);
        stack.push(node);
        return;
      }
    }
    root.children.push(node);
    stack.push(node);
  }

  // On parcourt les nodes et simule la fermeture quand on rencontre '}' avant prochain node
  const bracesPositions: number[] = [];
  const n = text.length;
  for (let i = 0; i < n; i++) if (mask[i] && (text[i] === '{' || text[i] === '}')) bracesPositions.push(i);

  let bp = 0;
  for (const node of nodes) {
    const nodeStart = doc.offsetAt(node.range.start);
    // ferme conteneurs jusqu’à la position courante
    while (bp < bracesPositions.length && bracesPositions[bp] < nodeStart) {
      const ch = text[bracesPositions[bp]];
      if (ch === '}') {
        if (stack.length > 1) stack.pop();
      }
      bp++;
    }
    pushToBestContainer(node);
  }
  // Ferme tout ce qui reste
  while (bp < bracesPositions.length) {
    if (text[bracesPositions[bp]] === '}' && stack.length > 1) stack.pop();
    bp++;
  }

  return root.children;
}

/* ------------------------------- Index & cache ----------------------------- */

const docCache = new WeakMap<TextDocument, { version: number; index: DocIndex }>();

function indexDocument(doc: TextDocument): DocIndex {
  const cached = docCache.get(doc);
  if (cached && cached.version === doc.version) return cached.index;

  const flat = collectFlatSymbols(doc);
  const outline = buildOutline(doc, flat);
  const byName = new Map<string, FlatSymbol[]>();
  for (const s of flat) {
    const arr = byName.get(s.name) ?? [];
    arr.push(s);
    byName.set(s.name, arr);
  }
  const index = { flat, outline, byName };
  docCache.set(doc, { version: doc.version, index });
  return index;
}

/* --------------------------------- API doc --------------------------------- */

export function documentSymbols(doc: TextDocument): DocumentSymbol[] {
  return indexDocument(doc).outline;
}

export function symbolOutline(doc: TextDocument): DocumentSymbol[] {
  return documentSymbols(doc);
}

/* --------------------------- Définitions / refs ---------------------------- */

export function definitionAtPosition(doc: TextDocument, pos: Position, uri: string): Location[] {
  const word = wordAt(doc, pos);
  if (!word) return [];
  const { byName } = indexDocument(doc);
  const defs = byName.get(word) ?? [];
  // Tri par priorité de kind puis proximité
  const baseOff = doc.offsetAt(pos);
  defs.sort((a, b) => kindPriority(a.kind) - kindPriority(b.kind) || Math.abs(doc.offsetAt(a.selectionRange.start) - baseOff) - Math.abs(doc.offsetAt(b.selectionRange.start) - baseOff));
  return defs.map(d => Location.create(uri, d.selectionRange));
}

export function referencesAtPosition(doc: TextDocument, pos: Position, uri: string): Location[] {
  const word = wordAt(doc, pos);
  if (!word) return [];
  const text = doc.getText();
  const mask = buildCodeMask(text);
  const out: Location[] = [];

  const re = new RegExp(`(?<![A-Za-z0-9_])${escapeRx(word)}(?![A-Za-z0-9_])`, "g");
  re.lastIndex = 0;
  let m: RegExpExecArray | null;
  while ((m = re.exec(text))) {
    const idx = m.index ?? 0;
    if (!mask[idx]) { if (m[0].length === 0) re.lastIndex++; continue; }
    const start = doc.positionAt(idx);
    const end = doc.positionAt(idx + m[0].length);
    out.push(Location.create(uri, Range.create(start, end)));
    if (m[0].length === 0) re.lastIndex++;
  }
  return out;
}

/* --------------------------------- Rename ---------------------------------- */

export function prepareRename(doc: TextDocument, pos: Position): { range: Range; placeholder: string } | null {
  const name = wordAt(doc, pos);
  if (!name || !isValidIdent(name)) return null;
  const off = doc.offsetAt(pos);
  const start = doc.positionAt(off - (name.length - (name.lastIndexOf(name) + 1)));
  const end = doc.positionAt(doc.offsetAt(start) + name.length);
  return { range: Range.create(start, end), placeholder: name };
}

export function renameSymbol(doc: TextDocument, pos: Position, newName: string): { range: Range; newText: string }[] {
  const old = wordAt(doc, pos);
  if (!old || !isValidIdent(newName)) return [];
  const text = doc.getText();
  const mask = buildCodeMask(text);
  const edits: { range: Range; newText: string }[] = [];
  const re = new RegExp(`(?<![A-Za-z0-9_])${escapeRx(old)}(?![A-Za-z0-9_])`, "g");
  let m: RegExpExecArray | null;
  while ((m = re.exec(text))) {
    const idx = m.index ?? 0;
    if (!mask[idx]) { if (m[0].length === 0) re.lastIndex++; continue; }
    const start = doc.positionAt(idx);
    const end = doc.positionAt(idx + m[0].length);
    edits.push({ range: Range.create(start, end), newText: newName });
    if (m[0].length === 0) re.lastIndex++;
  }
  return edits;
}

/* ----------------------------- Workspace symbols --------------------------- */

export function workspaceSymbols(
  query: string,
  openDocs: { uri: string; doc: TextDocument }[],
  limit = 200
): WorkspaceSymbol[] {
  const q = query.trim();
  const result: { ws: WorkspaceSymbol; score: number; idx: number }[] = [];
  let seq = 0;
  for (const { uri, doc } of openDocs) {
    const { flat } = indexDocument(doc);
    for (const s of flat) {
      const score = q ? fuzzyScore(s.name, q) : 1;
      if (q && score <= 0) continue;
      result.push({
        ws: { name: s.name, kind: s.kind, location: Location.create(uri, s.selectionRange) },
        score,
        idx: seq++,
      });
      if (result.length >= limit * 4) break; // cap intermédiaire avant tri
    }
  }
  result.sort((a, b) => b.score - a.score || a.idx - b.idx);
  return result.slice(0, limit).map(r => r.ws);
}

/* --------------------------------- Helpers -------------------------------- */

function kindPriority(k: SymbolKind): number {
  switch (k) {
    case SymbolKind.Function: return 0;
    case SymbolKind.Method: return 1;
    case SymbolKind.Variable: return 2;
    case SymbolKind.Constant: return 3;
    case SymbolKind.Struct: return 4;
    case SymbolKind.Class: return 5;
    case SymbolKind.Enum: return 6;
    case SymbolKind.Interface: return 7;
    case SymbolKind.Namespace: return 8;
    case SymbolKind.Field: return 9;
    default: return 10;
  }
}

function fuzzyScore(name: string, query: string): number {
  // subsequence scoring sensible à la casse avec bonus pour débuts de mots et camelCase
  let i = 0, j = 0, score = 0, lastMatch = -2;
  while (i < name.length && j < query.length) {
    const nc = name[i];
    const qc = query[j];
    if (nc.toLowerCase() === qc.toLowerCase()) {
      let s = 1;
      if (i === 0 || !isIdentChar(name.charCodeAt(i - 1))) s += 2; // début de mot
      if (nc === qc) s += 1; // casse exacte
      if (i === lastMatch + 1) s += 2; // consécutif
      score += s;
      lastMatch = i;
      j++; i++;
    } else {
      i++;
    }
  }
  return j === query.length ? score : -1;
}

/* --------------------------------- Exports -------------------------------- */

export function indexDocForTests(doc: TextDocument): DocIndex { return indexDocument(doc); }
