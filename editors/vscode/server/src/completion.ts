// completion.ts — complétions LSP sans dépendance externe, robustes et contextuelles

import * as path from "node:path";
import { fileURLToPath } from "node:url";
import {
  CompletionItemKind,
  InsertTextFormat,
  Range,
  SymbolKind,
  MarkupKind
} from "vscode-languageserver/node";
import type {
  CompletionItem,
  Position,
  MarkupContent
} from "vscode-languageserver/node";
import type { TextEdit } from "vscode-languageserver-types";
import type { TextDocument } from "vscode-languageserver-textdocument";
import {
  BOOL_LITERALS,
  KEYWORDS,
  NIL_LITERALS,
} from "./languageFacts.js";
import { searchWorkspaceSymbols, SK } from "./indexer.js";

/* ============================================================================
 * Tables de base
 * ========================================================================== */

// Heuristiques de membres courants, proposées après un point.
const COMMON_MEMBERS: readonly { recv: RegExp; members: string[] }[] = [
  { recv: /(str(ing)?|text)$/i, members: ["len()", "is_empty()", "trim()", "to_upper()", "to_lower()"] },
  { recv: /(vec|list|array|slice)$/i, members: ["len()", "is_empty()", "push(${1:x})", "pop()", "clear()", "iter()"] },
  { recv: /(map|dict|table)$/i, members: ["len()", "get(${1:key})", "set(${1:key}, ${2:val})", "remove(${1:key})", "keys()", "values()"] }
];

// Idées d’imports rapides. Utiles lorsque la ligne commence par `import `.
const QUICK_IMPORTS: readonly string[] = [
  "std.core",
  "std.io",
  "std.net",
  "std.fs",
  "std.math",
  "std.time",
  "std.testing"
];

const SYMBOL_KIND_LABEL: Record<number, string> = {
  [SymbolKind.File]: "File",
  [SymbolKind.Module]: "Module",
  [SymbolKind.Namespace]: "Namespace",
  [SymbolKind.Package]: "Package",
  [SymbolKind.Class]: "Class",
  [SymbolKind.Method]: "Method",
  [SymbolKind.Property]: "Property",
  [SymbolKind.Field]: "Field",
  [SymbolKind.Constructor]: "Constructor",
  [SymbolKind.Enum]: "Enum",
  [SymbolKind.Interface]: "Interface",
  [SymbolKind.Function]: "Function",
  [SymbolKind.Variable]: "Variable",
  [SymbolKind.Constant]: "Constant",
  [SymbolKind.String]: "String",
  [SymbolKind.Number]: "Number",
  [SymbolKind.Boolean]: "Boolean",
  [SymbolKind.Array]: "Array",
  [SymbolKind.Object]: "Object",
  [SymbolKind.Key]: "Key",
  [SymbolKind.Null]: "Null",
  [SymbolKind.EnumMember]: "EnumMember",
  [SymbolKind.Struct]: "Struct",
  [SymbolKind.Event]: "Event",
  [SymbolKind.Operator]: "Operator",
  [SymbolKind.TypeParameter]: "TypeParameter"
};

/* ============================================================================
 * Snippets
 * ========================================================================== */

const SNIPPETS: CompletionItem[] = [
  ciSnippet("module", "Déclare un module", "Déclare le module courant.",
    "module ${1:my.module};"),
  ciSnippet("import", "Importe un module", "Importe un chemin depuis un autre module.",
    "import ${1:std::core};"),
  ciSnippet("fn", "Déclare une fonction",
    "Fonction avec paramètres et type de retour optionnel.",
    "fn ${1:name}(${2:params})${3: -> ${4:Type}} {\n\t$0\n}"),
  ciSnippet("main", "Point d’entrée", "Déclare la fonction principale.",
    "fn main() {\n\t$0\n}"),
  ciSnippet("struct", "Déclare une struct", "Structure avec des champs typés.",
    "struct ${1:Name} {\n\t${2:field}: ${3:Type},\n}"),
  ciSnippet("enum", "Déclare une enum", "Énumération avec variantes.",
    "enum ${1:Name} {\n\t${2:Variant1},\n\t${3:Variant2}\n}"),
  ciSnippet("union", "Déclare une union", "Union tagged simple.",
    "union ${1:Name} {\n\t${2:field}: ${3:Type},\n}"),
  ciSnippet("type", "Alias de type", "Déclare un alias de type.",
    "type ${1:Alias} = ${2:Existing};"),
  ciSnippet("match", "Match expression", "Expression de branchement avec motifs.",
    "match ${1:expr} {\n\t${2:Pattern} => ${3:expr},\n\t_ => ${0:default}\n}"),
  ciSnippet("ifelse", "If / Else", "Structure conditionnelle complète.",
    "if ${1:cond} {\n\t${2}\n} else {\n\t${0}\n}"),
  ciSnippet("for", "Boucle for", "Boucle for-in sur un itérable.",
    "for ${1:pat} in ${2:iter} {\n\t$0\n}"),
  ciSnippet("while", "Boucle while", "Boucle conditionnelle.",
    "while ${1:cond} {\n\t$0\n}"),
  ciSnippet("loop", "Boucle infinie", "Boucle sans fin, utiliser break pour sortir.",
    "loop {\n\t$0\n}"),
  ciSnippet("let", "Binding local", "Déclare une variable locale.",
    "let ${1:name}${2: : ${3:Type}}${4: = ${5:value}};"),
  ciSnippet("const", "Constante", "Déclare une constante globale.",
    "const ${1:NAME}: ${2:Type} = ${3:value};"),
  ciSnippet("static", "Statique", "Déclare un statique global.",
    "static ${1:NAME}: ${2:Type} = ${3:value};"),
  ciSnippet("print", "Affichage console", "Affiche un message sur la sortie standard.",
    "println(\"${1:msg}\");"),
  ciSnippet("doc", "Commentaire doc", "Ajoute un commentaire de documentation.",
    "/// ${1:Résumé}\n///\n/// ${0:Détails}")
];

/* ============================================================================
 * Extraction symbolique simple
 * ========================================================================== */

interface ExtractedSym { name: string; kind: SymbolKind; }

function extractSymbols(doc: TextDocument): ExtractedSym[] {
  const text = doc.getText();
  const rules: { rx: RegExp; kind: SymbolKind; g: number }[] = [
    { rx: /^\s*module\s+([A-Za-z_][\w:]*)/gm,                 kind: SymbolKind.Namespace, g: 1 },
    { rx: /^\s*(?:pub\s+)?fn\s+([A-Za-z_]\w*)/gm,             kind: SymbolKind.Function,  g: 1 },
    { rx: /^\s*(?:pub\s+)?struct\s+([A-Za-z_]\w*)/gm,         kind: SymbolKind.Struct,    g: 1 },
    { rx: /^\s*(?:pub\s+)?enum\s+([A-Za-z_]\w*)/gm,           kind: SymbolKind.Enum,      g: 1 },
    { rx: /^\s*(?:pub\s+)?union\s+([A-Za-z_]\w*)/gm,          kind: SymbolKind.Struct,    g: 1 },
    { rx: /^\s*(?:pub\s+)?type\s+([A-Za-z_]\w*)/gm,           kind: SymbolKind.Interface, g: 1 },
    { rx: /^\s*(?:pub\s+)?const\s+([A-Za-z_]\w*)/gm,          kind: SymbolKind.Constant,  g: 1 },
    { rx: /^\s*(?:pub\s+)?static\s+([A-Za-z_]\w*)/gm,         kind: SymbolKind.Variable,  g: 1 },
  ];

  const out: ExtractedSym[] = [];
  for (const { rx, kind, g } of rules) {
    rx.lastIndex = 0;
    let m: RegExpExecArray | null;
    while ((m = rx.exec(text))) {
      const name = m[g];
      if (name) out.push({ name, kind });
      if (m[0].length === 0) rx.lastIndex++;
    }
  }

  const seen = new Set<string>();
  return out.filter(s => {
    const k = `${s.kind}:${s.name}`;
    if (seen.has(k)) return false;
    seen.add(k);
    return true;
  });
}

function mapSymbolKindToCompletionKind(k: SymbolKind): CompletionItemKind {
  switch (k) {
    case SymbolKind.Function:     return CompletionItemKind.Function;
    case SymbolKind.Method:       return CompletionItemKind.Method;
    case SymbolKind.Struct:       return CompletionItemKind.Struct;
    case SymbolKind.Enum:         return CompletionItemKind.Enum;
    case SymbolKind.Interface:    return CompletionItemKind.Interface;
    case SymbolKind.Namespace:    return CompletionItemKind.Module;
    case SymbolKind.Class:        return CompletionItemKind.Class;
    case SymbolKind.Property:     return CompletionItemKind.Property;
    case SymbolKind.Field:        return CompletionItemKind.Field;
    case SymbolKind.Variable:     return CompletionItemKind.Variable;
    case SymbolKind.Constant:     return CompletionItemKind.Constant;
    case SymbolKind.Constructor:  return CompletionItemKind.Constructor;
    case SymbolKind.TypeParameter:return CompletionItemKind.TypeParameter;
    default:                      return CompletionItemKind.Text;
  }
}

/* ============================================================================
 * Utilitaires tokenisation et score
 * ========================================================================== */

function getLinePrefix(doc: TextDocument, pos: Position): string {
  return doc.getText(Range.create({ line: pos.line, character: 0 }, pos));
}

function currentToken(doc: TextDocument, pos: Position): { token: string; range: Range } {
  const line = doc.getText(Range.create({ line: pos.line, character: 0 }, { line: pos.line, character: 10_000 }));
  const idx = Math.min(pos.character, line.length);
  const left = line.slice(0, idx);
  const right = line.slice(idx);

  const leftMatch = /[A-Za-z_]\w*$/.exec(left);
  const rightMatch = /^\w*/.exec(right);

  const start = leftMatch ? idx - leftMatch[0].length : idx;
  const end = rightMatch ? idx + rightMatch[0].length : idx;

  const token = line.slice(start, end);
  return {
    token,
    range: Range.create({ line: pos.line, character: start }, { line: pos.line, character: end })
  };
}

function fuzzyScore(candidate: string, query: string): number {
  if (!query) return 1;
  if (candidate.startsWith(query)) return 2; // boost prefix strict
  let qi = 0;
  for (let i = 0; i < candidate.length && qi < query.length; i++) {
    if (candidate[i].toLowerCase() === query[qi].toLowerCase()) qi++;
  }
  return 0.5 + qi / (2 * Math.max(1, query.length));
}

/* ============================================================================
 * Complétions contextuelles
 * ========================================================================== */

function diagnosticsCompletion(linePrefix: string): CompletionItem[] {
  const items: CompletionItem[] = [];
  if (linePrefix.includes("???")) {
    items.push({
      label: "TODO",
      kind: CompletionItemKind.Text,
      insertText: "TODO",
      detail: "Remplacer ??? par TODO",
      documentation: md("Transforme la séquence `???` en TODO/FIXME."),
      sortText: tier(0)
    });
  }
  if (/^\s*import\s+/.test(linePrefix)) {
    for (const imp of QUICK_IMPORTS) {
      items.push({
        label: imp,
        kind: CompletionItemKind.Module,
        detail: "Import rapide",
        documentation: md(`Ajoute l'import \`${imp}\`.`),
        sortText: tier(0, imp)
      });
    }
  }
  return items;
}

function memberCompletion(leftOfCursor: string): CompletionItem[] {
  const items: CompletionItem[] = [];
  const memberCtx = /([A-Za-z_]\w*)\.\s*$/.exec(leftOfCursor);
  if (!memberCtx) return items;
  const recv = memberCtx[1];

  for (const group of COMMON_MEMBERS) {
    if (group.recv.test(recv)) {
      for (const m of group.members) {
        items.push({
          label: m,
          kind: CompletionItemKind.Method,
          detail: `Méthode de ${recv}`,
          documentation: md(`Proposition basée sur le nom \`${recv}\`.`),
          sortText: tier(0, m),
          insertText: m,
          insertTextFormat: /[\$\{]/.test(m) ? InsertTextFormat.Snippet : InsertTextFormat.PlainText
        });
      }
    }
  }
  // Fallback générique
  if (items.length === 0) {
    for (const m of ["len()", "is_empty()", "to_string()", "clone()", "dbg()"]) {
      items.push({
        label: m,
        kind: CompletionItemKind.Method,
        detail: `Méthode`,
        documentation: md("Proposition générique."),
        sortText: tier(1, m),
        insertText: m,
        insertTextFormat: InsertTextFormat.PlainText
      });
    }
  }
  return items;
}

function workspaceSymbolCompletions(
  doc: TextDocument,
  token: string,
  range: Range,
  context: "import" | "general"
): CompletionItem[] {
  const normalized = token.trim();
  if (!normalized && context === "general") return [];
  const limit = context === "import" ? 200 : 80;
  const results = searchWorkspaceSymbols(normalized, limit);
  if (results.length === 0) return [];

  const items: CompletionItem[] = [];
  const seen = new Set<string>();

  for (const sym of results) {
    if (!sym?.name) continue;
    if (sym.uri === doc.uri) continue;
    if (context === "general" && fuzzyScore(sym.name, token) <= 0) continue;
    if (context === "import" && !isImportableKind(sym.kind)) continue;

    const key = `${sym.name}|${sym.uri}|${sym.kind}`;
    if (seen.has(key)) continue;
    seen.add(key);

    const kind = mapSymbolKindToCompletionKind(sym.kind as unknown as SymbolKind);
    const detailParts = [`${SYMBOL_KIND_LABEL[sym.kind] ?? "Symbole"}`];
    if (sym.uri) {
      const base = basenameFromUri(sym.uri);
      if (base) detailParts.push(base);
    }

    items.push({
      label: sym.name,
      kind,
      detail: detailParts.join(" — "),
      documentation: md(`Symbole workspace \`${sym.name}\`.`),
      sortText: tier(context === "import" ? 0 : 1, `ws:${sym.name}`),
      filterText: sym.name,
      textEdit: edit(range, sym.name),
      labelDetails: context === "import"
        ? { detail: "workspace" }
        : { description: SYMBOL_KIND_LABEL[sym.kind] ?? "symbol" }
    });
    if (context === "general" && items.length >= 40) break;
    if (context === "import" && items.length >= 60) break;
  }

  return items;
}

function isImportableKind(kind: SK): boolean {
  return kind === SK.Module || kind === SK.Namespace;
}

function basenameFromUri(uri: string): string {
  if (!uri) return "";
  try {
    if (uri.startsWith("file://")) {
      return path.basename(fileURLToPath(uri));
    }
  } catch {
    // ignore decoding issues
  }
  try {
    return path.basename(new URL(uri).pathname);
  } catch {
    return uri;
  }
}

/* ============================================================================
 * API
 * ========================================================================== */

export function provideCompletions(doc: TextDocument, position: Position): CompletionItem[] {
  const items: CompletionItem[] = [];
  const { token, range } = currentToken(doc, position);
  const linePrefix = getLinePrefix(doc, position);

  // Mots-clés
  for (const kw of KEYWORDS) {
    const score = fuzzyScore(kw, token);
    if (score > 0) {
      items.push({
        label: kw,
        kind: CompletionItemKind.Keyword,
        detail: "Mot-clé",
        documentation: md(`Mot-clé du langage \`${kw}\`.`),
        sortText: tier(2 - (score >= 2 ? 1 : 0), kw),
        filterText: kw,
        textEdit: edit(range, kw),
        preselect: kw === "fn" || kw === "let",
        commitCharacters: [" ", "\t", "(", ")", "{", "}", "[", "]", ";", ",", "."],
        labelDetails: { description: "keyword" }
      });
    }
  }

  // Littéraux
  for (const lit of [...BOOL_LITERALS, ...NIL_LITERALS]) {
    if (fuzzyScore(lit, token) > 0) {
      items.push({
        label: lit,
        kind: CompletionItemKind.Value,
        detail: "Littéral",
        documentation: md(`Littéral \`${lit}\`.`),
        sortText: tier(2, `~${lit}`),
        textEdit: edit(range, lit),
        labelDetails: { description: "literal" }
      });
    }
  }

  // Snippets
  for (const it of SNIPPETS) {
    items.push({
      ...it,
      sortText: tier(3, it.label),
      filterText: it.label
    });
  }

  // Symboles du document
  for (const s of extractSymbols(doc)) {
    const label = s.name;
    if (fuzzyScore(label, token) > 0) {
      items.push({
        label,
        kind: mapSymbolKindToCompletionKind(s.kind),
        detail: `Symbole (${SYMBOL_KIND_LABEL[s.kind] ?? "?"})`,
        documentation: md("Déclaré dans ce document."),
        sortText: tier(1, label),
        filterText: label,
        textEdit: edit(range, label),
        labelDetails: { description: SYMBOL_KIND_LABEL[s.kind] ?? "symbol" }
      });
    }
  }

  // Contexte: membres après un point
  items.push(...memberCompletion(linePrefix));

  // Contexte: import et autres heuristiques
  const isImportContext = /^\s*import\s+/.test(linePrefix);

  items.push(...diagnosticsCompletion(linePrefix).map(ci => ({
    ...ci,
    textEdit: ci.insertText ? undefined : edit(range, ci.label),
    sortText: ci.sortText ?? tier(0, ci.label)
  })));

  if (isImportContext) {
    items.push(...workspaceSymbolCompletions(doc, token, range, "import"));
  } else if (token.length >= 2) {
    items.push(...workspaceSymbolCompletions(doc, token, range, "general"));
  }

  return dedupe(items, it => `${it.label}|${it.kind}|${it.sortText ?? ""}`);
}

export function resolveCompletion(item: CompletionItem): CompletionItem {
  if (!item.documentation && typeof item.label === "string") {
    item.documentation = md(`Entrée de complétion \`${item.label}\`.`);
  }
  return item;
}

export function triggerCharacters(): string[] {
  return [".", ":", ">", "=", " ", "(", "[", "{", "\"", "'"];
}

/* ============================================================================
 * Helpers
 * ========================================================================== */

function ciSnippet(label: string, detail: string, documentation: string, insertText: string): CompletionItem {
  return {
    label,
    kind: CompletionItemKind.Snippet,
    insertText,
    insertTextFormat: InsertTextFormat.Snippet,
    detail,
    documentation: md(documentation)
  };
}

function tier(n: number, suffix = ""): string {
  // 0 = top, 1 = symboles, 2 = mots-clés/littéraux, 3 = snippets
  return `${n.toString(10).padStart(2, "0")}_${suffix}`;
}

function md(value: string): MarkupContent {
  return { kind: MarkupKind.Markdown, value };
}

function edit(range: Range, newText: string): TextEdit {
  return { range, newText };
}

function dedupe<T>(arr: T[], key: (t: T) => string): T[] {
  const seen = new Set<string>();
  const out: T[] = [];
  for (const el of arr) {
    const k = key(el);
    if (seen.has(k)) continue;
    seen.add(k);
    out.push(el);
  }
  return out;
}
