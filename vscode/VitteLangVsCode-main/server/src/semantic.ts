import { lexicalContextAt, maskNonCode, scanLexical } from "./lexical.js";
// semantic.ts — simple, robuste, et plus complet
// Améliorations clés:
// - Lexer en un seul passage: collecte des plages commentaires et chaînes + masque code
// - Émission triée et sans chevauchement pour respecter LSP SemanticTokens
// - Coloration ciblée des noms de déclarations Vitte réelles
// - Paramètres de fonctions et champs form/pick
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
import type { SourceSymbol } from "./sourceSymbols.js";
import { RESERVED_WORDS, PRIMITIVE_TYPE_SET, BOOL_LITERAL_SET, NIL_LITERAL_SET, NUMBER_PATTERN } from "./languageFacts.js";
import { resolveAliasType, resolveSourceSymbol, sourceIndex, visibleBindingsAt } from "./sourceSymbols.js";
import { COMPILER_BUILTINS } from "./callableFacts.js";
import { importedCompletionModel, readModuleExports, type CompletionEnvironment, type ImportedCompletionSymbol } from "./moduleResolver.js";

/* ------------------------------ Legend stable ----------------------------- */
// Garder l’ordre en phase avec server.ts
const TOKEN_TYPES = [
  "namespace", // 0
  "type",      // 1
  "function",  // 2
  "variable",  // 3
  "parameter", // 4
  "property",  // 5
  "macro",     // 6  (e.g. diagnostics code VITTE-XXXX)
  "keyword",   // 7
  "number",    // 8
  "string",    // 9
  "comment",   // 10
  "operator",  // 11
  "enumMember", // 12
  "decorator", // 13
] as const;

const TOKEN_MODIFIERS = ["declaration", "definition", "readonly", "public", "async", "modification"];
const MODIFIER_BIT = {
  declaration: 1 << 0,
  definition: 1 << 1,
  readonly: 1 << 2,
  public: 1 << 3,
  async: 1 << 4,
  modification: 1 << 5,
} as const;

interface SemanticSpan {
  start: number;
  end: number;
  type: number;
  modifiers: number;
  priority: number;
}

const TYPE_INDEX = {
  namespace: 0,
  type: 1,
  function: 2,
  variable: 3,
  parameter: 4,
  property: 5,
  macro: 6,
  keyword: 7,
  number: 8,
  string: 9,
  comment: 10,
  operator: 11,
  enumMember: 12,
  decorator: 13,
} as const;

export function getSemanticTokensLegend(): SemanticTokensLegend {
  return { tokenTypes: Array.from(TOKEN_TYPES), tokenModifiers: TOKEN_MODIFIERS };
}

/* --------------------------------- Hover ---------------------------------- */

const HOVER_DOC: Record<string, string> = {
  space: "Déclare l’espace courant.",
  pull: "Importe un package avec alias.",
  use: "Importe un symbole depuis un package.",
  as: "Assigne un alias à un import.",
  form: "Définit une structure (form).",
  pick: "Définit une somme de cas.",
  case: "Déclare un cas dans un pick ou un match.",
  type: "Déclare un alias de type.",
  proc: "Déclare une procédure.",
  let: "Déclare une variable locale.",
  const: "Déclare une constante.",
  if: "Instruction conditionnelle.",
  else: "Branche alternative.",
  match: "Branches par motifs.",
  for: "Boucle itérative.",
  in: "Itération sur une séquence.",
  loop: "Boucle infinie interrompue par break.",
  break: "Interrompt une boucle.",
  continue: "Passe à l’itération suivante.",
  return: "Retourne depuis une fonction.",
  give: "Retourne depuis une procédure.",
  requires: "Déclare une précondition de procédure.",
  ensures: "Déclare une postcondition de procédure.",
  elif: "Ajoute une branche conditionnelle.",
  set: "Affecte une valeur à une cible modifiable.",
  defer: "Déclare une action différée.",
  true: "Booléen vrai.",
  false: "Booléen faux.",
};

export function provideHover(doc: TextDocument, position: Position, importedSymbols: readonly ImportedCompletionSymbol[] = []): Hover | null {
  if (lexicalContextAt(doc.getText(), doc.offsetAt(position)) !== "code") return null;
  const w = wordAt(doc, position);
  if (!w) return null;
  const resolved = resolveSourceSymbol(doc, position)?.symbol;
  if (resolved) return hoverForSourceSymbol(resolved);
  const imported = importedSymbols.find((symbol) => symbol.name === w);
  if (imported) return hoverForImportedSymbol(imported);
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
  const builtin = COMPILER_BUILTINS.find((candidate) => candidate.name === w);
  if (builtin) return hoverMarkdown(`**proc ${builtin.name}(${builtin.parameters.join(", ")}) -> ${builtin.returnType}**\n\n${builtin.documentation}\n\nDéclaration intrinsèque du compilateur; aucune définition source navigable n’est fournie par Vitte 0.1.1.`);
  return hoverMarkdown(`**${w}**\n\nType ou déclaration indisponible : aucune information sémantique fiable n’est fournie par le frontend actif pour cet identifiant.`);
}

export async function provideHoverWithImports(doc: TextDocument, position: Position, environment: CompletionEnvironment): Promise<Hover | null> {
  if (lexicalContextAt(doc.getText(), doc.offsetAt(position)) !== "code") return null;
  const span = wordAt(doc, position);
  if (!span) return null;
  const imports = await importedCompletionModel(doc, environment);
  const index = sourceIndex(doc);
  const offset = doc.offsetAt(position);
  const prefix = maskNonCode(doc.getText().slice(0, offset));
  const memberPrefix = /([A-Za-z_]\w*)\s*\.\s*$/.exec(prefix);
  if (memberPrefix?.[1]) {
    const receiver = visibleBindingsAt(index.symbols, memberPrefix[1], offset)[0];
    const receiverName = receiver?.type?.match(/[A-Za-z_]\w*/)?.[0];
    const resolvedType = resolveAliasType(index.model.types, receiverName) ?? receiverName;
    const localField = index.symbols.find((symbol) => symbol.role === "field" && symbol.ownerType === resolvedType && symbol.name === span);
    if (localField) return hoverForSourceSymbol(localField);
    const importedType = imports.symbols.find((symbol) => symbol.name === receiverName && symbol.type);
    const field = importedType?.type?.fields.find((candidate) => candidate.name === span);
    if (importedType?.type && field) {
      const sections = [`**${field.name}: ${field.type}**`, `Champ public de \`${importedType.name}\` dans \`${importedType.modulePath}\`.`];
      if (field.documentation) sections.push(field.documentation);
      return hoverMarkdown(sections.join("\n\n"));
    }
    const importedMethod = imports.symbols.find((symbol) => symbol.name === span && symbol.procedure?.ownerType === resolvedType);
    if (importedMethod) return hoverForImportedSymbol(importedMethod);
  }
  const qualifiedPrefix = /([A-Za-z_]\w*)\s*::\s*$/.exec(prefix);
  if (qualifiedPrefix?.[1]) {
    const localVariant = index.symbols.find((symbol) => symbol.role === "variant" && symbol.ownerType === qualifiedPrefix[1] && symbol.name === span);
    if (localVariant) return hoverForSourceSymbol(localVariant);
    const importedType = imports.symbols.find((symbol) => symbol.name === qualifiedPrefix[1] && symbol.type);
    const variant = importedType?.type?.variants.find((candidate) => candidate.name === span);
    if (importedType?.type && variant) {
      const payload = variant.payload.length ? `(${variant.payload.join(", ")})` : "";
      const sections = [`**${variant.name}${payload}**`, `Variante publique de \`${importedType.name}\` dans \`${importedType.modulePath}\`.`];
      if (variant.documentation) sections.push(variant.documentation);
      return hoverMarkdown(sections.join("\n\n"));
    }
  }
  const imported = imports.symbols.find((symbol) => symbol.name === span);
  if (imported) return hoverForImportedSymbol(imported);
  const module = imports.modules.find((candidate) => candidate.alias === span);
  if (module) return hoverMarkdown(`**module ${module.alias}**\n\nChemin : \`${module.modulePath}\`\n\n${module.uri ? `Fichier : \`${module.uri}\`` : "Le fichier du module n’a pas été résolu."}`);
  const code = maskNonCode(doc.getText());
  const cursorOffset = doc.offsetAt(position);
  const lineStart = code.lastIndexOf("\n", cursorOffset) + 1;
  const linePrefix = code.slice(lineStart, cursorOffset);
  const path = /\b(?:use|pull)\s+([A-Za-z_]\w*(?:(?:::|\/)[A-Za-z_]\w*)*)/.exec(linePrefix)?.[1];
  if (path?.split(/::|\//).includes(span)) {
    const moduleExports = await readModuleExports(path, doc.uri, environment);
    if (moduleExports.moduleUri) return hoverMarkdown(`**module ${span}**\n\nChemin : \`${path}\`\n\nFichier : \`${moduleExports.moduleUri}\``);
    return hoverMarkdown(`**${span}**\n\nModule indisponible : le fichier correspondant n’a pas été trouvé dans les racines configurées.`);
  }
  return provideHover(doc, position, imports.symbols);
}

function hoverForSourceSymbol(symbol: SourceSymbol): Hover {
  const sections: string[] = [];
  if (symbol.role === "procedure") {
    sections.push(`**${symbol.signature ?? "proc " + symbol.name}**`);
    sections.push(symbol.isPublic ? "Visibilité : publique." : "Visibilité : privée au module.");
    if (symbol.ownerType || symbol.traitOwner) sections.push(`Méthode dans la portée de \`${symbol.ownerType ?? symbol.traitOwner}\`.`);
  } else if (symbol.role === "type") {
    sections.push(`**${symbol.detail ?? symbol.name}**`);
    sections.push(symbol.isPublic ? "Visibilité : publique." : "Visibilité : privée au module.");
  } else if (symbol.role === "field") {
    sections.push(`**${symbol.name}: ${symbol.type ?? "type inconnu"}**`);
    if (symbol.ownerType) sections.push(`Champ de \`${symbol.ownerType}\`.`);
    sections.push(symbol.isPublic ? "Visibilité : publique avec son type." : "Visibilité : privée au module.");
  } else if (symbol.role === "variant") {
    sections.push(`**${symbol.name}${symbol.detail && symbol.detail !== symbol.type ? `(${symbol.detail})` : ""}**`);
    if (symbol.type) sections.push(`Variante de \`${symbol.type}\`.`);
    sections.push(symbol.isPublic ? "Visibilité : publique avec son type." : "Visibilité : privée au module.");
  } else if (symbol.role === "parameter") {
    sections.push(`**paramètre ${symbol.name}${symbol.type ? ": " + symbol.type : ""}**`);
    if (!symbol.type) sections.push("Type du paramètre indisponible dans la déclaration.");
  } else if (symbol.role === "binding") {
    sections.push(`**${symbol.detail ?? symbol.name}**`);
    sections.push(symbol.type ? `Type annoté ou déduit de l’initialiseur : \`${symbol.type}\`.` : "Type non déduit par le frontend actif.");
  } else if (symbol.role === "module") {
    sections.push(`**module ${symbol.name}**`);
    if (symbol.detail) sections.push(`Chemin : \`${symbol.detail}\`.`);
  } else if (symbol.role === "implementation") {
    sections.push(`**${symbol.detail ?? symbol.name}**`);
  }
  if (symbol.documentation) sections.push(symbol.documentation);
  return hoverMarkdown(sections.join("\n\n"));
}

function hoverForImportedSymbol(symbol: ImportedCompletionSymbol): Hover {
  const sections: string[] = [];
  if (symbol.procedure) {
    const procedure = symbol.procedure;
    const generics = procedure.generics.length ? `<${procedure.generics.join(", ")}>` : "";
    const parameters = procedure.parameters.map((parameter) => (parameter.mode ? parameter.mode + " " : "") + parameter.name + (parameter.type ? ": " + parameter.type : "")).join(", ");
    sections.push(`**proc ${symbol.name}${generics}(${parameters})${procedure.returnType ? " -> " + procedure.returnType : ""}**`);
    sections.push("Visibilité : publique via l’import explicite.");
  } else if (symbol.type) {
    const type = symbol.type;
    const generics = type.generics.length ? `<${type.generics.join(", ")}>` : "";
    sections.push(`**${type.kind} ${symbol.name}${generics}${type.aliasTarget ? " = " + type.aliasTarget : ""}**`);
    sections.push("Visibilité : publique via l’import explicite.");
  } else {
    sections.push(`**const ${symbol.name}${symbol.constantType ? ": " + symbol.constantType : ""}**`);
    sections.push("Visibilité : publique via l’import explicite.");
  }
  if (symbol.originalName !== symbol.name) sections.push(`Alias de \`${symbol.originalName}\`.`);
  if (symbol.documentation) sections.push(symbol.documentation);
  sections.push(`Module : \`${symbol.modulePath}\`.`);
  return hoverMarkdown(sections.join("\n\n"));
}

function hoverMarkdown(value: string): Hover {
  return { contents: { kind: MarkupKind.Markdown, value } };
}

/* --------------------------- Semantic tokeniser --------------------------- */

const KW = RESERVED_WORDS;
const WORD_OPERATORS = new Set(["and", "or", "not", "is"]);

// Cache basique par document/version
const semCache = new WeakMap<TextDocument, { version: number; tokens: SemanticTokens }>();

export function buildSemanticTokens(doc: TextDocument): SemanticTokens {
  const cached = semCache.get(doc);
  if (cached && cached.version === doc.version) return cached.tokens;

  const text = doc.getText();
  const lex = scanLexical(text);
  const nlIdx = buildLineIndex(text);

  // On collecte d’abord tous les spans, puis on trie, puis on émet
  const spans: SemanticSpan[] = [];

  // 1) commentaires et chaînes (priorité forte)
  for (const span of lex.spans) insertSpan(spans, span.start, span.end, TYPE_INDEX[span.kind], 0, 100);

  // 2) nombres en zones de code
  for (const m of matchAll(NUMBER_PATTERN, text)) {
    if (!lex.mask[m.index]) continue;
    insertSpan(spans, m.index, m.index + m[0].length, TYPE_INDEX.number, 0, 30);
  }

  // Attributes are syntax names even when they spell a language keyword.
  for (const m of matchAll(/#!?\[\s*([A-Za-z_]\w*(?:(?:::\s*[A-Za-z_]\w*)*)?)/g, text)) {
    const raw = m[1] ?? "";
    const base = m.index + m[0].indexOf(raw);
    for (const segment of matchAll(/[A-Za-z_]\w*/g, raw)) {
      const start = base + segment.index;
      if (lex.mask[start]) insertSpan(spans, start, start + segment[0].length, TYPE_INDEX.decorator, 0, 70);
    }
  }

  // 3) codes diagnostics en zones de code (ex: VITTE-1001)
  for (const m of matchAll(/\bVITTE-[A-Z0-9][A-Z0-9_-]*\b/g, text)) {
    if (!lex.mask[m.index]) continue;
    insertSpan(spans, m.index, m.index + m[0].length, TYPE_INDEX.macro, 0, 50);
  }

  // 4) mots-clés en zones de code
  for (const m of matchAll(/\b[A-Za-z_]\w*\b/g, text)) {
    if (!lex.mask[m.index]) continue;
    if (!KW.has(m[0])) continue;
    const type = WORD_OPERATORS.has(m[0])
      ? TYPE_INDEX.operator
      : PRIMITIVE_TYPE_SET.has(m[0]) ? TYPE_INDEX.type : TYPE_INDEX.keyword;
    const modifiers = m[0] === "pub" || m[0] === "export"
      ? MODIFIER_BIT.public
      : m[0] === "async" ? MODIFIER_BIT.async
        : m[0] === "mut" ? MODIFIER_BIT.modification : 0;
    insertSpan(spans, m.index, m.index + m[0].length, type, modifiers, 40);
  }

  // Compiler-independent source symbols give declarations and resolvable local
  // references stable roles; unresolved names retain lexical fallback below.
  const index = sourceIndex(doc);
  const knownSymbolNames = new Set(index.symbols.map((symbol) => symbol.name));
  for (const symbol of index.symbols) {
    const type = semanticTypeForRole(symbol.role);
    if (type === undefined) continue;
    let modifiers = MODIFIER_BIT.declaration | MODIFIER_BIT.definition;
    if (symbol.isPublic) modifiers |= MODIFIER_BIT.public;
    modifiers |= bindingModifiers(text, symbol);
    if (symbol.role === "procedure") {
      const prefix = text.slice(Math.max(0, symbol.start - 48), symbol.start);
      if (/\basync\s*$/.test(prefix)) modifiers |= MODIFIER_BIT.async;
    }
    upsertSpan(spans, symbol.nameStart, symbol.nameEnd, type, modifiers, 90);
  }

  // Resolved references use the declaration's role and lexical fallback only
  // applies when the active source index cannot prove a target.
  for (const m of matchAll(/[A-Za-z_]\w*/g, text)) {
    const start = m.index;
    if (!lex.mask[start]) continue;
    if (!knownSymbolNames.has(m[0])) continue;
    const symbol = resolveSourceSymbol(doc, doc.positionAt(start))?.symbol;
    if (!symbol) continue;
    const type = semanticTypeForRole(symbol.role);
    if (type === undefined) continue;
    const modifiers = bindingModifiers(text, symbol);
    insertSpan(spans, start, start + m[0].length, type, modifiers, 80);
  }

  addIdentifierSpans(text, lex.mask, spans);
  for (const m of matchAll(/(?:\.\.=?|\?\?|\?\.|::|->|=>|==|!=|<=|>=|&&|\|\||<<|>>|\+=|-=|\*=|\/=|%=|[=+\-*\/%&|^!~<>.])/g, text)) {
    if (lex.mask[m.index]) insertSpan(spans, m.index, m.index + m[0].length, TYPE_INDEX.operator, 0, 5);
  }

  // Resolve overlaps after collection. This keeps classification O(n log n)
  // instead of scanning every previous token for each identifier.
  spans.sort((a, b) => a.start - b.start || b.priority - a.priority || b.end - a.end);
  const builder = new SemanticTokensBuilder();
  let previousEnd = -1;
  for (const s of spans) {
    if (s.start < previousEnd) continue;
    pushMultiline(builder, text, nlIdx, s.start, s.end, s.type, s.modifiers);
    previousEnd = s.end;
  }

  const tokens = builder.build();
  semCache.set(doc, { version: doc.version, tokens });
  return tokens;
}

function addIdentifierSpans(
  text: string,
  mask: Uint8Array,
  spans: SemanticSpan[],
) {
  for (const m of matchAll(/[A-Za-z_]\w*/g, text)) {
    const start = m.index;
    const end = start + m[0].length;
    const name = m[0];
    if (!mask[start] || KW.has(name) || PRIMITIVE_TYPE_SET.has(name)) continue;

    let beforeOffset = start - 1;
    while (beforeOffset >= 0 && /\s/.test(text[beforeOffset])) beforeOffset--;
    let afterOffset = end;
    while (afterOffset < text.length && /\s/.test(text[afterOffset])) afterOffset++;
    const previousPair = text.slice(Math.max(0, beforeOffset - 1), beforeOffset + 1);
    const after = text[afterOffset] ?? "";
    let type: number;
    if (/^[A-Z]/.test(name)) type = TYPE_INDEX.type;
    else if (after === "(") type = TYPE_INDEX.function;
    else if (text[beforeOffset] === "." || previousPair === "::") type = TYPE_INDEX.property;
    else type = TYPE_INDEX.variable;
    insertSpan(spans, start, end, type, 0, 10);
  }
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
  text: string,
  nlIdx: number[],
  startOff: number,
  endOff: number,
  tokenType: number,
  tokenModifiers: number,
) {
  const [sLine] = offsetToLC(nlIdx, startOff);
  const [eLine] = offsetToLC(nlIdx, endOff);
  for (let line = sLine; line <= eLine; line++) {
    const start = Math.max(startOff, nlIdx[line]);
    let end = Math.min(endOff, nlIdx[line + 1] ?? text.length);
    while (end > start && (text[end - 1] === "\n" || text[end - 1] === "\r")) end--;
    if (end > start) builder.push(line, start - nlIdx[line], end - start, tokenType, tokenModifiers);
  }
}

function semanticTypeForRole(role: SourceSymbol["role"]): number | undefined {
  switch (role) {
    case "module": return TYPE_INDEX.namespace;
    case "type":
    case "implementation": return TYPE_INDEX.type;
    case "procedure": return TYPE_INDEX.function;
    case "binding":
    case "global": return TYPE_INDEX.variable;
    case "parameter": return TYPE_INDEX.parameter;
    case "field": return TYPE_INDEX.property;
    case "variant": return TYPE_INDEX.enumMember;
  }
}

function bindingModifiers(text: string, symbol: SourceSymbol): number {
  let modifiers = symbol.detail?.startsWith("const ") ? MODIFIER_BIT.readonly : 0;
  if (symbol.role === "binding" || symbol.role === "global") {
    const declarationPrefix = text.slice(symbol.start, symbol.nameStart);
    if (/\bmut\s*$/.test(declarationPrefix)) modifiers |= MODIFIER_BIT.modification;
  }
  return modifiers;
}

function upsertSpan(spans: SemanticSpan[], start: number, end: number, type: number, modifiers: number, priority = 90): void {
  insertSpan(spans, start, end, type, modifiers, priority);
}

function insertSpan(spans: SemanticSpan[], start: number, end: number, type: number, modifiers = 0, priority = 20) {
  if (end <= start) return;
  spans.push({ start, end, type, modifiers, priority });
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
